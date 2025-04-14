#!/usr/bin/env python3

# Copyright 2023 The Toucan Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import glob;
import os;
import subprocess;
import sys;
files = glob.glob(os.path.relpath(os.path.join(os.path.dirname(__file__), '*.t')));
files = sorted(files)
debug_or_release = 'Release'
if sys.platform == 'win32':
  exe_path = os.path.join('out', debug_or_release, 'tj.exe');
else:
  exe_path = os.path.join('out', debug_or_release, 'tj');

for file in files:
  print('test/' + os.path.basename(file));
  sys.stdout.flush();
  subprocess.call([exe_path, file]);
