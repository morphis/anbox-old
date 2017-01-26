/*
 * Copyright (C) 2016 Simon Fels <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "anbox/input/manager.h"
#include "anbox/config.h"
#include "anbox/input/device.h"
#include "anbox/runtime.h"
#include "anbox/utils.h"

#include <boost/format.hpp>

namespace anbox {
namespace input {
Manager::Manager(const std::shared_ptr<Runtime> &runtime) : runtime_(runtime) {
  utils::ensure_paths({SystemConfiguration::instance().input_device_dir()});
}

Manager::~Manager() {}

std::shared_ptr<Device> Manager::create_device() {
  const auto id = next_id();
  const auto path = build_device_path(id);
  auto device = Device::create(path, runtime_);
  devices_.insert({id, device});
  return device;
}

std::uint32_t Manager::next_id() {
  static std::uint32_t next_id = 0;
  return next_id++;
}

std::string Manager::build_device_path(const std::uint32_t &id) {
  return (boost::format("%1%/event%2%") % SystemConfiguration::instance().input_device_dir() % id).str();
}

}  // namespace input
}  // namespace anbox
