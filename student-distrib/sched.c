#include "sched.h"
#define NUM_TERMINALS 3
#define BETA 0
#define PIT_MODE_REG 0x43
#define PIT_CMD 0x34
#define PIT_CHL_ZERO 0x40
#define PIT_MAGIC 1193182
// the run queue
pcb_t* term_curr_pcb[NUM_TERMINALS] = {0, 0, 0};
uint32_t qindex = 0; 

/*
* init_pit
*	description: it's always a good practice to initialize the devices
*	input: frequency -- the frequency at which the PIT is set
*	output: none
*	return: none
*	side effect: PIT is initialized
*/
void init_pit(int frequency){

	int16_t pit_divider;
	int8_t pit_div_lo, pit_div_hi;

	// Set the byte to be sent to PIT
	pit_divider = PIT_MAGIC / frequency;
	pit_div_lo = pit_divider & 0xFF; 
	pit_div_hi = pit_divider >> 8;

	// Do the work
	outb(PIT_CMD, PIT_MODE_REG);
	outb(pit_div_lo, PIT_CHL_ZERO);
	outb(pit_div_hi, PIT_CHL_ZERO);

}


/*
* do_handle_pit
*	description: handle the PIT, and very important, make context switch
*	input: none
*	output: none
*	return: none
*	side effect: context is switched
*/
void do_handle_pit(){
	
	// Look for a procees to switch to. Note that we do not prevent
	// switching to itself, because it has to set up correct kernel stack
	// to switch back to for next iteration.
	disable_irq(0);
	int j;	
	int i;
	uint32_t is_prog = 0;
	pcb_t * temp = get_pcb();

	for(j = 0; j < 3; j++){
		if(term_curr_pcb[j] != NULL) is_prog = 1;
		if(term_curr_pcb[j] == temp) break;
	}

	// If there is no program running, return. One thing to take note here
	// we need to send ack to the PIT to enable again.
	if(!is_prog){
		send_eoi(0);
		enable_irq(0);
		return;	// no program running yet
	}

	qindex = j;
	i = qindex + 1;

	// we are guaranteed to have at least one running process
	// that's the shell. hehe
	while(term_curr_pcb[(i) % NUM_TERMINALS] == NULL) {
		i++;
		if(i % NUM_TERMINALS == qindex){
			break;
		}
	}

	// update the queue
	qindex = i;
	qindex = qindex % NUM_TERMINALS;

	// set esp0 to the bottom of the stack
	tss.esp0 = term_curr_pcb[qindex]->tssESP;

	// prepare for the swich. We have to change the paging to point to the
	// new page table. TLB is flushed when we change the paging.

	set_prog_page(term_curr_pcb[qindex]->pt_idx);

	// change video memory map
	if(qindex != current_active_terminal) {
		set_video_page(0);
	}
	else {
		set_video_page(1);
	}

	//Critical section
	

	// do the switch. Over here, the most important pieces of information
	// is eip, esp and ebp. Those will define which program to run
	// because eventually, we will always switch to the kernel stack
	// and from there, they will go back to the user stack accordingly
	__switch_to(temp, (term_curr_pcb[qindex]));
	send_eoi(0);
	enable_irq(0);

	return;
}
