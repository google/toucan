#!/usr/bin/env python3

# Copyright 2024 The Toucan Authors
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

import os
import sys
import subprocess

vswhere = os.path.expandvars(os.path.join('%ProgramFiles(x86)%', 'Microsoft Visual Studio', 'Installer', 'vswhere.exe'))
paths = subprocess.check_output([vswhere, '-products', '*', '-property', 'installationPath']);
paths = paths.decode('utf-8').splitlines()
if paths:
  path = os.path.join(paths[0], 'VC')
  sys.stdout.write(path)
