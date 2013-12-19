#ifndef __SCHED_H
#define __SCHED_H

#include "lib.h"
#include "types.h"
#include "i8259.h"
#define BETA 0

// __switch_to MACRO. over here, the effect of switching the stack
// is effectively context switch. Restoring the state will be taken
// care of by IRET

#define __switch_to(prev, next) \
do{\
	asm volatile("pushfl \n\t"\
		"pushl %%ebp \n\t"\
		"movl %%esp, %[prev_esp] \n\t"\
		"movl %[next_esp], %%esp \n\t"\
		"movl %%ebp, %[prev_ebp] \n\t"\
		"movl $1f, %[prev_eip] \n\t"\
		"movl %[next_ebp], %%ebp \n\t"\
		"jmp *%[next_eip]\n"\
		"1:\t"\
		"popl %%ebp\n\t"\
		"popfl\n" \
		:[prev_esp]"=m"(prev->esp), [prev_eip]"=m"(prev->eip), [prev_ebp]"=m"(prev->ebp)\
		:[next_esp]"m"(next->esp), [next_eip]"m"(next->eip), [next_ebp]"m"(next->ebp)\
		:"memory", "cc", "esp");\
}while(0)

// External variables to be accessed by other program
extern pcb_t * term_curr_pcb[3];
extern uint32_t qindex;

// Helper function. It's a must to call initialize_queue()
// when start up. Sometimes, the compiler doesn't do a good job
// in initializing the vales, so we have to do it by hand.
void do_handle_pit();
void initialize_queue();
void init_pit();

#endif
