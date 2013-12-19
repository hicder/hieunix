/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask = 0xff; /* IRQs 0-7 */
uint8_t slave_mask = 0xff; /* IRQs 8-15 */


/*
 * i8259_init
 *   DESCRIPTION: initialize the PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: both slave and master PIC is initialized, and mapped to the IDT.
 */
void
i8259_init(void)
{

	outb(0xff, MASTER_8259_IMR); 			/* mask all of 8259A-1 */
	outb(0xff, SLAVE_8259_IMR);				/* mask all of 8259A-1 */

	//Initialize master
	outb(ICW1, MASTER_8259_PORT);			/* ICW1: select 8259A-1 init */
	outb(ICW2_MASTER, MASTER_8259_IMR);		/* ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27 */
	outb(ICW3_MASTER, MASTER_8259_IMR);		/* 8259A-1 (the master) has a slave on IR2 */
	outb(ICW4, MASTER_8259_IMR);			/* master expects normal EOI */

	//initialize slave 
	outb(ICW1, SLAVE_8259_PORT);			/* ICW1: select 8259A-2 init */
	outb(ICW2_SLAVE, SLAVE_8259_IMR);		/* ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2f */
	outb(ICW3_SLAVE, SLAVE_8259_IMR);		/* 8259A-2 is a slave on master's IR2 */
	outb(ICW4, SLAVE_8259_IMR);				/* (slave's support for AEOI in flat mode is to be investigated) */

	long i =0;  
	/* wait for 8259A to initialize */
	while(i<1000000) {
		i++;
	}

	outb(master_mask, MASTER_8259_IMR); 			/* restore master IRQ interrupt */
	outb(slave_mask, SLAVE_8259_IMR);   			/* restore slave IRQ interrupt */

}

/*
 * enable_irq
 *   DESCRIPTION: Enable (unmask) the specified IRQ
 *   INPUTS: irq_num -- the IRQ line to be enabled.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: master_mask and slave_mask is updated.
 */
void
enable_irq(uint32_t irq_num)
{
	if(irq_num < 8){  // Master handles IRQ (0-7)
		master_mask &= ~(1 << irq_num); 		// enable IRQ on the requested line
		outb(master_mask, MASTER_8259_IMR); 			// send the master PIC the new mask
	}
	else if (irq_num >= 8 && irq_num < 16){  // Slave handles IRQ (8-15)
		slave_mask &= ~(1 << (irq_num - 8));  	// enable IRQ on the requested line
		outb(slave_mask, SLAVE_8259_IMR);  //  send the slave PIC the new mask
	}

}

/*
 * disable_irq
 *   DESCRIPTION: Disable (unmask) the specified IRQ
 *   INPUTS: irq_num -- the IRQ line to be disabled.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: master_mask and slave_mask is updated.
 */
void
disable_irq(uint32_t irq_num)
{
	if(irq_num < 8){ // master handles IRQ (0-7)
		master_mask |= (1 << irq_num); 		// disable IRQ on the requested line
		outb(master_mask, MASTER_8259_IMR); // send the master PIC the new mask
	}
	else if (irq_num >= 8 && irq_num < 16){ // slave handles IRQ (8-15)
		slave_mask |= (1 << (irq_num - 8)); // disable IRQ on the requested line
		outb(slave_mask, SLAVE_8259_IMR); 	// send slave PIC the new mask
	}

}

/*
 * send_eoi
 *   DESCRIPTION: send End-Of-Interrupt signal to the specified IRQ
 *   INPUTS: irq_num -- the IRQ line to be sent to.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: master_mask and slave_mask is updated.
 */
void
send_eoi(uint32_t irq_num)
{
	uint8_t eoi; // vector table
	if(irq_num < 0 || irq_num > 15) return; // if IRQ is not between 0 - 15, return

	if(irq_num < 8){  // master handles IRQ (0-7)
		// mask irq before sending eoi 
		master_mask |= (1 << irq_num); 
		outb(master_mask,MASTER_8259_IMR);	

		// send EOI to master
		eoi = EOI | irq_num;
		outb(eoi, MASTER_8259_PORT);
	}
	else if (irq_num >= 8 && irq_num < 16){// slave handles IRQ (8-15)
		// mask irq before sending eoi
		slave_mask |= (1 << (irq_num-8) );
		outb(slave_mask,SLAVE_8259_IMR);	

		// send EOI to slave 
		eoi = EOI | (irq_num - 8);
		outb(eoi, SLAVE_8259_PORT);

		
		// send EOI to master-IRQ2
		eoi = EOI | SLAVE_IRQ;
		outb(eoi, MASTER_8259_PORT);
	}
}

