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

#include <llvm-c/Target.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include <api/init_types.h>
#include <ast/ast.h>
#include <ast/semantic_pass.h>
#include <ast/symbol.h>
#include <ast/type.h>
#include <bindings/gen_bindings.h>
#include <codegen/codegen_llvm.h>
#include <codegen/codegen_spirv.h>
#include <parser/parser.h>

using namespace Toucan;

void WriteCode(const std::vector<uint32_t>& code) {
  std::cout.write(reinterpret_cast<const char*>(code.data()), code.size() * 4);
}

int main(int argc, char** argv) {
  bool dump = false;
  bool dumpSymbolTable = false;
  bool spirv = false;

  int                      opt;
  char                     optstring[] = "dsvc:m:o:t:I:";
  std::string              classname = "Class";
  std::string              methodname = "method";
  std::string              outputFilename = "a.o";
  std::string              initTypesFilename = "init_types.cc";
  std::vector<std::string> includePaths;

  while ((opt = getopt(argc, argv, optstring)) > 0) {
    switch (opt) {
      case 'd': dump = true; break;
      case 's': dumpSymbolTable = true; break;
      case 'v': spirv = true; break;
      case 'c': classname = optarg; break;
      case 'm': methodname = optarg; break;
      case 'o': outputFilename = optarg; break;
      case 't': initTypesFilename = optarg; break;
      case 'I': includePaths.push_back(optarg); break;
    }
  }

  const char* filename = "(stdin)";
  if (optind < argc) {
    filename = argv[optind];
    yyin = fopen(filename, "r");
  } else {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    exit(1);
  }

  FILE* initTypesFile = fopen(initTypesFilename.c_str(), "w");
  if (!initTypesFile) { perror(initTypesFilename.c_str()); }

  SymbolTable symbols;
  TypeTable   types;
  NodeVector  nodes;
  symbols.PushNewScope();
  InitTypes(&symbols, &types, &nodes);
  auto*             apiStmts = nodes.Make<Stmts>();
  const TypeVector& apiTypes = types.GetTypes();
  Stmts*            rootStmts;
  int syntaxErrors = ParseProgram(filename, &symbols, &types, &nodes, includePaths, &rootStmts);
  if (syntaxErrors > 0) { exit(1); }
  Scope* topScope = symbols.PopScope();
  rootStmts->SetScope(topScope);
  SemanticPass semanticPass(&nodes, &symbols, &types);
  Stmts*       stmts = semanticPass.Resolve(rootStmts);
  if (semanticPass.GetNumErrors() > 0) { exit(2); }
  types.Layout();
  if (dumpSymbolTable) {
    symbols.Dump();
  } else if (spirv) {
    symbols.PushScope(topScope);
    Type* t = symbols.FindType(classname);
    symbols.PopScope();
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
  } else {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

#if TARGET_IS_WASM
    std::string targetTriple = "wasm32-unknown-unknown";
#else
    std::string targetTriple = llvm::sys::getDefaultTargetTriple();
#endif

    std::string error;
    auto        target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (targetTriple.empty()) {
      std::cerr << error;
      return 1;
    }
    llvm::LLVMContext             context;
    std::unique_ptr<llvm::Module> module(new llvm::Module("tc", context));
    module->setTargetTriple(targetTriple);

    auto cpu = "generic";
#if TARGET_IS_WASM
    auto features = "+simd128";
#else
    auto features = "";
#endif

    llvm::TargetOptions opt;
    auto                rm = std::optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, rm);

    module->setDataLayout(targetMachine->createDataLayout());
    symbols.PushScope(topScope);
    llvm::FunctionCallee c =
        module->getOrInsertFunction("toucan_main", llvm::Type::getFloatTy(context));
    llvm::Function* main = llvm::cast<llvm::Function>(c.getCallee());
    main->setCallingConv(llvm::CallingConv::C);
    llvm::BasicBlock*                 block = llvm::BasicBlock::Create(context, "mainEntry", main);
    llvm::IRBuilder<>                 builder(block);
    llvm::legacy::FunctionPassManager fpm(module.get());
    fpm.add(llvm::createLoopSimplifyPass());
    fpm.add(llvm::createPromoteMemoryToRegisterPass());
    fpm.add(llvm::createReassociatePass());
    fpm.add(llvm::createGVNPass());
    fpm.add(llvm::createCFGSimplificationPass());
    CodeGenLLVM codeGenLLVM(&context, &types, module.get(), &builder, &fpm);
    codeGenLLVM.SetDebugOutput(dump);
    std::string errStr;
    codeGenLLVM.Run(stmts);
    if (verifyFunction(*main)) { printf("LLVM main function is broken; aborting"); }
    fpm.run(*main);
    if (dump) {
#ifdef NDEBUG
      fprintf(stderr, "no LLVM function dumping in Release builds\n");
      exit(4);
#else
//      main->dump();
#endif
    } else {
      std::error_code      ec;
      llvm::raw_fd_ostream dest(outputFilename, ec);

      if (ec) {
        std::cerr << "Could not open file: " << ec.message() << std::endl;
        return 1;
      }

      llvm::legacy::PassManager pass;
      auto                      fileType = llvm::CodeGenFileType::ObjectFile;

      if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "targetMachine can't emit a file of this type";
        return 1;
      }

      pass.run(*module);
      dest.flush();
      GenBindings bindings(&symbols, &types, initTypesFile, nullptr, false);
      bindings.Run();
    }
    llvm::llvm_shutdown();
  }
  return 0;
}
