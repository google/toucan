// Copyright 2025 The Toucan Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _AST_FILE_LOCATION_H_
#define _AST_FILE_LOCATION_H_

#include <memory>
#include <string>

namespace Toucan {

struct FileLocation {
  FileLocation();
  FileLocation(const FileLocation& other);
  FileLocation(std::shared_ptr<std::string> f, int n);
  std::shared_ptr<std::string> filename;
  int                          lineNum = -1;
};

class ScopedFileLocation {
 public:
  ScopedFileLocation(FileLocation* p, const FileLocation& newLocation)
      : location_(p), previous_(*p) {
    *location_ = newLocation;
  }
  ~ScopedFileLocation() { *location_ = previous_; }

 private:
  FileLocation* location_;
  FileLocation  previous_;
};

}  // namespace Toucan

#endif  //  _AST_FILE_LOCATION_H_
