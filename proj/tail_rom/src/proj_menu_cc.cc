/*
 * Copyright 2021 The CFU-Playground Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "proj_menu.h"

#include <stdio.h>

#include "calc_once_data.h"
#include "menu.h"

namespace {

void do_reset_cache() { calculate_once::SetCache(NULL); }

}  // anonymous namespace

static struct Menu MENU = {
    "Project Menu",
    "project",
    {
        MENU_ITEM('0', "reset cache", do_reset_cache),
        MENU_END,
    },
};

void do_proj_menu() { menu_run(&MENU); }
