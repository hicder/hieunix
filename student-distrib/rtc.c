#include "rtc.h"
#include "lib.h"
#include "i8259.h"


#define PIE 0x40
#define A_REG_NMI 0x8A
#define B_REG_NMI 0x8B
#define C_REG 0x0C 
#define RTC_IRQ_LINE 8
#define TWO_HERTZ_RATE 0x0F
#define MAX_FREQUENCY 1024
#define MIN_FREQUENCY 2
#define LOG_FREQ_OFFSET 16
#define FREQ_BITMASK 0xF0

static volatile int RTC_INT_OCCURED = 1;

/*
 * init_rtc
 *   DESCRIPTION: initialize RTC and set up periodic timer interurpt.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: RTC is initialized.
 */
void init_rtc(){

	/* Choose register B and disable non-maskable interrupts */
	outb(B_REG_NMI, RTC_ADDR);

	/* Get a backup of previous config*/
	char prev = inb(RTC_DATA);
	outb(B_REG_NMI, RTC_ADDR);

	/* Set the periodic interrupt bit*/
	outb((prev | PIE), RTC_DATA);
}


/*
 * do_handle_rtc
 *   DESCRIPTION: Handle RTC interrupts.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: EOI is sent to RTC to acknowledge intr.
 */
void do_handle_rtc(){

	disable_irq(RTC_IRQ_LINE);

	//	test_interrupts();  // interrupt test

	/* Make sure we can continue to receive this interrupt. */
	outb(C_REG, RTC_ADDR);
	inb(RTC_DATA);

	/* Clear RTC flag to signal to rtc read than an interrupt has occurred */
	RTC_INT_OCCURED = 0;

	// write EOI to PIC
	send_eoi(RTC_IRQ_LINE);
	enable_irq(RTC_IRQ_LINE);
}


/*
 * rtc_open
 *   DESCRIPTION: Open RTC and set frequency to 2 Hz
 *   INPUTS: file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on sucess, -1 if invalid file descriptor
 *   SIDE EFFECTS: 
 */
int rtc_open(fd_t* file_desc){

	if(!file_desc) {
		return -1;
	}

	file_desc->fops_p = &rtc_fops; // in syscall
	file_desc->inode_p = NULL;
	file_desc->file_pos = 0;	   
	file_desc->flags = 1;

	disable_irq(RTC_IRQ_LINE);

	/* initialize rtc before setting default clock rate of 2 Hz */
	init_rtc();

	/* Set clock rate to 2 Hz */
	char rate = TWO_HERTZ_RATE;

	/* Choose register A and disable non-maskable interrupts */
	outb(A_REG_NMI, RTC_ADDR);

	/* Store current register state from data port */
	char current = inb(RTC_DATA);

	/* modify rate to 2 Hz */
	rate = current | rate; 

	/* Reset register to A */
	outb(A_REG_NMI, RTC_ADDR);

	/* Set frequency in register A */
	outb(rate, RTC_DATA);

	enable_irq(RTC_IRQ_LINE);

	return 0;

}


/*
 * rtc_read
 *   DESCRIPTION: Wait until an RTC interrupt occurs
 *   INPUTS: File Descriptor, Buffer, Nbytes, all standard for read calls
 *   OUTPUTS: none
 *   RETURN VALUE: 0 when interrupt occurs
 *   SIDE EFFECTS: could possibly wait forever if no interrupts occur
 */
int rtc_read(fd_t* file_desc, uint8_t* buf, uint32_t nbytes){

	RTC_INT_OCCURED = 1;

	while(RTC_INT_OCCURED == 1)
	{}

	return 0;

}

/*
 * rtc_write
 *   DESCRIPTION: Set RTC interrupt frequency equal to given frequency
 *   INPUTS: File descritpor, Buffer, Nbytes, all standard for write calls
 *	 Frequency passed in the buffer
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure, 0 on success
 *   SIDE EFFECTS: frequency of RTC interrupts will change
 */
int rtc_write(fd_t* file_desc, const uint8_t* buf, uint32_t nbytes){

	if(!buf) {
		return -1;
	}

	uint32_t frequency = buf[0]; // get frequency

	disable_irq(RTC_IRQ_LINE);

	int i = 0;
	char rate = 0;

	/* If given frequency is greater than 1024 Hz or less than 2Hz do not set frequency */
	if(frequency > MAX_FREQUENCY || frequency < MIN_FREQUENCY){
		return -1;
	}

	/* Check that interrupt frequency is a power of 2, if not return */
	if(frequency & (frequency - 1)){
		return -1;
	}

	/* Determine i, i = log2(frequency) */
	while(frequency != 1){
		frequency = frequency / 2;
		i++;
	}

	rate = LOG_FREQ_OFFSET - i;

	/* Choose register A and disable non-maskable interrupts */
	outb(A_REG_NMI, RTC_ADDR);

	/* Store current register state from data port */
	char current = inb(RTC_DATA);

	/* Reset register to A */
	outb(A_REG_NMI, RTC_ADDR);

	/* wipe the four low bits of current */
	current = current & FREQ_BITMASK;

	/* set the low four bits of current equal to the rate we want */
	rate = current | rate; 

	/* Set frequency in register A */
	outb(rate, RTC_DATA);

	enable_irq(RTC_IRQ_LINE);

	return 0;

}

/*
 * rtc_close
 *   DESCRIPTION: No-Operation
 *   INPUTS: File Descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on sucess, else -1 on invalid file descriptor
 *   SIDE EFFECTS: 
 */
int rtc_close(fd_t* file_desc){

	if(!file_desc) {
		return -1;
	}
	// Set the file descriptor to original
	file_desc->fops_p = NULL;
	file_desc->inode_p = NULL;
	file_desc->file_pos = 0;	   
	file_desc->flags = 0;	

	return 0;
}
