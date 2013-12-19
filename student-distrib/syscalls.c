#include "syscalls.h"
#define IN_USE 1
#define VIDEO_MEMORY_ADDRESS 0x8048000
#define VIDEO_ASSIGNED_MEM_ADDR 0x8400000
#define PAGE_SIZE_T 0x400000
#define PAGE_SIZE_4KB 0x1000
#define MAX_FILE_NUM 8
#define EIGHT_BIT_MASK 0xFF

extern int pcb_used[MAX_NUM_PROG];
extern pcb_t * term_curr_pcb[3];
/*
 * access_ok
 *   DESCRIPTION: check if the pointer user passes in is valid
 *   INPUTS: addr - address of pointer
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t access_ok(uint32_t addr){
	if(addr == NULL) return ERROR;
	if(addr < USER_PROG_ADDR || addr >= VIDEO_ASSIGNED_MEM_ADDR + PAGE_SIZE_4KB) return ERROR;
	return 0;
}

/*
 * parse
 *   DESCRIPTION: parses the command and args for execute
 *   INPUTS: command - the command string to parse
 			 cmd - the buffer to write the command (file name) after parsing
 			 new_pcb - the pcb of the current process to write the argument buffer into
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: writes to cmd and updates values in new_pcb
 */
int32_t parse(const uint8_t* command, uint8_t* cmd, pcb_t* new_pcb)
{
	// sanity check
	int i, n, k;
	if (cmd == NULL || command == NULL)
	{
		return -1;
	}

	i = 0; k = 0;
	/* Remove leading zeroes */
	while (command[i] == ' '){i++;} 


	for (; command[i] != ' ' && command[i] != 0; i++) // write the first part of the command, the file name of the program, to cmd
	{
		cmd[k++] = command[i];
	}

	cmd[k] = '\0'; // null terminates the file name

	// if new_pcb is passed in as NULL, we take it as the caller only wants
	// the command name, not the argument
	if(new_pcb != NULL){
		n = 0;
		while (command[i] == ' '){i++;} // eats up extra spaces between filename and first arg


		// parse argument
		while (command[i])
		{
			new_pcb->argument_buffer[n] = command[i];
			i++;
			n++;
		}

		// NULL terminates character and update the length
		new_pcb->argument_buffer[n] = '\0';
		new_pcb->argument_buffer_size = n + 1;
	}
	
	return 0;
}

/*
 * set_up_fops
 *   DESCRIPTION: initializes the fops table for terminal, rtc, and file system
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets function pointers
 */
void set_up_fops(){
	// There are just 4 different file types, so all we need to do 
	// is set up 4 fops_table and point the whatever file
	// to the correct file type

	//  Terminal
	terminal_fops.read = (read_func) terminal_read;
	terminal_fops.write = (write_func) terminal_write;
	terminal_fops.close = (close_func) terminal_close;
	terminal_fops.open = (uint32_t *) terminal_open;

	// RTC file.
	rtc_fops.open = (uint32_t *) rtc_open;
	rtc_fops.read = (read_func) rtc_read;
	rtc_fops.write = (write_func) rtc_write;
	rtc_fops.close = (close_func) rtc_close;

	// Normal file
	file_fops.open = (uint32_t *) file_open;
	file_fops.read = (read_func) file_read;
	file_fops.write = (write_func) file_write;
	file_fops.close = (close_func) file_close;

	//Directory
	dir_fops.open = (uint32_t *) dir_open;
	dir_fops.read = (read_func) dir_read;
	dir_fops.write = (write_func) dir_write;
	dir_fops.close = (close_func) dir_close;
}

/*
 * get_kstack_addr
 *   DESCRIPTION: Return the kernel stack of child process
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 
 */
uint32_t get_kstack_addr(pcb_t * child_pcb){
	return (uint32_t) child_pcb + PCB_OFFSET - 4;
}

