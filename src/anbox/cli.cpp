/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 *
 */

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "anbox/cli.h"

namespace cli = anbox::cli;
namespace po = boost::program_options;

namespace {
namespace pattern {
static constexpr const char* help_for_command_with_subcommands =
    "NAME:\n"
    "    %1% - %2%\n"
    "\n"
    "USAGE:\n"
    "    %3% [command options] [arguments...]";

static constexpr const char* commands = "COMMANDS:";
static constexpr const char* command = "    %1% %2%";

static constexpr const char* options = "OPTIONS:";
static constexpr const char* option = "    --%1% %2%";
}

void add_to_desc_for_flags(po::options_description& desc,
                           const std::set<cli::Flag::Ptr>& flags) {
  for (auto flag : flags) {
    if (flag->with_value()) {
      auto vf = std::static_pointer_cast<cli::ValueFlag>(flag);
      auto v = po::value<std::string>()->notifier(
          [vf](const std::string& s) { vf->notify(s); });
      desc.add_options()(flag->name().as_string().c_str(), v,
                         flag->description().as_string().c_str());
    } else {
      desc.add_options()(flag->name().as_string().c_str(),
                         flag->description().as_string().c_str());
    }
  }
}
}

std::vector<std::string> cli::args(int argc, char** argv) {
  std::vector<std::string> result;
  for (int i = 1; i < argc; i++) result.push_back(argv[i]);
  return result;
}

const cli::Name& cli::Flag::name() const { return name_; }

const cli::Description& cli::Flag::description() const { return description_; }

bool cli::Flag::is_set() const { return set_; }

void cli::Flag::mark_set() { set_ = true; }

bool cli::Flag::with_value() const { return false; }

cli::Flag::Flag(const Name& name, const Description& description)
    : name_{name}, description_{description} {}

bool cli::ValueFlag::with_value() const { return true; }

cli::Command::FlagsWithInvalidValue::FlagsWithInvalidValue()
    : std::runtime_error{"Flags with invalid value"} {}

cli::Command::FlagsMissing::FlagsMissing()
    : std::runtime_error{"Flags are missing in command invocation"} {}

cli::Name cli::Command::name() const { return name_; }

cli::Usage cli::Command::usage() const { return usage_; }

cli::Description cli::Command::description() const { return description_; }

cli::Command::Command(const cli::Name& name, const cli::Usage& usage,
                      const cli::Description& description)
    : name_(name), usage_(usage), description_(description) {}

cli::CommandWithSubcommands::CommandWithSubcommands(
    const Name& name, const Usage& usage, const Description& description)
    : Command{name, usage, description} {
  command(std::make_shared<cmd::Help>(*this));
}

cli::CommandWithSubcommands& cli::CommandWithSubcommands::command(
    const Command::Ptr& command) {
  commands_[command->name().as_string()] = command;
  return *this;
}

cli::CommandWithSubcommands& cli::CommandWithSubcommands::flag(
    const Flag::Ptr& flag) {
  flags_.insert(flag);
  return *this;
}

void cli::CommandWithSubcommands::help(std::ostream& out) {
  out << boost::format(pattern::help_for_command_with_subcommands) %
             name().as_string() % usage().as_string() % name().as_string()
      << std::endl;

  if (flags_.size() > 0) {
    out << std::endl
        << pattern::options << std::endl;
    for (const auto& flag : flags_)
      out << boost::format(pattern::option) % flag->name() % flag->description()
          << std::endl;
  }

  if (commands_.size() > 0) {
    out << std::endl
        << pattern::commands << std::endl;
    for (const auto& cmd : commands_) {
      if (cmd.second)
        out << boost::format(pattern::command) % cmd.second->name() %
                   cmd.second->description()
            << std::endl;
    }
  }
}

