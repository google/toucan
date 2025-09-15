#!/usr/bin/env python3

# Copyright 2025 The Toucan Authors
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

import argparse
import os
import plistlib
import shutil
import subprocess
import tempfile

argparser = argparse.ArgumentParser(description="Create an .ipa file")
argparser.add_argument('--app-file')
argparser.add_argument('--ipa-file')
args = vars(argparser.parse_args())

app_file = args['app_file']
ipa_file = args['ipa_file']
tempdirname = tempfile.mkdtemp()
payload_dir = tempdirname + "/Payload"
os.mkdir(payload_dir)
os.symlink(app_file, payload_dir + "/" + os.path.basename(app_file))
os.chdir(tempdirname)
subprocess.check_call(["zip", "-r", ipa_file, "Payload"])
shutil.rmtree(tempdirname)
