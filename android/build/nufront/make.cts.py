#!/usr/bin/env python
#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import sys

if sys.hexversion < 0x02040000:
  print >> sys.stderr, "Python 2.4 or newer is required."
  sys.exit(1)

import os
import re
import copy
import zipfile

def RemoveMount(data):
  output = []
  for line in data.split("\n"):
    original_line = line
    line = line.strip()
    if line and line[0] != '#':
      if line == "on fs":
        continue
      if line.startswith("mount ext"):
        continue
    output.append(original_line)
  return "\n".join(output) + "\n"

def main(argv):

  input_zip = zipfile.ZipFile(argv[0], "r")
  output_zip = zipfile.ZipFile(argv[1], "w")
    
  for info in input_zip.infolist():
    data = input_zip.read(info.filename)
    out_info = copy.copy(info)
    if info.filename.startswith("BOOT/RAMDISK/"):
      out_info.filename = info.filename.replace("BOOT/RAMDISK/", "")
      name = os.path.basename(info.filename)
      if re.match("init.*.rc", name):
        new_data = RemoveMount(data)
        output_zip.writestr(out_info, new_data)
      else:
        output_zip.writestr(out_info, data)
    if info.filename.startswith("SYSTEM/"):
      out_info.filename = info.filename.replace("SYSTEM/", "system/")
      output_zip.writestr(out_info, data)
    if info.filename.startswith("META/"):
      output_zip.writestr(out_info, data)

  if os.path.exists("build/nufront/fs_config.sh"):
    output_zip.write("build/nufront/fs_config.sh", "fs_config.sh")
    
  input_zip.close()
  output_zip.close()

  print "done."


if __name__ == '__main__':
  try:
    main(sys.argv[1:])
  except Exception, e:
    print
    print "   ERROR: %s" % (e,)
    print
    sys.exit(1)
