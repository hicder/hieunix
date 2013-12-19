#include "mouse.h"

#define MOUSE_DATA_PORT 0x60
#define MOUSE_CMD_PORT 0x64
#define SCREEN_W 80
#define SCREEN_H 25


static char* video_mem = (char *)VIDEO;
uint32_t mouse_cycle=0;     //unsigned char
char mouse_byte[3];    //signed char
char mouse_x[2];         //signed char
char mouse_y[2];         //signed char
uint32_t button[2];

/*
 * mouse_init
 *   DESCRIPTION: initialize the mouse
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: mouse is initialized
 */
void mouse_init() {

	uint32_t i;
	char _status;  //unsigned char
	for(i = 0;i<2;i++) {
		mouse_x[i] = 0;
		mouse_y[i] = 0;
		button[i] = 0;
		selected[i] = 0;
	}

	strncpy((int8_t*)mouse_command,(int8_t*)"",32);

	//Enable the auxiliary mouse device
	mouse_wait(1);
	outb(0xA8,MOUSE_CMD_PORT);
	mouse_read();  //Acknowledge

	//Enable the interrupts
	mouse_wait(1);
	outb(0x20,MOUSE_CMD_PORT);
	mouse_wait(0);
	_status=inb(MOUSE_DATA_PORT);
	_status |= 2;
	_status &= ~(1 << 5);
	mouse_wait(1);
	outb(MOUSE_DATA_PORT, MOUSE_CMD_PORT);
	mouse_wait(1);
	outb(_status,MOUSE_DATA_PORT);

	//Tell the mouse to use default settings
	mouse_write(0xF6);
	mouse_read();  //Acknowledge

	//Enable the mouse
	mouse_write(0xF4);
	mouse_read();  //Acknowledge

	return;
}

/*
 * mouse_wait
 *   DESCRIPTION: artificially wait for the mouse
 *   INPUTS: a_type -- either DATA or SIGNAl
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
inline void mouse_wait(char a_type) //unsigned char
{
	uint32_t _time_out=100000; //unsigned int
	if(a_type==0)
	{
		while(_time_out--) //Data
		{
			if((inb(MOUSE_CMD_PORT) & 1)==1)
			{
				return;
			}
		}
		return;
	}
	else
	{
		while(_time_out--) //Signal
		{
			if((inb(MOUSE_CMD_PORT) & 2)==0)
			{
				return;
			}
		}
		return;
	}
}

/*
 * mouse_write
 *   DESCRIPTION: write command to the mouse
 *   INPUTS: a_write -- command
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
inline void mouse_write(char a_write) //unsigned char
{
	//Wait to be able to send a command
	mouse_wait(1);
	//Tell the mouse we are sending a command
	outb(0xD4, MOUSE_CMD_PORT);
	//Wait for the final part
	mouse_wait(1);
	//Finally write
	outb(a_write,MOUSE_DATA_PORT);
}
/*
 * mouse_read
 *   DESCRIPTION: read command from the mouse 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: the data from mouse
 *   SIDE EFFECTS: none
 */
char mouse_read()
{
	//Get's response from mouse
	mouse_wait(0); 
	return inb(MOUSE_DATA_PORT);
}