/*
 * check_magic_header
 *   DESCRIPTION: Check the magic number to ensure the dir is executable
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t check_magic_header(const unsigned char * buf){
	if(buf == 0) return 1;
	return (buf[0] != 0x7F || buf[1] != 'E' || buf[2] != 'L' || buf[3] !='F');
}

/*
 * add_pcb
 *   DESCRIPTION: Function to allocate another process control block when executing
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t add_pcb(){
	int i;

	// Look for an empty PCB
	for(i = 0; i < MAX_NUM_PROG; i++){
		if(pcb_used[i] == 0) break;
	}
	
	// Have to check if whether it's really free PCB
	// Or all pcb are already taken up.
	if(i < MAX_NUM_PROG) {
		pcb_used[i] = 1;
		return i;
	}
	else return -1;
}

/*
 * get_pcb
 *   DESCRIPTION: getter function for current pcb
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
pcb_t* get_pcb() {
	// The way we design is this: We put each pcb at 8KB from each other
	// and the first one is 8K away from the end of kernel space.
	// The reason for this is so that it's easier to find the address of the current PCB
	// And easier to implement, since we only allow up to 6 process to execute

	uint32_t curr_esp;
	uint32_t flags;
	
	cli_and_save(flags);
	
	asm volatile(" \n\
		movl %%esp, %0\n\
	":"=g"(curr_esp));

	restore_flags(flags);
	return (pcb_t *) (curr_esp & PCB_MASK);
}

/*
 * initialize_pcb
 *   DESCRIPTION: initializes the passed pcb with null values and fops
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t initialize_pcb(pcb_t * new_pcb){

	int i;
	// init content of file desc array to 0 and NULL
	for (i = 0; i < MAX_FILE_NUM; i++)
	{
		new_pcb->file_desc[i].inode_p = (uint32_t*)NULL;
		new_pcb->file_desc[i].fops_p = (fops_table_t*)NULL;
		new_pcb->file_desc[i].flags = (uint32_t)0;
		new_pcb->file_desc[i].file_pos = (uint32_t)0;
	}
	/* Initialize the argument buffer*/
	new_pcb->argument_buffer_size = 0;
	new_pcb->argument_buffer[0] = 0;

	/* Attach the process to the current terminal */
	new_pcb->terminal_number = current_active_terminal;

	/* Initialize stdin*/
	new_pcb->file_desc[0].inode_p = NULL;
	new_pcb->file_desc[0].fops_p = &terminal_fops;
	new_pcb->file_desc[0].flags = 1;
	new_pcb->file_desc[0].file_pos = 0;
	
	/*Initialize stdout*/
	new_pcb->file_desc[1].inode_p = NULL;
	new_pcb->file_desc[1].fops_p = &terminal_fops;
	new_pcb->file_desc[1].flags = 1;
	new_pcb->file_desc[1].file_pos = 0;

	// initialize tssESP
	new_pcb->tssESP =  get_kstack_addr(new_pcb);

	return 0;
}

/*
 * halt
 *   DESCRIPTION: the halt syscall, responsible for halting execution of the 
 				  current process and switching to the shell
 *   INPUTS: ststus
 *   OUTPUTS: none
 *   RETURN VALUE: return %bl extended to 32-bits
 *   SIDE EFFECTS: 
 */
