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

#ifndef _AST_API_VALIDATION_PASS_H_
#define _AST_API_VALIDATION_PASS_H_

#include "file_location.h"
#include "type.h"

namespace Toucan {

class APIValidator {
 public:
  APIValidator();
  void ValidateType(Type* type, const FileLocation& fileLocation);
  int  GetNumErrors() const { return numErrors_; }

 private:
  void ValidateVertexAttributeType(ClassType* buffer, Type* type);
  void ValidateVertexBufferType(ClassType* buffer, Type* type);
  void ValidateIndexBufferType(ClassType* buffer, Type* type);
  void ValidateUniformDataType(ClassType* buffer, Type* type);
  void ValidateStorageDataType(ClassType* buffer, Type* type);
  void ValidateBuffer(ClassType* classType, int qualifiers);
  void ValidateBindGroup(ClassType* classType);
  void ValidateRenderPipelineFields(ClassType* renderPipeline);
  void ValidateComputePipelineFields(ClassType* computePipeline);
  void ValidateRenderPipeline(ClassType* renderPipeline);
  void ValidateComputePipeline(ClassType* renderPipeline);
  void Error(ClassType* instance, const char* fmt, ...);

 private:
  int          numErrors_ = 0;
  FileLocation fileLocation_;
};

};  // namespace Toucan
#endif
