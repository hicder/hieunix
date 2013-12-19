#include "queue.h"
#define NUM_TERMINALS 3

// this is actually the run queue!!!!!!
pcb_t* term_curr_pcb[NUM_TERMINALS]={(pcb_t *)(0x800000 - 0x2000),0,0};
uint32_t qindex = 0;

