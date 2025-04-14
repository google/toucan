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

import sys

filename = sys.argv[1]

with open(filename, 'rb') as f:
  contents = f.read()

contents = contents.replace(b'\r\n', b'\n')
contents = contents.replace(b'\\', b'/')

with open(filename, 'wb') as f:
  f.write(contents)
