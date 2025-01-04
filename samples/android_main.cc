// Copyright 2024 The Toucan Authors
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

#include <stdio.h>

#include <android_native_app_glue.h>

#include <api/init_types.h>
#include <api/api_android.h>
#include <ast/ast.h>
#include <ast/symbol.h>

using namespace Toucan;

extern "C" {

extern void        toucan_main();
const Type* const* _type_list;
}

void android_main(struct android_app* app) {
  SetAndroidApp(app);
  SymbolTable symbols;
  TypeTable   types;
  NodeVector  nodes;
  InitTypes(&symbols, &types, &nodes);
  types.ComputeFieldOffsets();
  _type_list = types.GetTypes().data();
  toucan_main();
}
