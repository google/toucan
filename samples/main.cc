#include <stdio.h>

#include <api/init_types.h>
#include <ast/ast.h>
#include <ast/symbol.h>

using namespace Toucan;

extern "C" {

extern float       toucan_main();
const Type* const* _type_list;
}

int main(int argc, char** argv) {
  SymbolTable symbols;
  TypeTable   types;
  NodeVector  nodes;
  InitTypes(&symbols, &types, &nodes);
  types.Layout();
  _type_list = types.GetTypes().data();
  float result = toucan_main();
  printf("result is %f\n", result);
  return 0;
}
