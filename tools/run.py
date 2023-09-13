#!/usr/bin/python3
import sys
import subprocess
args = sys.argv
args.pop(0)
subprocess.check_call(args)