int cli::CommandWithSubcommands::run(const cli::Command::Context& ctxt) {
  po::positional_options_description pdesc;
  pdesc.add("command", 1);

  po::options_description desc("Options");
  desc.add_options()("command", po::value<std::string>()->required(),
                     "the command to be executed");

  add_to_desc_for_flags(desc, flags_);

  try {
    po::variables_map vm;
    auto parsed = po::command_line_parser(ctxt.args)
                      .options(desc)
                      .positional(pdesc)
                      .style(po::command_line_style::unix_style)
                      .allow_unregistered()
                      .run();

    po::store(parsed, vm);
    po::notify(vm);

    auto cmd = commands_[vm["command"].as<std::string>()];
    if (!cmd) {
      ctxt.cout << "Unknown command '" << vm["command"].as<std::string>() << "'"
                << std::endl;
      help(ctxt.cout);
      return EXIT_FAILURE;
    }

    return cmd->run(cli::Command::Context{
        ctxt.cin, ctxt.cout,
        po::collect_unrecognized(parsed.options, po::include_positional)});
  } catch (const po::error& e) {
    ctxt.cout << e.what() << std::endl;
    help(ctxt.cout);
    return EXIT_FAILURE;
  }

  return EXIT_FAILURE;
}

cli::CommandWithFlagsAndAction::CommandWithFlagsAndAction(
    const Name& name, const Usage& usage, const Description& description)
    : Command{name, usage, description} {}

cli::CommandWithFlagsAndAction& cli::CommandWithFlagsAndAction::flag(
    const Flag::Ptr& flag) {
  flags_.insert(flag);
  return *this;
}

cli::CommandWithFlagsAndAction& cli::CommandWithFlagsAndAction::action(
    const Action& action) {
  action_ = action;
  return *this;
}

int cli::CommandWithFlagsAndAction::run(const Context& ctxt) {
  po::options_description cd(name().as_string());

  bool help_requested{false};
  cd.add_options()("help", po::bool_switch(&help_requested),
                   "produces a help message");

  add_to_desc_for_flags(cd, flags_);

  try {
    po::variables_map vm;
    auto parsed = po::command_line_parser(ctxt.args)
                      .options(cd)
                      .style(po::command_line_style::unix_style)
                      .allow_unregistered()
                      .run();
    po::store(parsed, vm);
    po::notify(vm);

    if (help_requested) {
      help(ctxt.cout);
      return EXIT_SUCCESS;
    }

    for (auto &flag : flags_) {
      if (vm.count(flag->name()))
        flag->mark_set();
    }

    return action_(cli::Command::Context{
        ctxt.cin, ctxt.cout,
        po::collect_unrecognized(parsed.options, po::include_positional)});
  } catch (const po::error& e) {
    ctxt.cout << e.what() << std::endl;
    help(ctxt.cout);
    return EXIT_FAILURE;
  }

  return EXIT_FAILURE;
}

bool cli::CommandWithFlagsAndAction::is_flag_set(const Name& name) {
  for (const auto &flag : flags_) {
    if (flag->name() == name)
      return flag->is_set();
  }
  return false;
}

void cli::CommandWithFlagsAndAction::help(std::ostream& out) {
  out << boost::format(pattern::help_for_command_with_subcommands) %
             name().as_string() % description().as_string() % name().as_string()
      << std::endl;

  if (flags_.size() > 0) {
    out << std::endl
        << boost::format(pattern::options) << std::endl;
    for (const auto& flag : flags_)
      out << boost::format(pattern::option) % flag->name() % flag->description()
          << std::endl;
  }
}

cli::cmd::Help::Help(Command& cmd)
    : Command{cli::Name{"help"}, cli::Usage{"prints a short help message"},
              cli::Description{"prints a short help message"}},
      command{cmd} {}

// From Command
int cli::cmd::Help::run(const Context& context) {
  command.help(context.cout);
  return EXIT_FAILURE;
}

void cli::cmd::Help::help(std::ostream& out) { command.help(out); }

cli::Flag::Ptr cli::make_flag(const cli::Name& name, const cli::Description& description) {
  return std::make_shared<Flag>(name, description);
}
