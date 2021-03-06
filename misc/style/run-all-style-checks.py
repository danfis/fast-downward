#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
Run some syntax checks. Return 0 if all tests pass and 1 otherwise.
"""

from __future__ import print_function

import glob
import os
import re
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SRC_DIR = os.path.join(REPO, "src")


def check_translator_style():
    output = subprocess.check_output([
        "./reindent.py", "--dryrun", "--recurse", "--verbose",
        os.path.join(SRC_DIR, "translate")], cwd=DIR).decode("utf-8")
    ok = True
    for line in output.splitlines():
        match = re.match("^checking (.+) ... changed.$", line)
        if match:
            ok = False
            print('Wrong format detected in %s. '
                  'Please run "./reindent.py -r ../src/translate"' %
                  match.group(1))
    return ok


def _run_pyflakes(path):
    python_files = []
    for root, dirs, files in os.walk(path):
        python_files.extend([
            os.path.join(root, f) for f in files
            if f.endswith(".py") and f != "__init__.py"])
    try:
        return subprocess.check_call(["pyflakes"] + python_files) == 0
    except OSError as err:
        if err.errno == 2:
            print(
                "Python style checks need pyflakes. Please install it "
                "with \"sudo apt-get install pyflakes\".")
        else:
            raise

def check_translator_pyflakes():
    return _run_pyflakes(os.path.join(SRC_DIR, "translate"))

def check_driver_pyflakes():
    return _run_pyflakes(os.path.join(REPO, "driver"))


def check_include_guard_convention():
    return subprocess.call("./check-include-guard-convention.py", cwd=DIR) == 0


def check_cc_files():
    search_dir = os.path.join(SRC_DIR, "search")
    cc_files = (
        glob.glob(os.path.join(search_dir, "*.cc")) +
        glob.glob(os.path.join(search_dir, "*", "*.cc")))
    return subprocess.call(["./check-cc-file.py"] + cc_files, cwd=DIR) == 0


def check_preprocessor_and_search_style():
    output = subprocess.check_output(["hg", "uncrustify"])
    if output:
        print('Run "hg uncrustify -m" to fix the style in the following files:')
        print(output.rstrip())
    return not output


def main():
    tests = [test for test_name, test in globals().items()
             if test_name.startswith("check_")]
    if all(test() for test in tests):
        print("All style checks passed")
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
