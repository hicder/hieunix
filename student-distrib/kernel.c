/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "x86_idt.h"
#include "rtc.h"
#include "page.h"
#include "terminal.h"
#include "fs.h"
#include "syscall_entry.h" 
#include "syscalls.h"
#include "sched.h"
/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))
#define TRAP_GATE 0x8F00
#define INTR_GATE 0x8E00
#define SYSTEM_GATE 0XEF00
#define SYSTEM_INTR_GATE 0XEE00
#define TASK_GATE 0x8500
#define MSB_16 0xFFFF0000
#define LSB_16 0xFFFF
#define FOR_FUN 0
#define PIT_IRQ 0
#define KB_IRQ 1
#define MOUSE_IRQ 12
#define RTC_IRQ 8
// Keep track of which pcb has been used.
int pcb_used[6];

/*
 * set_trap_gate
 *   DESCRIPTION: this function will set up a new trap gate and write it to the table
 *   INPUTS: gate - the index in the table for the current descriptor.
 *   	     addr - a pointer to the function to pass to SET_IDT_ENTRY to set the offsets 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: IDT table is updated.
 */
void set_trap_gate(int gate, void * addr){
	long hiword = TRAP_GATE | ((long)addr & MSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate) + 4), &hiword, 4);
	long lowword = (KERNEL_CS << 16) | ((long)addr & LSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate)), &lowword, 4);
}

/*
 * set_intr_gate
 *   DESCRIPTION: this function will set up a new interrupt gate and write it to the table
 *   INPUTS: gate - the index in the table for the current descriptor.
 *   	     addr - a pointer to the function to pass to SET_IDT_ENTRY to set the offsets 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: IDT table is updated.
 */
void set_intr_gate(int gate, void *addr){
	long hiword = INTR_GATE | ((long)addr & MSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate) + 4), &hiword, 4);
	//idt[gate].seg_selector = KERNEL_CS;
	long lowword = (KERNEL_CS << 16) | ((long)addr & LSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate)), &lowword, 4);
}
/*
 * set_system_intr_gate
 *   DESCRIPTION: this function will set up a new system interrupt gate and write it to the table
 *   INPUTS: gate - the index in the table for the current descriptor.
 *   	     addr - a pointer to the function to pass to SET_IDT_ENTRY to set the offsets 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: IDT table is updated.
 */
void set_system_intr_gate(int gate, void *addr){
	long hiword = SYSTEM_INTR_GATE | ((long)addr & MSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate) + 4), &hiword, 4);
	//idt[gate].seg_selector = KERNEL_CS;
	long lowword = (KERNEL_CS << 16) | ((long)addr & LSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate)), &lowword, 4);
}

/*
 * set_system_gate
 *   DESCRIPTION: this function will set up a new system gate and write it to the table
 *   INPUTS: gate - the index in the table for the current descriptor.
 *   	     addr - a pointer to the function to pass to SET_IDT_ENTRY to set the offsets 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: IDT table is updated.
 */
void set_system_gate(int gate, void *addr){
	long hiword = SYSTEM_GATE | ((long)addr & MSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate) + 4), &hiword, 4);
	//idt[gate].seg_selector = KERNEL_CS;
	long lowword = (KERNEL_CS << 16) | ((long)addr & LSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate)), &lowword, 4);
}

/*
 * set_task_gate
 *   DESCRIPTION: this function will set up a new task gate and write it to the table
 *   INPUTS: gate - the index in the table for the current descriptor.
 *   	     addr - a pointer to the function to pass to SET_IDT_ENTRY to set the offsets  
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: IDT table is updated.
 */
void set_task_gate(int gate, void *addr){
	long hiword = TASK_GATE | ((long)addr & MSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate) + 4), &hiword, 4);
	//idt[gate].seg_selector = KERNEL_CS;
	long lowword = (KERNEL_CS << 16) | ((long)addr & LSB_16);
	memcpy((void *)((unsigned long)(&idt) + (8 * gate)), &lowword, 4);
}

/*
 * init_idt
 *   DESCRIPTION: initialize the IDT
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: update the IDT
 */
void idt_init(){
	/* Set exception entries*/
	set_trap_gate(0, &divide_error);
	set_trap_gate(1, &debug);
	set_intr_gate(2, &nmi);
	set_system_intr_gate(3, &int3);
	set_system_gate(4, &overflow);
	set_system_gate(5, &bounds);
	set_trap_gate(6, &invalid_op);
	set_trap_gate(7, &device_not_available);
	set_task_gate(8, &coprocessor_segment_overrun);
	set_trap_gate(10, &invalid_TSS);
	set_trap_gate(11, &segment_not_present);
	set_trap_gate(12, &stack_segment);
	set_trap_gate(13, &general_protection);
	set_trap_gate(14, &page_fault);
	set_trap_gate(16, &coprocessor_error);
	set_trap_gate(19, &simd_coprocessor_error);
	set_trap_gate(17, &alignment_check);
	set_trap_gate(18, &machine_check);
	
	/* Set up system call entry*/
	set_system_gate(128, &handle_syscall);


	/* Set interrupt entries*/
	set_intr_gate(32, &handle_pit);
	set_intr_gate(33, &handle_keyboard);
	set_intr_gate(40, &handle_rtc);

	set_intr_gate(44, &handle_mouse);
	

}

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;
	//int a;
	/* Clear the screen. */
	clear();
	int i;
	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/*
		Initialize IDT table with exceptions and a few interrupts
	*/

	
	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	/* Set up the IDT table.
	* The table address is already loaded in the LDTR.
	*/
	idt_init();

	printf("IDT is loaded correctly\n");

	/* Init the PIC */
	i8259_init();

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	init_rtc();
	init_pit(100);
	rtc_open(NULL);
	keyboard_init();
	mouse_init();
	/* Enable IRQ lines for RTC. We have to enable both master and slave*/
	/* Keyboard interrupt*/
	

	/* Enable paging*/
	printf("Enabling Paging\n");
	paging_init();

	// intialize keyboard, does nothing as of now, included for style
	set_up_fops();
	
	/* Enable read-only file system*/
	module_t* mod = (module_t*)mbi->mods_addr;
	fs_init ((uint32_t *)mod->mod_start);


	/*Initialize the PCB list*/
	for(i = 0; i < MAX_NUM_PROG; i++){
		pcb_used[i] = 0;
	}

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	printf("Enabling Interrupts\n");

	enable_irq(KB_IRQ);
	// slave IRQ line on master
	enable_irq(SLAVE_IRQ);
	// rtc on slave
	enable_irq(RTC_IRQ);
	enable_irq(MOUSE_IRQ);
	enable_irq(PIT_IRQ);

	//Clear the screen for our first program
	clear();
	sti();
#if(FOR_FUN == 1)
	printf("  _    _ _                  _      \n");
	printf(" | |  | (_)                (_)     \n");
	printf(" | |__| |_  ___ _   _ _ __  ___  __\n");
	printf(" |  __  | |/ _ \\ | | | '_ \\| \\ \\/ /\n");
	printf(" | |  | | |  __/ |_| | | | | |>  < \n");
	printf(" |_|  |_|_|\\___|\\__,_|_| |_|_/_/\\_\\\n");
#endif
	// Execute shell, start our OS!!!!!
	execute((uint8_t *)"shell");

	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}

