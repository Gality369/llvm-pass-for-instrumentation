//
// Created by Gality on 2024/3/12.
//

#ifndef LLVM_PASS_FOR_INSTRUMENTATION_TOOLS_H
#define LLVM_PASS_FOR_INSTRUMENTATION_TOOLS_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;

#define VERSION   "test-0.0.1"

#define cLGN "\x1b[1;92m"
#define cYEL "\x1b[1;93m"
#define cBRI "\x1b[1;97m"
#define cLRD "\x1b[1;91m"
#define cCYA "\x1b[0;36m"
#define cRST "\x1b[0m"

#define bSTOP    "\x0f"
#define RESET_G1 "\x1b)B"

#define CURSOR_SHOW   "\x1b[?25h"

#define MAP_SIZE_POW2       16
#define MAP_SIZE            (1 << MAP_SIZE_POW2)

#define SAYF(x...)    fprintf(stderr, x)

#define Rand(x) (random() % (x))

#define WARNF(x...) do { \
    SAYF(cYEL "[!] " cBRI "WARNING: " cRST x); \
    SAYF(cRST "\n"); \
  } while (0)

#define OKF(x...) do { \
    SAYF(cLGN "[+] " cRST x); \
    SAYF(cRST "\n"); \
  } while (0)

#define FATAL(x...) do { \
    SAYF(bSTOP RESET_G1 CURSOR_SHOW cRST cLRD "\n[-] PROGRAM ABORT : " \
         cBRI x); \
    SAYF(cLRD "\n         Location : " cRST "%s(), %s:%u\n\n", \
         __FUNCTION__, __FILE__, __LINE__); \
    exit(1); \
  } while (0)

#define alloc_printf(_str...) ({ \
    u8* _tmp; \
    s32 _len = snprintf(NULL, 0, _str); \
    if (_len < 0) FATAL("Whoa, snprintf() fails?!"); \
    _tmp = ck_alloc(_len + 1); \
    snprintf((char*)_tmp, _len + 1, _str); \
    _tmp; \
  })


/* Die by calling abort() to provide a core dump. */

#define ABORT(x...) do { \
    SAYF(bSTOP RESET_G1 CURSOR_SHOW cRST cLRD "\n[-] PROGRAM ABORT : " \
         cBRI x); \
    SAYF(cLRD "\n    Stop location : " cRST "%s(), %s:%u\n\n", \
         __FUNCTION__, __FILE__, __LINE__); \
    abort(); \
  } while (0)


#endif //LLVM_PASS_FOR_INSTRUMENTATION_TOOLS_H