int32_t halt (uint8_t status)
{
#ifndef SYSCALL_BETA
	// For halt, we follow a hacky way suggested by Puskar :D
	// Instead of IRET, which may cause lots of erors , which 
	// we actually spend hours trying to figure out, we jump back to a label
	// in execute(), because all the correct value that's necessary for IRET
	// is already set up for us (when use do INT 0x80)

	// This may be a huge sync problem in multi-processor. But, since we're
	// working on single processor, we don't really have this kind of problem.
	uint32_t flags;
	pcb_t* curr_pcb_ptr;

	curr_pcb_ptr = get_pcb();	// get current pcb
	
	// clear the buffer, just to make sure
	clear_buffer(curr_pcb_ptr->terminal_number);
	pcb_t* parent_pcb_ptr = curr_pcb_ptr->parent;	// get parent pcb

	if(parent_pcb_ptr != NULL) {
		// critical section
		cli_and_save(flags);	
		term_curr_pcb[curr_pcb_ptr->terminal_number] = parent_pcb_ptr; 
		restore_flags(flags);

		free_prog_page(curr_pcb_ptr->pt_idx);	// free current process's page 
		set_prog_page(parent_pcb_ptr->pt_idx);	// set parent process's page

		tss.esp0 = curr_pcb_ptr->old_tssESP;    
		// free the current page
		pcb_used[curr_pcb_ptr->pcb_idx] = 0;

		// set return value for current process in parent. note we take only 8 bits
		parent_pcb_ptr->return_val = (status & EIGHT_BIT_MASK);

		// re-build stack frame and jump back
		asm volatile( "\
			movl %0, %%esp;\
			movl %1, %%ebp;\
			jmp ret_here;\
			"::"g"(curr_pcb_ptr->old_esp), "g"(curr_pcb_ptr->old_ebp));
	}
	else {
		// free current process's page 
		// note that this is the critical section, so we have to cli
		cli_and_save(flags);	
		term_curr_pcb[curr_pcb_ptr->terminal_number] = NULL; 
		restore_flags(flags);

		// free program page and reexecute	
		free_prog_page(curr_pcb_ptr->pt_idx);	
		tss.esp0 = curr_pcb_ptr->old_tssESP;   
		pcb_used[curr_pcb_ptr->pcb_idx] = 0;
		// re-execute shell, for this CP
		execute((uint8_t*)"shell");
	}

	return 0; // should never be here, like ever, lol
#else



#endif
}

/*
 * execute
 *   DESCRIPTION: the execute syscall, called to execute a given program with given args
 *   INPUTS: command - the buffer containing the program (file name) to execute
 					   and any args
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure, 256 if killed by exception, 0-255 if stopped by halt
 *   SIDE EFFECTS: none
 */
