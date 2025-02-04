#!/usr/bin/env python3
# Copyright 2021 The CFU-Playground Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

__package__ = 'nmigen_cfu'

from nmigen import Signal
from nmigen.sim import Settle

from .util import TestBase, ValueBuffer

import unittest


class ValueBufferTest(TestBase):
    def create_dut(self):
        self.capture = Signal()
        self.in_signal = Signal(4)
        return ValueBuffer(self.in_signal, self.capture)

    def test(self):
        DATA = [
            ((0, 0), 0),
            ((1, 5), 5),
            ((0, 3), 5),
            ((0, 2), 5),
            ((0, 2), 5),
            ((1, 2), 2),
            ((0, 2), 2),
            ((0, 2), 2),
        ]

        def process():
            for n, ((capture, in_sig), expected_output) in enumerate(DATA):
                yield self.in_signal.eq(in_sig)
                yield self.capture.eq(capture)
                yield Settle()
                self.assertEqual((yield self.dut.output), expected_output, f"cycle={n}")
                yield
        self.run_sim(process, True)


if __name__ == '__main__':
    unittest.main()
