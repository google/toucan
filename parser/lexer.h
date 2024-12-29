// Copyright 2023 The Toucan Authors
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

#ifndef _LEXER_H
#define _LEXER_H
extern std::string GetFileName();
extern int         GetLineNum();
extern void        IncLineNum();
extern void        yyerror(const char* str);
extern FILE*       IncludeFile(const char* filename);
extern void        PopFile();
namespace Toucan {
class Arg;
class ArgList;
class Expr;
class Stmt;
class UnresolvedInitializer;
};  // namespace Toucan
#endif
#define register
