//
// Created by Gality on 2024/3/13.
//

#include "tools.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define CONST_PRIO 0
#define SHM_ENV_VAR "_SHM_ID"
#define FORKSRV_FD 198

// 对应llvm-pass中的__afl_prev_loc，存储上一个基本块的编号
u8  __area_initial[MAP_SIZE];
u8* __area_ptr = __area_initial;

// 对应llvm-pass中的__afl_prev_loc，存储上一个基本块的编号
__thread u32 __prev_loc;


static void __map_shm(void) {

  u8 *id_str = getenv(SHM_ENV_VAR);

  /* If we're running under AFL, attach to the appropriate region, replacing the
     early-stage __afl_area_initial region that is needed to allow some really
     hacky .init code to work correctly in projects such as OpenSSL. */

  if (id_str) {

    u32 shm_id = atoi(id_str);

    __area_ptr = shmat(shm_id, NULL, 0);

    /* Whooooops. */

    if (__area_ptr == (void *)-1) _exit(1);

    //写入值表示成功
    __area_ptr[0] = 1;

  }

}

// fork server的剩下部分逻辑(前半部分在afl-fuzz中的init_forkserver中)
static void __start_forkserver(void) {

  static u8 tmp[4];
  s32 child_pid;

  u8  child_stopped = 0;

  /* 写入4字节，告诉父进程：子进程正常启动 */

  if (write(FORKSRV_FD + 1, tmp, 4) != 4) return;

  // 不断从自身fork进程出来
  while (1) {

    u32 was_killed;
    int status;

    // 从管道中读取父进程的命令
    if (read(FORKSRV_FD, &was_killed, 4) != 4) _exit(1);

    if (child_stopped && was_killed) {
      child_stopped = 0;
      if (waitpid(child_pid, &status, 0) < 0) _exit(1);
    }

    // 不断fork进程用于fuzz
    child_pid = fork();
    if (child_pid < 0) _exit(1);

    // 子进程执行
    if (!child_pid) {

      // 关闭管道，继续执行(子进程无需跟fuzzer通信，只有forkserver跟fuzzer通信)
      close(FORKSRV_FD);
      close(FORKSRV_FD + 1);
      return;

    }

    // 父进程执行：写入fork出现的子进程的PID，
    if (write(FORKSRV_FD + 1, &child_pid, 4) != 4) _exit(1);

    // 等待子进程执行完毕
    if (waitpid(child_pid, &status, 0) < 0)
      _exit(1);

    // 将子进程的结束状态发送给fuzzer，继续循环
    if (write(FORKSRV_FD + 1, &status, 4) != 4) _exit(1);

  }
}

void __manual_init(void) {

  static u8 init_done;

  // 第一个进程(forkserver)才会进来
  if (!init_done) {

    __map_shm();
    __start_forkserver();
    init_done = 1;

  }

}


/* Proper initialization routine. */
// __attribute__((constructor(CONST_PRIO))) 来指示编译器在程序执行之前自动调用这个函数(runtime的构造函数)
__attribute__((constructor(CONST_PRIO))) void __auto_init(void) {

  __manual_init();

}