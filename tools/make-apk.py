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

import argparse
import os
import shutil
import subprocess
import tempfile

argparser = argparse.ArgumentParser(description="Create an APK")
argparser.add_argument('--target-name')
argparser.add_argument('--target-cpu')
argparser.add_argument('--sdk-dir')
argparser.add_argument('--keystore')
argparser.add_argument('--source-lib')
argparser.add_argument('--out')
args = vars(argparser.parse_args())

target_name = args['target_name']
target_cpu = args['target_cpu']
android_sdk_dir = args['sdk_dir']
source_lib = args['source_lib']
keystore = args['keystore']
outfile = args['out']

package_name = target_name.replace("-", "_")

manifest = '''<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
          xmlns:tools="http://schemas.android.com/tools"
          package="org.toucanlang.''' + package_name + '''">
  <uses-sdk android:minSdkVersion="26"/>
  <application android:allowBackup="false"
               android:label="''' + target_name + '''"
               android:hasCode="false">
    <activity android:name="android.app.NativeActivity"
              android:label="''' + target_name + '''"
              android:exported="true">
      <meta-data android:name="android.app.lib_name"
                 android:value="''' + target_name + '''"/>
      <intent-filter>
        <action android:name="android.intent.action.MAIN"/>
        <category android:name="android.intent.category.LAUNCHER"/>
      </intent-filter>
    </activity>
  </application>
</manifest>
'''

lib_dirs = {
  'arm' : 'armeabi-v7a',
  'arm64' : 'arm64-v8a',
  'x86' : 'x86',
  'x64' : 'x86_64'
}

apk_dir = tempfile.mkdtemp(prefix="toucan-apk-")
apk_path = apk_dir + "/lib/" + lib_dirs[target_cpu] + "/"
os.makedirs(apk_path)
basename = os.path.basename(source_lib)
dest_lib = apk_path + os.path.basename(source_lib)
os.symlink(source_lib, dest_lib)

manifest_dir = tempfile.mkdtemp(prefix="toucan-manifest-")
manifest_file = manifest_dir + "/AndroidManifest.xml"
with open(manifest_file, "w") as f:
  f.write(manifest)
  f.close()

subprocess.check_call([android_sdk_dir + '/build-tools/latest/aapt', 'package', '-f' , '-M', manifest_file, '-I', android_sdk_dir + '/platforms/android-26/android.jar', '-F', outfile, apk_dir])
subprocess.check_call([android_sdk_dir + '/build-tools/34.0.0/apksigner', 'sign', '--ks', keystore, '--ks-pass', 'pass:toucan', outfile])
shutil.rmtree(apk_dir)
shutil.rmtree(manifest_dir)