/*
 * do_handle_mouse
 *   DESCRIPTION: interrupt handler for mouse
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void do_handle_mouse(){

	if(mouse_cycle == 0) {
		mouse_cycle = 3;
		inb(MOUSE_DATA_PORT);
	}
	else {
		switch(mouse_cycle%3)
		{
			case 0:
				if(mouse_byte[0] % 2 == 1) {
					button[0] = 1;
				}
				else {
					button[0] = 0;
				}
				mouse_byte[0]=inb(MOUSE_DATA_PORT);
				mouse_cycle++;
				if(mouse_byte[0] % 2 == 1) {
					button[1] = 1;
				}
				else {
					button[1] = 0;
				}

				break;
			case 1:
				mouse_byte[1]=inb(MOUSE_DATA_PORT);
				mouse_cycle++;
				break;
			case 2:
				mouse_byte[2]=inb(MOUSE_DATA_PORT);
				mouse_cycle++;

				update_cursor();
				break;
		}
	}
	mouse_cycle = (mouse_cycle) %3 +3;
	send_eoi(12);
	enable_irq(12);
	return;
}
/*
 * update_cursor
 *   DESCRIPTION: update the cursor on the screen 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void update_cursor()
{
	uint32_t old_pos,new_pos;
	mouse_x[1] = mouse_x[0] + mouse_byte[1]/4;
	mouse_y[1] = mouse_y[0] - mouse_byte[2]/4;

	if(mouse_x[1] >= SCREEN_W) mouse_x[1] = SCREEN_W-1;
	if(mouse_x[1] <0) mouse_x[1] = 0;
	if(mouse_y[1] >= SCREEN_H) mouse_y[1] = SCREEN_H-1;
	if(mouse_y[1] <0) mouse_y[1] = 0;

	old_pos = (mouse_y[0]*SCREEN_W) + mouse_x[0];
	new_pos = (mouse_y[1]*SCREEN_W) + mouse_x[1];


	//left pressed
	if(button[1] ) {
		if(!button[0]) {
			//clear
			select(selected[0],selected[1],0);
			selected[0] = new_pos;
		}
		else {
			// update selected area
			select(old_pos,new_pos,0);
			select(selected[0],new_pos,1);
		}
	}
	else {
		if(!button[0]) {
			update_attribute(old_pos,2);
			update_attribute(new_pos,2);
		}
		else {
			// click completed
			selected[1] = new_pos;
			if(selected[0] == selected[1]) {

				run_prog(new_pos);
			}
		}
	}

	//update cursor position
	mouse_x[0] = mouse_x[1];
	mouse_y[0] = mouse_y[1];
	//  printf("status: %x x:%d y:%d\n",mouse_byte[0],mouse_x[0],mouse_y[0]);
}

/*
 * udpate_attribute
 *   DESCRIPTION: update the attribute on the screen
 *   INPUTS: position -- which coordinate
 * 			 attrib -- what attribute to be updated
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void update_attribute(uint32_t position,uint32_t attrib) {
	uint8_t* attribute = (uint8_t *)(video_mem + (position << 1) + 1); 
	if((attrib == 2 && *attribute == ATTRIB) || attrib == 1)  { 
		*attribute = MOUSE_ATTRIB;
	}
	else {
		*attribute = ATTRIB;
	}

	return;
}
/*
 * select
 *   DESCRIPTION: 
 *   INPUTS: 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void select (uint32_t pos1,uint32_t pos2,uint32_t white) {
	uint32_t i;
	if(pos1 < pos2) {
		for(i = pos1;i<= pos2;i++) {
			update_attribute(i,white);
		}
	}
	else {
		for(i = pos2;i<= pos1;i++) {
			update_attribute(i,white);
		}
	}
}
/*
 * run_prog
 *   DESCRIPTION: run when there is a mouse click 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void run_prog(uint32_t new_pos) {

	uint32_t row_start = new_pos - (new_pos % SCREEN_W);

	uint32_t i;

	for(i=0;i<32;i++) {
		mouse_command[i] = *((uint8_t *)video_mem + ((row_start + i) << 1));
	}

	enter_flag[current_active_terminal] = 1;
}

/*
 * remove_cursor
 *   DESCRIPTION: remove the current cursor 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void remove_cursor() {

	uint32_t pos = (mouse_y[1]*SCREEN_W) + mouse_x[1];
	update_attribute(pos,0);
}
/*
 * reset curosr
 *   DESCRIPTION: reset the cursor 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void reset_cursor() {

	uint32_t pos = (mouse_y[1]*SCREEN_W) + mouse_x[1];
	update_attribute(pos,2);
}


