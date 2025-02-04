/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.
   Copyright 2021 The CFU PLayground Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/kernels/internal/reference/integer_ops/mnv2_conv.h"

#include "mnv2_cfu.h"
#include "tf_util/print_params.h"

//
// This file contains specialized conv 2D implementations to support
// MobileNet v2 models
//
namespace tflite {
namespace reference_integer_ops {

static inline int32_t macc(const int8_t* input_data, const int8_t* filter_data,
                           int out_channel, int in_channel, int input_depth,
                           int32_t input_offset) {
  int32_t input_val = input_data[in_channel];
  int32_t filter_val = filter_data[out_channel * input_depth + in_channel];
  return filter_val * (input_val + input_offset);
}

static inline int32_t accumulate(const int8_t* input_data,
                                 const int8_t* filter_data, int out_channel,
                                 int input_depth, int32_t input_offset) {
  int32_t acc = 0;
  for (int in_channel = 0; in_channel < input_depth; ++in_channel) {
    acc += macc(input_data, filter_data, out_channel, in_channel, input_depth,
                input_offset);
  }
  return acc;
}

// Fixed-point per-channel-quantization convolution reference kernel.
void Mnv2ConvPerChannel1x1(
    const ConvParams& params, const int32_t* output_multiplier,
    const int32_t* output_shift, const RuntimeShape& input_shape,
    const int8_t* input_data, const RuntimeShape& filter_shape,
    const int8_t* filter_data, const RuntimeShape& bias_shape,
    const int32_t* bias_data, const RuntimeShape& output_shape,
    int8_t* output_data) {
  // Get parameters.
  const int32_t input_offset = params.input_offset;  // r = s(q - Z)
  const int32_t output_offset = params.output_offset;

  // Set min and max value of the output.
  const int32_t output_activation_min = params.quantized_activation_min;
  const int32_t output_activation_max = params.quantized_activation_max;

  // Consistency check
  TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
  TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);
  const int input_depth = MatchingDim(input_shape, 3, filter_shape, 3);
  const int output_depth = MatchingDim(filter_shape, 0, output_shape, 3);
  if (bias_data) {
    TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
  }
  // Check dimensions of the tensors.
  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);

  // Set parameters for op
  CFU_SET_INPUT_DEPTH(input_depth);
  CFU_SET_OUTPUT_DEPTH(output_depth);
  CFU_SET_INPUT_OFFSET(input_offset);
  CFU_SET_OUTPUT_OFFSET(output_offset);
  CFU_SET_ACTIVATION_MIN(output_activation_min);
  CFU_SET_ACTIVATION_MAX(output_activation_max);

  // Do the processing in batches, by output channel. batch size is number of
  // output channels processed per batch and it is chosen to avoid overflowing
  // filter_data memory, and then rounded down to a multiple of 4.
  //
  // For each batch, the entire input will be read once
  int channels_per_batch =
      std::min(output_depth, (NUM_FILTER_DATA_BYTES / input_depth) / 4 * 4);
  int num_pixels = output_height * output_width;
  int num_batches =
      (channels_per_batch - 1 + output_depth) / channels_per_batch;

  for (int batch = 0; batch < num_batches; batch++) {
    int batch_base = batch * channels_per_batch;
    int batch_end = std::min(output_depth, batch_base + channels_per_batch);
    int batch_size = batch_end - batch_base;

    // Load up parameters
    CFU_SET_OUTPUT_BATCH_SIZE(batch_size);
    for (int out_channel = batch_base; out_channel < batch_end; ++out_channel) {
      CFU_STORE_OUTPUT_MULTIPLIER(output_multiplier[out_channel]);
      CFU_STORE_OUTPUT_SHIFT(output_shift[out_channel]);
      CFU_STORE_OUTPUT_BIAS(bias_data[out_channel]);
    }

    // Reset input and output pointers
    const int8_t* input_ptr = input_data;
    int8_t* output_ptr = output_data + batch_base;
    for (int p = 0; p < num_pixels; p++) {
      for (int out_channel = batch_base; out_channel < batch_end;
           ++out_channel) {
        int32_t acc = accumulate(input_ptr, filter_data, out_channel,
                                 input_depth, input_offset);

        int32_t out = CFU_POST_PROCESS(acc);
        *(output_ptr++) = static_cast<int8_t>(out);
      }
      output_ptr += output_depth - batch_size;
      input_ptr += input_depth;
    }
  }
}

}  // namespace reference_integer_ops
}  // namespace tflite