int32_t execute (const uint8_t* command)
{
	cli();
	int i;
	uint8_t buf[MAX_BUFFER_SIZE+1];
	uint8_t cmd[MAX_BUFFER_SIZE+1];
	uint8_t temp_cmd[MAX_BUFFER_SIZE+1];
	if (command == NULL || command[0] == 0) return -1;
	// initialize the temporary holder for command
	for (i = 0; i < strlen((int8_t*)command) && i < MAX_BUFFER_SIZE; i++){
		temp_cmd[i] = command[i];
	}
	// NULL terminated
	temp_cmd[i] = 0;
	pcb_t * parent_pcb;
	uint32_t eip;
	int length;
	int pt_idx;
	int pcb_idx;
	pcb_t * child_pcb;

	dentry_t dentry;

	// parse command
	parse(command, cmd, NULL);
	/* Check if the file exists*/
	if(read_dentry_by_name(cmd, &dentry) == ERROR) return ERROR;

	/* Read the first 40 bytes, most important info*/
	if(read_data(dentry.inode, 0, buf, 40) == ERROR) return ERROR;
	
	/* Check the first magic number to verify that this is a executable*/
	if(check_magic_header(buf)) return ERROR;
	
	// load the eip from the executable file
	eip = ((unsigned int)buf[27]) << 24 | ((unsigned int)buf[26]) << 16 | ((unsigned int)buf[25]) << 8 | ((unsigned int)buf[24]);
	
	// sanity check for executable address
	if(eip < EXEC_ADDR || eip >= USER_STACK) return ERROR;

	// get file length
	length = get_file_length(dentry.inode);

	//Paging
	if((pt_idx = add_prog_page()) == ERROR) return ERROR;

	// Get PCB for child process
	if((pcb_idx = add_pcb()) == ERROR) return ERROR;
	child_pcb = (pcb_t *) (KERNEL_STACK_BOT - ((pcb_idx + 1) * PCB_OFFSET));

	// Init pcb for child process
	initialize_pcb(child_pcb);
	child_pcb->pt_idx = pt_idx;
	child_pcb->pcb_idx = pcb_idx;

	// Parse command line argument
	parse(temp_cmd, cmd, child_pcb);
	parent_pcb = get_pcb();


	// If the running program is not the first one, set parent and save
	// important information
	if(term_curr_pcb[current_active_terminal] != NULL) {
		child_pcb->parent = parent_pcb;
		asm volatile("	\n\
			movl %%esp,%0 	\n\
			movl %%ebp,%1 	\n\
			movl $ret_here,%2 	\n\
			"
			:"=a"(child_pcb->old_esp),"=b"(child_pcb->old_ebp),"=c"(child_pcb->old_eip)
			:
			:"cc"
		);
    	child_pcb->old_tssESP = tss.esp0;
	}
	else {
	// If it's the first program running, it has no parent
		child_pcb->parent = NULL;
	}

	asm volatile("movl %%esp, %[parent_esp]":[parent_esp]"=r"(parent_pcb->esp));
	asm volatile("movl %%ebp, %[parent_ebp]":[parent_ebp]"=r"(parent_pcb->ebp));
	asm volatile("movl $ret_here, %[parent_eip]":[parent_eip]"=r"(parent_pcb->eip));	

	//File loader
	// copy the program into the address
	read_data(dentry.inode, 0, (uint8_t *) EXEC_ADDR, length);

	//new PCB
	/* I think we need to update esp0 before context switch */
	tss.esp0 = get_kstack_addr(child_pcb);

	term_curr_pcb[current_active_terminal] = child_pcb;

	asm volatile("movl $ret_here, %[next_eip]":[next_eip]"=m"(parent_pcb->eip));

	// context switch
	// things to push into the stack before iret: (top to bottom)
	// EIP, CS, EFLGAGS, ESP, SS
	// and upate $ds


	asm volatile("pushfl;\
		popl %%eax;\
		movl %%eax, %[parent_flags];\
		":[parent_flags]"=r"(parent_pcb->flags)::"eax");


	asm volatile(" 			\
		cli;				\
		movw %0, %%ax; 		\
		movw %%ax, %%ds; 	\
		movw %%ax, %%es; 	\
      	movw %%ax, %%fs; 	\
      	movw %%ax, %%gs; 	\
		pushl %0;			\
		pushl %1;			\
		pushfl;				\
		popl %%eax;			\
		orl $0x200, %%eax;	\
		pushl %%eax;		\
		pushl %2;			\
		pushl %3;			\
		iretl;				\
	": /* no output*/
	: "g"(USER_DS), "g"(USER_STACK  - 4), "g"(USER_CS), "g"(eip)
	: "eax", "memory");

	// Label which halt jumps to
	asm volatile(" \n\
		ret_here: \n\
	":::"memory");
	send_eoi(0);
	enable_irq(0);
	// return value of process that was just executed. 	
	return parent_pcb->return_val;
}

/*
 * read
 *   DESCRIPTION: The read syscall, calls the appropriate read function for the appropriate device
 *   INPUTS: fd - the file descriptor
 			 buf - the buffer to read into
 			 nbytes - number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes)
{
	// Sanity check
	if(fd <0  || fd >= MAX_FILE_NUM) {
		return ERROR; // fd is invalid
	}


	if(access_ok((uint32_t) buf) == ERROR){
		return ERROR;
	}
	if(fd == 1 || get_pcb()->file_desc[fd].flags == 0) {
		return ERROR; // invalid file to read, not in use or write-only
	}

	// One line return, call the respective read function
	return get_pcb()->file_desc[fd].fops_p->read( (fd_t*)(&get_pcb()->file_desc[fd]),buf,nbytes);	//call the optable read of given file
}

/*
 * write
 *   DESCRIPTION: The write syscall, calls the appropriate write function
 *   INPUTS: fd - the file descriptor
 			 buf - the buffer to read from
 			 nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes)
{
	if(fd <0  || fd >= MAX_FILE_NUM) {
		return ERROR;	//fd is invalid
	}
	// check if buf is in user page.
	if(access_ok((uint32_t) buf) == ERROR){
		return ERROR;
	}

	if(fd == 0 || get_pcb()->file_desc[fd].flags == 0) {
		return ERROR; // invalid file to write
	}

	return get_pcb()->file_desc[fd].fops_p->write( (fd_t*)(&get_pcb()->file_desc[fd]), buf,nbytes);		//call the optable write of given file
}

/*
 * open
 *   DESCRIPTION: Calls the appropriate open function
 *   INPUTS: filename - the filename of the file/device to open
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on succes, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t open (const uint8_t* filename)
{
	fd_t * file_table = get_pcb()->file_desc;
	uint32_t fd;
	dentry_t file_dentry;

	// check if the file actually exists
	if((!filename || read_dentry_by_name(filename, &file_dentry) != 0)) {
		return ERROR; // invalid file name
	}

	if(access_ok((uint32_t) filename) == ERROR) return ERROR;

	// look for the slot in the file descriptor array
	for(fd=0;fd<MAX_FILE_NUM;fd++) {
		if(file_table[fd].flags == 0) {	
			break;	// current fd is unused and selected for given file
		}
	}

	if(fd >= MAX_FILE_NUM) {
		return ERROR; // file_desc is full
	}

	// call type specific open function
	switch(file_dentry.type){
		// RTC files
		case RTC_FILE:
			rtc_open( (fd_t*)(&file_table[fd]) );
			break;

		// Directory
		case DIR_FILE:
			dir_open( (fd_t*)(&file_table[fd]), filename );
			break;

		// data files
		case REG_FILE:
			file_open( (fd_t*)(&file_table[fd]), filename );
			break;
		default:
			return ERROR;	// file type invalid
	}

	return fd;	// return the file descriptor number
}

/*
 * close
 *   DESCRIPTION: calls the appropriate devices close function
 *   INPUTS: fd - the file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure, 0 on success
 *   SIDE EFFECTS: none
 */
