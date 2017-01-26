/*
* Copyright (C) 2011-2015 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef RENDER_API_H
#define RENDER_API_H

#include <string>

#include "emugl/common/logging.h"

typedef struct {
  logger_t coarse;
  logger_t fine;
} emugl_logger_struct;

namespace anbox {
namespace graphics {
namespace emugl {
struct GLLibraries {
  std::string egl_path;
  std::string glesv1_path;
  std::string glesv2_path;
};

bool initialize(const GLLibraries &libs, emugl_logger_struct log_funcs, logger_t crash_func);
}  // namespace emugl
}  // namespace graphics
}  // namespace anbox

#endif
