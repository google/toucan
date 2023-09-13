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

#ifdef _WIN32
#include <getopt.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>

#include <ast/semantic_pass.h>
#include <ast/symbol.h>
#include <parser/parser.h>
#include "gen_bindings.h"

using namespace Toucan;

int main(int argc, char** argv) {
  int   opt;
  FILE* outfile = stdout;
  FILE* headerfile = nullptr;
  char  optstring[] = "o:h:";
  while ((opt = getopt(argc, argv, optstring)) > 0) {
    switch (opt) {
      case 'o':
        outfile = fopen(optarg, "w");
        if (!outfile) {
          perror(optarg);
          exit(3);
        }
        break;
      case 'h':
        headerfile = fopen(optarg, "w");
        if (!headerfile) {
          perror(optarg);
          exit(4);
        }
    }
  }

  if (optind < argc) {
    yyin = fopen(argv[optind], "r");
  } else {
    yyin = stdin;
  }

  SymbolTable symbols;
  TypeTable   types;
  symbols.PushNewScope();
  NodeVector nodes;
  Stmts*     rootStmts;
  int        syntaxErrors = ParseProgram(&symbols, &types, &nodes, "", &rootStmts);
  if (syntaxErrors > 0) { exit(1); }
  rootStmts->SetScope(symbols.PopScope());
  SemanticPass semanticPass(&nodes, &symbols, &types);
  Stmts*       semanticStmts = semanticPass.Resolve(rootStmts);
  if (semanticPass.GetNumErrors() > 0) { exit(2); }

  GenBindings genBindings(&symbols, &types, outfile, headerfile, true);
  genBindings.Run();
  return 0;
}
