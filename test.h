#ifndef __LINUX_NEW_SYSCALL_H
#define __LINUX_NEW_SYSCALL_H
#include "unistd_64.h"
#include <asm/unistd.h>
#define new_syscall(x) syscall(__NR_new_syscall, x)
#endif