#!/usr/bin/python


import argparse
import re
import shutil
import logging
import csv
import os.path
import platform
import subprocess


_TRAMPOLINE = "ci.py"
_FLAGS = "ci.csv"


class Ci(object):

  def __init__(self, argv):
    self.exe = argv[0]
    self.command = argv[1]
    self.argv = argv

  # Build the option parser to use when this script is called not as a
  # trampoline.
  def build_option_parser(self):
    parser = argparse.ArgumentParser()
    commands = sorted(self.get_handlers().keys())
    parser.add_argument('command', choices=commands)
    parser.add_argument('--config', default=None, help='The root configuration')
    return parser

  def parse_options(self):
    return self.build_option_parser().parse_args(self.argv[1:])

  # Returns a dict mapping command names to their handler methods (methods that
  # start with "handle_").
  def get_handlers(self):
    result = {}
    for (name, value) in Ci.__dict__.items():
      match = re.match(r'handle_(.*)', name)
      if match is None:
        continue
      action = match.group(1)
      result[action] = value
    return result

  # Performs the "begin" action: copies this file to the current directory (such
  # that it can be called through "python ci.py", the syntax of which is the
  # same across platforms) and creates ci.csv with the options passed to this
  # command.
  def handle_begin(self):
    options = self.parse_options()
    shutil.copyfile(self.exe, _TRAMPOLINE)
    tools = os.path.dirname(self.exe)
    with open(_FLAGS, "wb") as file:
      writer = csv.writer(file)
      writer.writerow(["config", options.config])
      writer.writerow(["tools", tools])

  def get_flags(self):
    flags = {}
    with open(_FLAGS, "rb") as file:
      reader = csv.reader(file)
      for row in reader:
        flags[row[0]] = row[1]
    return flags

  def get_shell_script_name(self, base):
    if self.is_windows():
      return "%s.bat" % base
    else:
      return "%s.sh" % base

  def is_windows(self):
    return platform.system() == "Windows"

  def handle_run(self):
    flags = self.get_flags()
    rest = self.argv[2:]
    mkmk_run = ["mkmk", "run", "--config", flags["config"]] + rest
    if self.is_windows():
      self.sh([mkmk_run])
    else:
      enter_devenv_base = os.path.join(flags['tools'], "enter-devenv")
      enter_devenv = ["source", self.get_shell_script_name(enter_devenv_base)]
      self.sh([enter_devenv, mkmk_run], shell="bash")

  def sh(self, command_lists, shell=None):
    commands = [" ".join(c) for c in command_lists]
    command = " &&  ".join(commands)
    if shell is None:
      subprocess.check_call(command, shell=True)
    else:
      subprocess.check_call([shell, "-c", command])


  def main(self):
    handlers = self.get_handlers()
    if not self.command in handlers:
      logging.error("Unknown command %s", self.command)
      sys.exit(1)
    handler = handlers[self.command]
    return handler(self)


if __name__ == '__main__':
  import sys
  Ci(sys.argv).main()
