#include "sched.h"
#define NUM_TERMINALS 3
#define DEBUG 1
// the run queue
pcb_t* term_curr_pcb[NUM_TERMINALS] = {(pcb_t *)(0x800000 - 0x2000), 0, 0};
uint32_t qindex = 0; 
reg_t buf_reg;
void initialize_queue(){

	// for the first shell. it's always at this address
	term_curr_pcb[0] = (pcb_t *)(0x800000 - 0x2000);

	// first item in the queue.
	qindex = 0;
}

void do_handle_pit(reg_t reg){
	uint32_t old_index = qindex;
	// check if there is next pcb	
	int i = qindex + 1;	
	while(term_curr_pcb[(i) % NUM_TERMINALS] == NULL) i++;

	i = i % NUM_TERMINALS;
	
	// if we don't find any other tasks to run
	if(i == qindex) {
		send_eoi(0);
		enable_irq(0);
		return;
	}

	// copy the register context to the temporary buffer
	buf_reg = reg;

	// update the queue
	qindex = i;
	qindex = qindex % NUM_TERMINALS;
		
	// do the switch. it's guaranteed to switch
	switch_to(term_curr_pcb[old_index], term_curr_pcb[(qindex) % NUM_TERMINALS], &buf_reg);

	send_eoi(0);
	enable_irq(0);
	return;
}

#if(DEBUG == 1)
// the context switch function for the scheduler
void switch_to(pcb_t * prev, pcb_t *next, reg_t * reg){

	// we will switch the context from prev to next
	// we have to save the hardware context of prev into prev->reg, and then
	// copy next->reg into the registers
	
	// increase the index of the queue
	// save the current registers
	prev->reg = *reg;
	if(prev->reg.cs == KERNEL_CS){
		prev->reg.esp1 += 12;			//???
		prev->reg.ds = KERNEL_DS;
	}
	else {
		prev->reg.ds = USER_DS;
	}


	//
	set_prog_page(next->pt_idx);	// set new process's page


	// do the switch
	// we have to do differently for user and kernel, because the stack that iret requires
	// are different for both

	if(next->reg.cs == KERNEL_CS){
		__switch_to_kernel(next);
	}else if(next->reg.cs == USER_CS){
		__switch_to_user(next);
	}
	else {
		return;
	}
}


void __switch_to_user(pcb_t *next){

	tss.esp0 = next->tssESP;
	

	//restore hardware context and iret
	asm volatile("\
		movl %0, %%esp;\
		movl %1, %%ebx;\
		movl %2, %%ecx;\
		movl %3, %%edx;\
		movl %4, %%esi;\
		movl %5, %%edi;\
		movl %6, %%ebp; \
		movl %7, %%eax;\
		":
		:"g"(next->reg.esp),"g"(next->reg.ebx),"g"(next->reg.ecx),"g"(next->reg.edx),
		"g"(next->reg.esi),"g"(next->reg.edi), "g"(next->reg.ebp),"g"(next->reg.eax),
		"g"(USER_DS),"g"(next->reg.esp),"g"(next->reg.flags),"g"(USER_CS),
		"g"(next->reg.eip)
		:"memory", "cc");
}

void __switch_to_kernel(pcb_t *next){
	// restore ds

	tss.esp0 = next->tssESP;
	

	//restore hardware context and iret
	asm volatile("\
		movl %1, %%ebx;\
		movl %2, %%ecx;\
		movl %3, %%edx;\
		movl %4, %%esi;\
		movl %5, %%edi;\
		movl %6, %%ebp;\
		movl %7, %%eax;\
		movl %0, %%esp;\
		":
		:"g"(next->reg.esp),"g"(next->reg.ebx),"g"(next->reg.ecx),"g"(next->reg.edx),
		"g"(next->reg.esi),"g"(next->reg.edi), "g"(next->reg.ebp),"g"(next->reg.eax),
		"g"(next->reg.flags),"g"(KERNEL_CS),
		"g"(next->reg.eip)
		:"memory", "cc");
}
 

#endif



.globl ___switch_to
___switch_to:
	