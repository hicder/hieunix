#include "page.h"
#define VIDEO_VIRTUAL 0x8400000
#define VIDEO 0xB8000
#define VIDEO_BACKUP 0xBC000
/*global page directory and kernel page_table*/
uint32_t page_directory[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
uint32_t page_table[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
uint32_t pt_vid1[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));

/*table for used pages to be used by program*/
uint32_t prog_used_page[MAX_NUM_PROG];


/*
* paging_init
*	description: initialize paging and set up page directory, pagetables
*	input: none
*	output: none
*	return: none
*	side effect: enter protected mode, and turn on paging.
*/

void paging_init()
{

	uint32_t i = 0;
	uint32_t address = PAGE_SIZE;	// starting at 4kb
	uint32_t cr0;		// stores value in cr0
	uint32_t cr4;		// stores values in cr4

	/* Set up the first 4kB to be not accessible*/
	page_table[0] =  USER_SUPER | READ_WRITE; 

	/* Set up pages for the first page table */ 
	for(i =1; i<TABLE_SIZE;i++) {
		page_table[i] =  address | USER_SUPER | READ_WRITE | PRESENT;   // user , read/write, present
		// skip 4 kb
		address += PAGE_SIZE;
	}

	for(i = 0; i < MAX_NUM_PROG; i++){
		prog_used_page[i] = 0;
	}

	/* 0-4MB page table in the page directory*/
	page_directory[0] = (uint32_t) page_table;
	page_directory[0] |= /*USER_SUPER | */READ_WRITE | PRESENT;	

	// 4Mb page for kernel at 1
	page_directory[1] =  KERNEL_ADR | _4MB_PAGE /*| USER_SUPER*/ | PRESENT;	// read only kernel page


	// write pointer to page directory into PDB Register
	asm volatile("mov %0, %%cr3":: "b"(page_directory));

	// reads cr4, setting PSE bit in cr4, and writes it back 
	asm volatile("mov %%cr4, %0": "=b"(cr4));
	cr4 |= 0x10;
	asm volatile("mov %0, %%cr4":: "b"(cr4));


	//reads cr0, switches the "paging enable" bit, and writes it back.
	asm volatile("mov %%cr0, %0": "=b"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0":: "b"(cr0));
}

/*
* add_prog_page
*	description: look for a free page to add to the program
*	input: none
*	output: none
*	return: index of page on success
*			-1 on failure
*	side effect: enter protected mode, and turn on paging.
*/
int32_t add_prog_page() {
	uint32_t i;
	uint32_t addr = 0;

	for(i=0;i<MAX_NUM_PROG;i++) {
		/* If the page is free*/
		if(prog_used_page[i] == 0) {
			
			/* Set the busy bit and calculate the address*/
			prog_used_page[i] = 1;	//set the page in use
			addr = FIRST_PROG_ADR + i*PROG_PAGE_SIZE;	//find the physical address for the page

			page_directory[PROG_PD_ENTRY] = addr | _4MB_PAGE | CACHE_DISABLE | USER_SUPER | READ_WRITE | PRESENT;	//set the pd entry for 128MB virtual address	
			// write pointer to page directory into PDB Register
			asm volatile("mov %0, %%cr3":: "b"(page_directory));	
			break;
		}
	}

	if(i >= MAX_NUM_PROG) {
		return -1; // max number of program reached
	}

	return i;
}


/*
* set_prog_page
*	description: set the page for program at address idx
*	input: none
*	output: none
*	return: 0
*	side effect: enter protected mode, and turn on paging.
*/
int32_t set_prog_page(uint32_t idx) {

	if(idx < 0 || idx >= MAX_NUM_PROG) {
		return -1; // invalid idx of program
	}

	uint32_t addr = FIRST_PROG_ADR + idx*PROG_PAGE_SIZE;

	page_directory[PROG_PD_ENTRY] = addr | _4MB_PAGE | CACHE_DISABLE | USER_SUPER | READ_WRITE | PRESENT; //set the pd entry for 128MB virtual address	
	// write pointer to page directory into PDB Register
	asm volatile("mov %0, %%cr3":: "b"(page_directory));

	prog_used_page[idx] = 1;	// reset the page to be used

	return 0;
}

/*
*free_prog_page
*	description: free the page of the program
*	input: none
*	output: none
*	return: 0 on success
*			-1 on failure
*	side effect: enter protected mode, and turn on paging.
*/

int32_t free_prog_page (uint32_t idx) {

	if(idx < 0 || idx >= MAX_NUM_PROG) {
		return -1; // invalid idx of program
	}

	page_directory[PROG_PD_ENTRY] = 0; // reset the pd_entry to 0
	// write pointer to page directory into PDB Register
	asm volatile("mov %0, %%cr3":: "b"(page_directory));
	
	prog_used_page[idx] = 0;	// reset the page to be unused

	return 0;
}

/*
*add_new_pt
*	description: add a new page
*	input: none
*	output: none
*	return: 0 on success
*			-1 on failure
*	side effect: enter protected mode, and turn on paging.
*/
int32_t add_new_pt(uint32_t vir, uint32_t physical){
	// Get the index for page directory and page table
	uint32_t pd_idx = vir >> 22;
	uint32_t pt_idx = (vir >> 12) & 0x3FF;

	// Set up page directory and page table
	page_directory[pd_idx] = (uint32_t) pt_vid1;
	page_directory[pd_idx] |= USER_SUPER | READ_WRITE | PRESENT;
	pt_vid1[pt_idx] = (physical & pt_mask) | 7;
	
	// flush TLB
	asm volatile("mov %0, %%cr3":: "b"(page_directory));
	return 0;
}


int32_t set_video_page(uint32_t set_on) {

	// Get the index for page directory and page table
	uint32_t pd_idx = VIDEO_VIRTUAL >> 22;
	uint32_t pt_idx = (VIDEO_VIRTUAL >> 12) & 0x3FF;

	// Set up page directory and page table
	page_directory[pd_idx] = (uint32_t) pt_vid1;
	page_directory[pd_idx] |= USER_SUPER | READ_WRITE | PRESENT;
	
	if(set_on) {
		pt_vid1[pt_idx] = (VIDEO & pt_mask) |  USER_SUPER | READ_WRITE | PRESENT;
	}	
	else {
		pt_vid1[pt_idx] = (VIDEO_BACKUP & pt_mask) |  USER_SUPER | READ_WRITE | PRESENT;
	}


	// write pointer to page directory into PDB Register
	asm volatile("mov %0, %%cr3":: "b"(page_directory));

	return 0;
}

