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

import os
import subprocess
import sys

# Equivalent to: rm -f $2 && $1 rcs $2 @$3

ar, output, rspfile = sys.argv[1:]

if os.path.exists(output):
  os.remove(output)

if sys.platform != 'darwin':
  sys.exit(subprocess.call([ar, "rcs", output, "@" + rspfile]))

# Mac ar doesn't support @rspfile syntax.
objects = open(rspfile).read().split()
# It also spams stderr with warnings about objects having no symbols.
pipe = subprocess.Popen([ar, "rcs", output] + objects, stderr=subprocess.PIPE)
_, err = pipe.communicate()
for line in err.splitlines():
  if b'has no symbols' not in line:
    sys.stderr.write(line.decode("utf-8") + '\n')
sys.exit(pipe.returncode)
