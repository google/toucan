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

#include <fstream>

#include <ast/semantic_pass.h>
#include <parser/parser.h>
#include "gen_bindings.h"

using namespace Toucan;

int main(int argc, char** argv) {
  int   opt;
  std::ofstream outfile;
  std::ofstream headerfile;
  char  optstring[] = "o:h:";
  while ((opt = getopt(argc, argv, optstring)) > 0) {
    switch (opt) {
      case 'o':
        outfile.open(optarg, std::ofstream::out);
        if (outfile.fail()) {
          std::perror(optarg);
          exit(3);
        }
        break;
      case 'h':
        headerfile.open(optarg, std::ofstream::out);
        if (headerfile.fail()) {
          std::perror(optarg);
          exit(4);
        }
    }
  }

  const char* filename = "(stdin)";
  if (optind < argc) {
    filename = argv[optind];
    yyin = fopen(filename, "r");
  } else {
    yyin = stdin;
  }

  TypeTable   types;
  NodeVector nodes;
  Stmts*     rootStmts = nodes.Make<Stmts>();
  int        syntaxErrors = ParseProgram(filename, &nodes, &types, {}, rootStmts);
  if (syntaxErrors > 0) { exit(1); }
  SemanticPass semanticPass(&nodes, &types);
  Stmts*       semanticStmts = semanticPass.Run(rootStmts);
  if (semanticPass.GetNumErrors() > 0) { exit(2); }

  GenBindings genBindings(outfile, headerfile, true);
  genBindings.Run(types.GetTypes());
  return 0;
}
