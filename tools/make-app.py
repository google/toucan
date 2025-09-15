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
import json
import os
import plistlib
import shutil
import subprocess
import tempfile

argparser = argparse.ArgumentParser(description="Create an iOS or MacOS .app")
argparser.add_argument('--target-name')
argparser.add_argument('--target-os')
argparser.add_argument('--mobile-provision')
argparser.add_argument('--team-identifier')
argparser.add_argument('--codesign-identity')
argparser.add_argument('--app-icon')
argparser.add_argument('--minimum-deployment-target')
argparser.add_argument('--enable-debugging', action='store_true')
args = vars(argparser.parse_args())

target_name = args['target_name']
target_os = args['target_os']

dest_app_path = target_name + ".app/"
if os.path.exists(dest_app_path):
  shutil.rmtree(dest_app_path)

if target_os == "mac":
  dest_contents_path = dest_app_path + "Contents/"
  dest_os_path = dest_contents_path + "MacOS/"
  dest_resources_path = dest_contents_path + "Resources/"
else:
  dest_os_path = dest_contents_path = dest_resources_path = dest_app_path

os.makedirs(dest_os_path)

if target_os == "mac":
  dylibs = [
    "libdawn_native.dylib",
    "libdawn_proc.dylib",
  ]
  for dylib in dylibs:
    shutil.copy2(dylib, dest_os_path + dylib)

shutil.copy2(target_name, dest_os_path + target_name)

assets_dir_name = tempfile.mkdtemp()

if target_os == "ios":
  appiconset_contents = {
    "images" : [
      {
        "filename" : "AppIcon.png",
        "idiom" : "universal",
        "platform" : "ios",
        "size" : "1024x1024",
      },
    ],
  }
else:
  appiconset_contents = {
    "images" : [
      {
        "filename" : "AppIcon.png",
        "idiom" : "mac",
        "scale" : "2x",
        "size" : "512x512",
      },
    ],
  }

app_icon_dir_name = assets_dir_name + '/AppIcon.appiconset'
os.mkdir(app_icon_dir_name)
app_icon_png = app_icon_dir_name + '/AppIcon.png'
app_icon_svg = args['app_icon']

with open(app_icon_dir_name + '/Contents.json', 'w') as f:
  json.dump(appiconset_contents, f, indent=2)

subprocess.check_call(['sips', app_icon_svg, '-o', app_icon_png,  '-s', 'format', 'png', '--resampleHeightWidth', '1024', '1024'])
partial_info_plist = tempfile.mkstemp()[1]
minimum_deployment_target = args['minimum_deployment_target']

platform = 'iphoneos' if target_os == 'ios' else 'macosx'
if not os.path.exists(dest_resources_path):
  os.mkdir(dest_resources_path)
subprocess.check_call(['actool', assets_dir_name, '--compile', dest_resources_path, '--output-format', 'human-readable-text', '--app-icon', 'AppIcon', '--output-partial-info-plist', partial_info_plist, '--platform', platform, '--minimum-deployment-target', minimum_deployment_target])

info_plist_file = dest_contents_path + "Info.plist"

info = {
  'CFBundleExecutable': target_name,
  'CFBundleIdentifier': 'org.toucanlang.sample.' + target_name,
  'CFBundleName': target_name,
  'UIDeviceFamily': [ 1, 2, ],
  'UISupportedInterfaceOrientations~ipad': [
    'UIInterfaceOrientationPortrait',
    'UIInterfaceOrientationPortraitUpsideDown',
    'UIInterfaceOrientationLandscapeLeft',
    'UIInterfaceOrientationLandscapeRight',
  ],
  'UISupportedInterfaceOrientations~iphone': [
    'UIInterfaceOrientationPortrait',
    'UIInterfaceOrientationLandscapeLeft',
    'UIInterfaceOrientationLandscapeRight',
  ],
}

with open(partial_info_plist, "rb") as fp:
  partial_info = plistlib.load(fp)
  info.update(partial_info)

with open(info_plist_file, "wb") as fp:
  plistlib.dump(info, fp)
  fp.close()

if target_os == "ios":
  team_identifier = args['team_identifier']
  codesign_identity = args['codesign_identity']
  mobile_provision = os.path.abspath(args['mobile_provision'])
  shutil.copy2(mobile_provision, dest_app_path + '/' + 'embedded.mobileprovision')

  (entitlements_file, entitlements_filename) = tempfile.mkstemp()
  entitlements = dict([
    ('application-identifier', team_identifier + ".org.toucanlang.sample." + target_name),
    ('com.apple.developer.team-identifier', team_identifier),
  ])
  if args['enable_debugging']:
    entitlements['get-task-allow'] = True

  with open(entitlements_filename, "wb") as fp:
    plistlib.dump(entitlements, fp)
    fp.close()

  subprocess.check_call(["codesign", "-s", codesign_identity, "--entitlements", entitlements_filename, dest_os_path])
  os.remove(entitlements_filename)

shutil.rmtree(assets_dir_name)
os.remove(partial_info_plist)
