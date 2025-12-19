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

#include <iostream>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include <api/init_api.h>
#include <ast/ast.h>
#include <ast/semantic_pass.h>
#include <ast/type.h>
#include <codegen/codegen_llvm.h>
#include <codegen/codegen_spirv.h>
#include <parser/parser.h>

using namespace Toucan;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
double GetTimeUsec() {
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  LARGE_INTEGER v;
  v.LowPart = ft.dwLowDateTime;
  v.HighPart = ft.dwHighDateTime;
  return static_cast<double>(v.QuadPart) / 10.0;
}
#else
#include <sys/time.h>
double GetTimeUsec() {
  struct timeval t;
  gettimeofday(&t, nullptr);
  return 1000000.0 * t.tv_sec + t.tv_usec;
}
#endif

typedef void (*PFV)();

void WriteCode(const std::vector<uint32_t>& code) {
  std::cout.write(reinterpret_cast<const char*>(code.data()), code.size() * 4);
}

int main(int argc, char** argv) {
  bool dump = false;
  bool spirv = false;
  bool showTime = false;

  int                      opt;
  char                     optstring[] = "dsvtc:m:I:";
  std::string              classname = "Class";
  std::string              methodname = "method";
  std::vector<std::string> includePaths;

  while ((opt = getopt(argc, argv, optstring)) > 0) {
    switch (opt) {
      case 'd': dump = true; break;
      case 'v': spirv = true; break;
      case 't': showTime = true; break;
      case 'c': classname = optarg; break;
      case 'm': methodname = optarg; break;
      case 'I': includePaths.push_back(optarg); break;
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
  NodeVector  nodes;
  auto rootStmts = nodes.Make<Stmts>();
  InitAPI(&nodes, &types, rootStmts);
  int syntaxErrors = ParseProgram(filename, &nodes, &types, includePaths, rootStmts);
  if (syntaxErrors > 0) { exit(1); }
  types.SetMemoryLayout();
  SemanticPass semanticPass(&nodes, &types);
  rootStmts = semanticPass.Run(rootStmts);
  if (semanticPass.GetNumErrors() > 0) { exit(2); }
  types.ComputeFieldOffsets();
  double start, end;
  if (spirv) {
    Type* t = rootStmts->FindType(classname);
    if (!t) {
      fprintf(stderr, "Class \"%s\" not found.\n", classname.c_str());
      exit(3);
    } else if (!t->IsClass()) {
      fprintf(stderr, "\"%s\" is not a class type.\n", classname.c_str());
      exit(4);
    }
    ClassType* c = static_cast<ClassType*>(t);
    Method*    m = nullptr;
    for (auto& method : c->GetMethods()) {
      if (method->name == methodname) { m = method.get(); }
    }
    if (!m) {
      fprintf(stderr, "Method \"%s\" not found on class \"%s\".\n", methodname.c_str(),
              classname.c_str());
    }
    std::vector<uint32_t> output;
    CodeGenSPIRV          codeGenSPIRV(&types);
    codeGenSPIRV.Run(m);
    WriteCode(codeGenSPIRV.header());
    WriteCode(codeGenSPIRV.annotations());
    WriteCode(codeGenSPIRV.decl());
    WriteCode(codeGenSPIRV.GetBody());
    exit(0);
  }

  LLVMLinkInMCJIT();
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  llvm::LLVMContext             context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("test", context));
  llvm::FunctionCallee c = module->getOrInsertFunction("tjmain", llvm::Type::getVoidTy(context));
  llvm::Function*      main = llvm::cast<llvm::Function>(c.getCallee());
  main->setCallingConv(llvm::CallingConv::C);
  llvm::BasicBlock*                 block = llvm::BasicBlock::Create(context, "main_entry", main);
  llvm::IRBuilder<>                 builder(block);
  llvm::legacy::FunctionPassManager fpm(module.get());
  fpm.add(llvm::createLoopSimplifyPass());
  fpm.add(llvm::createPromoteMemoryToRegisterPass());
  fpm.add(llvm::createReassociatePass());
  fpm.add(llvm::createGVNPass());
  fpm.add(llvm::createCFGSimplificationPass());
  CodeGenLLVM codeGenLLVM(&context, &types, module.get(), &builder, &fpm);
  codeGenLLVM.SetDebugOutput(dump);
  std::string            errStr;
  llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::move(module))
                                      .setEngineKind(llvm::EngineKind::JIT)
                                      .setErrorStr(&errStr)
                                      .create();
  if (!engine) {
    fprintf(stderr, "Failure to create LLVM JIT engine\n");
    fprintf(stderr, "error: %s\n", errStr.c_str());
    exit(1);
  }
  // FIXME:  Lazy JIT in LLVM 2.6 clobbers the SSE regs on Win32; disable it
  // on that platform until this is fixed.
#ifdef WIN32
  engine->DisableLazyCompilation();
#endif
  codeGenLLVM.Run(rootStmts);
  auto typeList = codeGenLLVM.GetReferencedTypes().data();
  engine->addGlobalMapping(codeGenLLVM.GetTypeList(), &typeList);
  if (verifyFunction(*main)) { printf("LLVM main function is broken; aborting\n"); }
  Toucan::exitOnAbort = true;
  fpm.run(*main);
  if (dump) {
#ifdef NDEBUG
    fprintf(stderr, "no LLVM function dumping in Release builds\n");
    exit(4);
#else
//    main->dump();
#endif
  } else {
    engine->finalizeObject();
    PFV ptr = reinterpret_cast<PFV>(engine->getPointerToFunction(main));
    start = GetTimeUsec();
    (*ptr)();
    end = GetTimeUsec();
    if (showTime) printf("LLVM time is %lf usec\n", end - start);
  }
  delete engine;
  llvm::llvm_shutdown();
  exit(0);
  return 0;
}
