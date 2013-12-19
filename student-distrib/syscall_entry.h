#ifndef __SYSCALL_ENTRY_H
#define __SYSCALL_ENTRY_H
#define ENOSYS -1
#ifndef ASM
#include "syscalls.h"
extern void handle_syscall();
#endif
#endif
