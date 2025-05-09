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

import json

with open('stanford-dragon.json', 'r') as file:
  data = json.load(file)

cells = data["cells"]
positions = data["positions"]

count = 0
print("var dragonTriangles : [" + str(len(cells)) + "][3]uint = {");
for cell in cells:
  count += 1
  possibleComma = "," if (count < len(cells)) else ""
  print("  {" + str(cell[0]) + ", " + str(cell[1]) + ", " + str(cell[2]) + "}" + possibleComma)

print("};");

count = 0;
print("var dragonVertices : [" + str(len(positions)) + "]float<3> = {");
for position in positions:
  count += 1
  possibleComma = "," if (count < len(positions)) else ""
  print("  {" + str(position[0]) + ", " + str(position[1]) + ", " + str(position[2]) + "}" + possibleComma)
print("};");
