//
// Created by Gality on 2024/3/12.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "tools.h"
#include "alloc-inl.h"

static uint8_t *  obj_path;  /* Path to runtime libraries         */
static uint8_t ** cc_params;              /* Parameters passed to the real CC  */
static uint32_t cc_par_cnt = 1;         /* Param count, including argv0      */

static void find_obj(u8* argv0) {
  u8 *slash, *tmp;

  slash = strrchr(argv0, '/');

  if (slash) {

    u8 *dir;

    *slash = 0;
    dir = ck_strdup(argv0);
    *slash = '/';

    tmp = alloc_printf("%s/afl-llvm-rt.o", dir);

    if (!access(tmp, R_OK)) {
      obj_path = dir;
      ck_free(tmp);
      return;
    }

    ck_free(tmp);
    ck_free(dir);

  }

  FATAL("Unable to find 'afl-llvm-rt.o', please check");

}

static void edit_params(u32 argc, char** argv) {
  u8 fortify_set = 0, asan_set = 0, x_set = 0, bit_mode = 0;
  u8 *name;

  cc_params = ck_alloc((argc + 128) * sizeof(u8*));

  name = strrchr(argv[0], '/');
  if (!name) name = argv[0]; else name++;

  if (!strcmp(name, "myclangwrapper++")) {
    cc_params[0] = (u8*)"clang++";
  } else {
    cc_params[0] = (u8*)"clang";
  }

  // use my llvm pass to instrument
  cc_params[cc_par_cnt++] = "-Xclang";
  cc_params[cc_par_cnt++] = "-load";
  cc_params[cc_par_cnt++] = "-Xclang";
  cc_params[cc_par_cnt++] = alloc_printf("%s/libllvm_pass_for_instrumentation.dylib", obj_path);
  cc_params[cc_par_cnt++] = "-Qunused-arguments";

  while (--argc) {
    u8* cur = *(++argv);

    if (!strcmp(cur, "-m32")) bit_mode = 32;
    if (!strcmp(cur, "armv7a-linux-androideabi")) bit_mode = 32;
    if (!strcmp(cur, "-m64")) bit_mode = 64;

    if (!strcmp(cur, "-x")) x_set = 1;

    if (!strcmp(cur, "-fsanitize=address") ||
        !strcmp(cur, "-fsanitize=memory")) asan_set = 1;

    if (strstr(cur, "FORTIFY_SOURCE")) fortify_set = 1;

    if (!strcmp(cur, "-Wl,-z,defs") ||
        !strcmp(cur, "-Wl,--no-undefined")) continue;

    cc_params[cc_par_cnt++] = cur;

  }

  if (!getenv("DONT_OPTIMIZE")) {

    cc_params[cc_par_cnt++] = "-g";
    cc_params[cc_par_cnt++] = "-O3";
    cc_params[cc_par_cnt++] = "-funroll-loops";

  }

  switch (bit_mode) {

    case 0:
      cc_params[cc_par_cnt++] = alloc_printf("%s/afl-llvm-rt.o", obj_path);
      break;

    case 32:
      cc_params[cc_par_cnt++] = alloc_printf("%s/afl-llvm-rt-32.o", obj_path);

      if (access(cc_params[cc_par_cnt - 1], R_OK))
        FATAL("-m32 is not supported by your compiler");

      break;

    case 64:
      cc_params[cc_par_cnt++] = alloc_printf("%s/afl-llvm-rt-64.o", obj_path);

      if (access(cc_params[cc_par_cnt - 1], R_OK))
        FATAL("-m64 is not supported by your compiler");

      break;

  }
  cc_params[cc_par_cnt] = NULL;
}

int main(int argc, char** argv) {
  SAYF(cCYA "myclangwrapper " cBRI VERSION  cRST " by Gality\n");
  if (argc < 2) {
    SAYF("\n"
         "At least provide one parameter, A common use pattern would be one of the following:\n\n"
         "  CC=%s/myclangwrapper ./configure\n"
         "  CXX=%s/myclangwrapper++ ./configure\n\n"
         );

    exit(1);

  }
  // 获取runtime
  find_obj(argv[0]);
  // 编辑参数
  edit_params(argc, argv);
  // 调用clang/clang++
  execvp(cc_params[0], (char**)cc_params);

  FATAL("Oops, failed to execute '%s' - check your PATH", cc_params[0]);

  return 0;
}