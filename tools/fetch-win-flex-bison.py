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

import os, tempfile, zipfile
from urllib.request import urlopen

win_flex_bison_zip = os.path.join(tempfile.mkdtemp(), 'win_flex_bison.zip')
with open(win_flex_bison_zip, 'wb') as f:
  url = "https://github.com/lexxmark/winflexbison/releases/download/v2.5.25/win_flex_bison-2.5.25.zip"
  f.write(urlopen(url).read())

with zipfile.ZipFile(win_flex_bison_zip, 'r') as f:
  f.extractall('third_party/winflexbison')
