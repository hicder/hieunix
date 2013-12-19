void switch_to(pcb_t * prev, pcb_t *next, reg_t reg){

	// we will switch the context from prev to next
	// we have to save the hardware context of prev into prev->reg, and then
	// copy next->reg into the registers

	// save the current registers
	prev->reg = reg;

	

	// do the switch
	__switch_to_kernel(pcb_t * next);
}


void __switch_to_kernel(pcb_t *next){
	// restore ds
	asm volatile("\
		movl %0, %%eax;\
		movw %%ax, %%ds;\
	":/* no output*/
	:"g"(next->reg.ds)
	:"eax");

	//restore hardware context
	asm volatile("\
		movl %0, %%eax;\
		movl %1, %%ebx;\
		movl %2, %%ecx;\
		movl %3, %%edx;\
	": /*no output*/
	:"g"(next->reg.eax), "g"(next->reg.ebx), "g"(next->reg.ecx), "g"(next->reg.edx)
	:"eax", "ebx", "ecx", "edx");
	

	// update tss
	tss.esp0 = next->tssESP;

	//magic iret
	// for IRET, the registers we have to push into the stack is: (from bottom to top)
	// ds, esp, flags, cs, eip


	// since we're implementing switching between kernel processes, it's simpler

	
	asm volatile(" 			\
		cli;				\
		movw %0, %%ax; 		\
		movw %%ax, %%ds; 	\
		pushfl;				\
		popl %%eax;			\
		orl $0x200, %%eax;	\
		pushl %%eax;		\
		pushl %2;			\
		pushl %3;			\
		iretl;				\
	": /* no output*/
	: "g"(KERNEL_DS), "g"(USER_STACK  - 4), "g"(KERNEL_CS), "g"(next->reg.eip)
	: "eax", "memory");

}