int32_t close (int32_t fd)
{
	// We may have to close the file desc.

	if(fd <2  || fd >= MAX_FILE_NUM) {
		return ERROR;	// invalid fd to close
	}

	if(get_pcb()->file_desc[fd].flags == 0) {
		return ERROR; // file already closed
	}

	return get_pcb()->file_desc[fd].fops_p->close( (fd_t*)(&get_pcb()->file_desc[fd]) );	// call appropriate close function for the given file type via table
}

/*
 * getargs
 *   DESCRIPTION: gets the arguments passed in with the program
 *   INPUTS: buf - the buffer to write the arguments into
 *			 nbytes - the number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
 *   SIDE EFFECTS: none
 */
int32_t getargs (uint8_t* buf, int32_t nbytes)
{
	int i, n;

	pcb_t* curr_pcb = get_pcb();

	// make sure there is enough space in the buffer for arguments
	if (buf == NULL || nbytes < curr_pcb -> argument_buffer_size ||curr_pcb->argument_buffer_size <= 1 )
	{
		return ERROR;
	}

	if(access_ok((uint32_t) buf) == ERROR) return ERROR;

	// clear buffer if nbytes exceeds the available space in the buffer
	for (i = 0; i < nbytes; i++)
	{
		buf[i] = '\0';
	}
	n = (curr_pcb->argument_buffer_size) < (nbytes)? (curr_pcb->argument_buffer_size): (nbytes);
	// check that argument length is within bounds
	memcpy(buf, curr_pcb->argument_buffer, n);

	return 0;
}

/*
 * vidmap
 *   DESCRIPTION: get video memory page for process
 *   INPUTS: screen_start - pointer to beginning of video memory
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
 *   SIDE EFFECTS: none
 */
int32_t vidmap (uint8_t** screen_start)
{

	// check if address points to NULL
	if (screen_start == NULL)
	{
		return ERROR;
	}

	// check bounds of address for validity
	if(access_ok((uint32_t)screen_start) == ERROR) return ERROR;

	// clear the screen for security
	//clear();

	// Add new page table for current process's video memory

	add_new_pt(VIDEO_ASSIGNED_MEM_ADDR, VIDEO);

	// map video memory to starting address (always same address)
	*screen_start = (uint8_t*)VIDEO_ASSIGNED_MEM_ADDR;

	return 0;
}

/*
 * set_handler
 *   DESCRIPTION: 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t set_handler (int32_t signum, void* handler)
{
	return -1;

}

/*
 * sigreturn
 *   DESCRIPTION: 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t sigreturn (void)
{
	return -1;
}



