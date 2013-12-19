#define ASM
#include "x86_desc.h"
#include "syscall_entry.h"

# save registers macro for system calls
#define SAVE_ALL_SYS \
	cld;\
	push %es;\
	push %ds;\
	pushl %eax;\
	pushl %ebp;\
	pushl %edi;\
	pushl %esi;\
	pushl %edx;\
	pushl %ecx;\
	pushl %ebx;\

# restore registers macro for system calls
#define RESTORE_ALL_SYS \
	popl %ebx;\
	popl %ecx;\
	popl %edx;\
	popl %esi;\
	popl %edi;\
	popl %ebp;\
	popl %eax;\
	pop %ds;\
	pop %es;

.text
.align 4

# handle_syscall
# description: handles system calls
# input: input will place on eax, ebx
# output: none
# return: error code on invalid system call
# side effect: modify eax, ebx

.globl handle_syscall
handle_syscall:
	pushl %eax							
	SAVE_ALL_SYS						## save all registers (except eax)
	cmpl $1, %eax
	jb syscall_invalid
	cmpl $11, %eax 						## check for bad system call
	jb syscall_is_valid 				## if valid goto jump table
syscall_invalid:	
	movl $(ENOSYS), 24(%esp)		    ## load error code for bad system call
	jmp resume_userspace			    ## if invalid return to user with error code

## if valid goto jump table and appropriate system call
syscall_is_valid:
	pushl %edx
	pushl %ecx
	pushl %ebx
	jmp *sys_call_table-4(,%eax,4)
ret_from_syscalls:
	addl $12, %esp

## return to space either from executed system call or invalid call number
resume_userspace:
	movl %eax,24(%esp)
	RESTORE_ALL_SYS
	add $4, %esp
	iret

.extern halt
.extern execute
.extern read
.extern write
.extern open
.extern close
.extern getargs
.extern vidmap
.extern set_handler
.extern sigreturn

## jump table for all system calls
sys_call_table: .long __halt, __execute, __read, __write, __open, __close, __getargs, __vidmap, __set_handler, __sigreturn

## halt system call
__halt:
	call halt 		## goto C code
	jmp ret_from_syscalls

__execute:
	call execute 	## goto C code
	jmp ret_from_syscalls

__read:
## call function
	call read
## done, return
	jmp ret_from_syscalls

__write:
	call write
	jmp ret_from_syscalls

__open:
	call open
## done, return
	jmp ret_from_syscalls

__close:
	call close
## done, return
	jmp ret_from_syscalls

__getargs:
	call getargs
## done, return
	jmp ret_from_syscalls
	
__vidmap:
	call vidmap
## done, return
	jmp ret_from_syscalls

__set_handler:
	call set_handler
## done, return
	jmp ret_from_syscalls

__sigreturn:
	call sigreturn
## done, return
	jmp ret_from_syscalls





