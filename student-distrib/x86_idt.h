#ifndef X86_H
#define X86_H
#include "lib.h"
#include "rtc.h"
#include "terminal.h"
#include "mouse.h"
#define KEYBOARD_IRQ 1
#define RTC_IRQ 8
#define INTR_OFFSET 0x20
#define NR_SYSCALLS 10

 
#ifndef ASM
/* Different exception handlers for each of the exceptions.
* Each is written in assembly, and will not return access
* for the first check point. Eventually, these funciton will have to
* kill the process and return access.
*/
extern void divide_error();
extern void debug();
extern void nmi();
extern void int3();
extern void overflow();
extern void bounds();
extern void invalid_op();
extern void device_not_available();
extern void coprocessor_segment_overrun();
extern void invalid_TSS();
extern void segment_not_present();
extern void stack_segment();
extern void general_protection();
extern void page_fault();
extern void coprocessor_error();
extern void simd_coprocessor_error();
extern void alignment_check();
extern void spurious_interrupt_bug();
extern void machine_check();
//extern void handle_syscall();
extern void handle_rtc();
extern void handle_keyboard();
extern void handle_pit();
extern void handle_mouse();
#endif
#endif

