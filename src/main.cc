// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
// Alexis hates iostream
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "llvmemitter.h"
#include "parser.h"
#include "tokenizer.h"
// ---------------------------------------------------------------------------
using Parser = qcp::Parser<qcp::emitter::LLVMEmitter>;
// ---------------------------------------------------------------------------
struct option longopts[] = {
    {"help", no_argument, nullptr, 'h'},
    {"ld", required_argument, nullptr, 0},
    {"pp", required_argument, nullptr, 0},
    {"no-pp", no_argument, nullptr, 'p'},
    {"--emit-bc", no_argument, nullptr, 'b'},
    {"--emit-llvm", no_argument, nullptr, 'l'},
    {0, 0, nullptr, 0}};
// ---------------------------------------------------------------------------
const char *helpmsg = R"(
Usage: qcp [options] file0 ...fileN
  -h, --help          Display this help message
  --ld                Path to the linker (default: ld)
  --pp                preprocessor command (default: cc -nostdinc -E)
  -I                  Add a directory to the include path
  -D                  Define a macro
  -U                  Undefine a macro
  -L, -l              Passed to the linker
   -o                  Output file (default: a.out)
   -E                  Stop after the preprocessing stage
   -c                  Compile only (do not link)
   -p, --no-pp         Do not run the preprocessor
   -b, --emit-bc       Emit LLVM bitcode
   -l, --emit-llvm     Emit LLVM IR
)";
// ---------------------------------------------------------------------------
struct ParserConfig {
   std::string pp;
   std::string ppargs;
   std::string OF;
   std::string ld;
   std::string ldargs;
   unsigned char stopAfterPP : 1,
       compileOnly : 1,
       noPP : 1,
       emitBC : 1,
       emitLLVM : 1;
};
// ---------------------------------------------------------------------------
// } // namespace
// ---------------------------------------------------------------------------
std::FILE *processFile(const std::string &filename, ParserConfig &cfg) {
   std::FILE *tmpf;
   int tmpfd;

   std::FILE *OF = nullptr;
   int OFd;

   void *data = nullptr;
   std::size_t mmapSize;

   if (!cfg.OF.empty()) {
      OF = std::fopen(cfg.OF.c_str(), "w");
      if (!OF) {
         std::cerr << "Failed to open output file '" << cfg.OF << "'\n";
         goto cleanup;
      }
      OFd = fileno(OF);
   }

   if (cfg.stopAfterPP) {
      assert(OF && "OF must be open");
      tmpf = OF;
   } else if (cfg.noPP) {
      tmpf = std::fopen(filename.c_str(), "r");
   } else {
      tmpf = tmpfile();
   }
   if (!tmpf) {
      std::cerr << "Failed to open file\n";
      goto cleanup;
   }
   tmpfd = fileno(tmpf);

   if (!cfg.noPP) {
      std::string cmd = cfg.pp + " " + cfg.ppargs + " " + filename + " > /dev/fd/" + std::to_string(tmpfd);
      int ret = std::system(cmd.c_str());

      if (ret != 0) {
         std::cerr << "Preprocessor failed\n";
         goto cleanup;
      }
   }

   if (cfg.stopAfterPP) {
      goto cleanup;
   }

   {
      struct stat statbuf;
      fstat(tmpfd, &statbuf);
      mmapSize = static_cast<std::size_t>(statbuf.st_size);

      data = mmap(nullptr, mmapSize, PROT_READ, MAP_PRIVATE, tmpfd, 0);

      std::string_view sv{static_cast<const char *>(data), mmapSize};

      qcp::DiagnosticTracker diag{filename, sv};
      Parser parser{sv, diag};
      parser.addIntTypeDef("__builtin_va_list");
      parser.parse();

      std::cerr << diag;

      ftruncate(tmpfd, 0);
      
      auto& emitter = parser.getEmitter();
      int fd = OF ? OFd : tmpfd;
      if (cfg.emitLLVM) {
         emitter.writeLLVMToFile(fd);
      } else if (cfg.emitBC) {
         emitter.writeToBitcodeFile(fd);
      } else {
         emitter.writeToObjFile(fd);
      }

      if (cfg.compileOnly) {
         goto cleanup;
      }
   }

cleanup:
   if (data) {
      if (munmap(data, mmapSize)) {
         std::cerr << "munmap failed\n";
      }
   }

   if (tmpf && OF) {
      std::fclose(tmpf);
   }

   if (OF) {
      std::fclose(OF);
   }

   return OF ? nullptr : tmpf;
}
// ---------------------------------------------------------------------------
#define NOT_BOTH_OPTS(repx, x, repy, y)\
   if ((x) && (y)) {    \
      std::cerr << "Cannot specify '-" repx "' with '-" repy "'\n"; \
      return 1; \
   }
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
   int EC = 0;
   int c;
   const char *OFName = nullptr;
   ParserConfig cfg = {
       .pp = "cc -nostdinc -E",
       .ppargs = "",
       .OF = "",
       .ld = "ld",
       .ldargs = "",
       .stopAfterPP = false,
       .compileOnly = false,
       .noPP = false,
       .emitBC = false,
       .emitLLVM = false};

   // process options that affect all files
   opterr = 0;
   while (1) {
      int option_index = 0;

      c = getopt_long(argc, argv, "I:U:D:o:Echp", longopts, &option_index);
      if (c == -1) {
         break;
      }

      switch (c) {
         case 'h':
            std::cout << helpmsg;
            if (qcp::emitter::LLVMEmitter::HELP_MESSAGE) {
               std::cout << qcp::emitter::LLVMEmitter::HELP_MESSAGE;
            }
            return 0;

         case 'D':
         case 'U':
         case 'I':
            cfg.ppargs += " -" + std::string(1, c) + " " + optarg;
            break;

         case 'E':
            NOT_BOTH_OPTS("E", cfg.stopAfterPP, "c", cfg.compileOnly);
            cfg.stopAfterPP = 1;
            break;

         case 'p':
            if (cfg.noPP) {
               std::cerr << "already specified '-p'\n";
            }
            cfg.noPP = 1;
            break;

         case 'b':
            NOT_BOTH_OPTS("b", cfg.emitBC, "l", cfg.emitLLVM);
            NOT_BOTH_OPTS("b", cfg.emitBC, "E", cfg.stopAfterPP);
            if (cfg.emitBC) {
               std::cerr << "Already specified '-b'\n";
            }
            cfg.emitBC = 1;
            break;

         case 'l':
            NOT_BOTH_OPTS("l", cfg.emitLLVM, "b", cfg.emitBC);
            NOT_BOTH_OPTS("l", cfg.emitLLVM, "E", cfg.stopAfterPP);
            if (cfg.emitLLVM) {
               std::cerr << "Already specified '-l'\n";
            }
            cfg.emitLLVM = 1;
            break;

         case 'c':
            NOT_BOTH_OPTS("c", cfg.compileOnly, "E", cfg.stopAfterPP);
            if (cfg.compileOnly) {
               std::cerr << "Already specified '-c'\n";
            }
            cfg.compileOnly = 1;
            break;

         case 0:
            ((option_index == 1) ? cfg.ld : cfg.pp) = optarg;
            break;

         case 'o':
            OFName = optarg;
            break;

         default:
            break;
      }
   }

   if (optind == argc) {
      std::cerr << "No input files\n";
      return 1;
   }

   std::vector<std::FILE *> OFs{};

   // cannot reuse argc and argv because the '-' in the optstring requires optind to be set to zero
   char **remArgv = argv + optind - 1;
   int remArgc = argc - optind + 1;

   // reset getopt state
   optind = 0;
   opterr = 1;
   while ((c = getopt(remArgc, remArgv, "-L:l:")) != -1) {
      switch (c) {
         case 'L':
         case 'l':
            cfg.ldargs += " -" + std::string(1, c) + " " + optarg;
            break;

         case 1:
            if (!OFs.empty() && OFName && (cfg.compileOnly || cfg.stopAfterPP)) {
               std::cerr << "Cannot specify '-o' with -c or -E and multiple files\n";
               return 1;
            } else if (!OFName && (cfg.stopAfterPP || cfg.compileOnly)) {
               const char *ext =
                   cfg.emitBC      ? ".bc" :
                   cfg.emitBC      ? ".ll" :
                   cfg.stopAfterPP ? ".i" :
                                     ".o";
               cfg.OF = std::filesystem::path(optarg).replace_extension(ext).string();
            }
            if (auto *of = processFile(optarg, cfg)) {
               OFs.push_back(of);
            }
            break;

         case '?':
            // todo:
            break;

         default:
            std::cerr << "?? getopt returned character code 0%o ??\n"
                      << c;
            EC = 1;
            goto cleanup;
      }
   }

   if (optind != remArgc) {
      std::cerr << "Extra arguments '";
      for (int i = optind; i < remArgc; ++i) {
         std::cerr << argv[i] << ' ';
      }
      std::cerr << "'\n";
      EC = 1;
      goto cleanup;
   }

   if (!cfg.compileOnly && !cfg.stopAfterPP) {
      if (!OFName) {
         OFName = "a.out";
      }
      std::string cmd = cfg.ld + " " + cfg.ldargs + " -o " + OFName;
      for (auto *of : OFs) {
         cmd += " /dev/fd/" + std::to_string(fileno(of));
      }
      int EC = std::system(cmd.c_str());
      if (EC != 0) {
         std::cerr << "Linker failed\n";
      }
   }
cleanup:
   for (auto *of : OFs) {
      std::fclose(of);
   }
   return EC;
}
// ---------------------------------------------------------------------------
