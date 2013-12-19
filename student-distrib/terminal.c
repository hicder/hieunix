#include "terminal.h"
#include "lib.h"
#include "i8259.h"



// input buffer
static char input_buffer[NUM_TERMINALS][BUFFER_SIZE];
static char enter_buffer[BUFFER_SIZE];
static int curr_screen_pos_y;
static int curr_screen_pos_x;
static int curr_buff_pos[NUM_TERMINALS];
int current_active_terminal;
// function key flags
static unsigned char shift_flag;
static unsigned char caps_lock_flag;
static unsigned char ctrl_flag;
static unsigned char alt_flag;

/* Keymap to translate from character received by keyboard
* to ASCII character.
* Source: http://svn.pa23.net/svn/pub/rebootos/branches/purpos-revised/keyboard.c
*/
static unsigned char keymap[128] =
{
    0,  0/* esc */, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  0,			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  0/*prnt scrn*/,
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/* Keymap to translate from character received by keyboard when shift is pressed
* to ASCII character.
* Source: http://svn.pa23.net/svn/pub/rebootos/branches/purpos-revised/keyboard.c
*/
static unsigned char shift_keymap[128] =
{
    0,  0/* esc */, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  0,			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  0/*prnt scrn*/,
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/* Keymap to translate from character received by keyboard when caps lock is on
* to ASCII character.
* Source: http://svn.pa23.net/svn/pub/rebootos/branches/purpos-revised/keyboard.c
*/
static unsigned char caps_lock_keymap[128] =
{
    0,  0/* esc */, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* BACKSPACE */
  0,			/* TAB */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',	/* ENTER KEY */
    0,			/* 29   - CONTROL */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';',	/* 39 */
 '\'', '`',   0,		/* LEFT SHIFT */
 '\\', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', ',', '.', '/',   0,				/* RIGHT SHIFT */
  0/*prnt scrn*/,
    0,	/* ALT */
  ' ',	/* SPACE BAR */
    0,	/* CAPS LOCK */
    0,	/* 59 - F1 KEY ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/* Keymap to translate from character received by keyboard when caps lock is on and shift is pressed
* to ASCII character.
* Source: http://svn.pa23.net/svn/pub/rebootos/branches/purpos-revised/keyboard.c
*/
static unsigned char shift_cl_keymap[128] =
{
    0,  0/* esc */, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  0,			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n',	/* enter key */
    0,			/* 29   - control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':',	/* 39 */
 '"', '~',   0,		/* left shift */
 '|', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', '<', '>', '?',   0,				/* right shift */
  0/*prnt scrn*/,
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/*
 * keyboard_init
 *   DESCRIPTION: enable keyboard interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void keyboard_init()
{
	enable_irq(KB_IRQ_LINE);
}

/*
 * do_handle_keyboard
 *   DESCRIPTION: this function reads input from the keyboard and displays the pressed key on the screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: EOI is sent to the keyboard to acknowledge the interrupt, screen changes based on key pressed
 */
void do_handle_keyboard(){
	unsigned char c, key, fn_key;
	int i;
	uint32_t flags;
	cli_and_save(flags);	
  /* Disable the IRQ line for keyboard */
	disable_irq(KB_IRQ_LINE);

  /* Receive the character pressed */
	c = inb(KB_PORT);

	// get ascii character
	key = get_key(c);

	// set fn key flag
	fn_key = ((c == SHIFT_MAKE) ||
	 		  (c == CAPS_LOCK_MAKE) ||
	 		  (c == CTRL_MAKE) ||
	 		  (c == SHIFT_BREAK) ||
	 		  (c == CAPS_LOCK_BREAK) ||
	 		  (c == CTRL_BREAK) ||
	 		  (c == LEFT_SHIFT_BREAK) ||
	 		  (c == LEFT_SHIFT_MAKE) ||
	 		  (c == ALT_MAKE) ||
	 		  (c == ALT_BREAK));
	
	/*sync with libc screen y*/
	curr_screen_pos_y = get_screen_y();
	curr_screen_pos_x = get_screen_x();

	select (selected[0],selected[1],0);
    /* Check if the key was released (check for break code) and update flags */
	if(c & FIRST_BIT_SET)
	{
		/*Check modifier keys*/
		switch (c)
		{
			case LEFT_SHIFT_BREAK: // falls through to shift_break
			case SHIFT_BREAK:
				shift_flag = 0;
				break;
				/*Caps Lock key*/
			case CAPS_LOCK_BREAK:
				caps_lock_flag = !caps_lock_flag;
				break;
				/*CTRL key*/
			case CTRL_BREAK:
				ctrl_flag = 0;
				break;
			case ALT_BREAK:
				alt_flag = 0;
				break;
			default:
				break;
		}
	}
	else // handle key pressed down (flag keys and normal key recognition)
	{
		/*Check modifier key*/
		switch (c)
		{
			case LEFT_SHIFT_MAKE: // falls through to shift_make
			case SHIFT_MAKE:
				shift_flag = 1;
				break;
				/*CTRL key*/
			case CTRL_MAKE:
				ctrl_flag = 1;
				break;
			case ALT_MAKE:
				alt_flag = 1;
				break;
			default:
				break;
		} // caps lock handled on break

		// handles terminal switch
		if ((alt_flag == 1) && (c == F1_MAKE || c == F2_MAKE || c == F3_MAKE))
		{
			send_eoi(KB_IRQ_LINE);
			enable_irq(KB_IRQ_LINE);
			int old_num = current_active_terminal;
			int new_num = c - F1_MAKE;
			change_video_mem(old_num, new_num);
			reset_cursor();
		}
		else if (key == '\b') // backspace handling code
		{
				input_buffer[current_active_terminal][curr_buff_pos[current_active_terminal] - 1] = 0;
				curr_buff_pos[current_active_terminal]--;
				/*Prevent go before 0*/
				if (curr_buff_pos[current_active_terminal] < 0)
				{
					curr_buff_pos[current_active_terminal] = 0;
				}
				else
				{
					/*Sanity check done, delete a character in the screen from video mem*/
					put_backspace(current_active_terminal);
				}
		}
		else if ((key == '\n') || (key == '\r')) // handle return
		{
			enter_flag[current_active_terminal] = 1;
			/*Clear the buffer*/
			for (i = 0; i < BUFFER_SIZE; i++)
			{
				enter_buffer[i] = input_buffer[current_active_terminal][i];
				input_buffer[current_active_terminal][i] = 0;
			}

			putc(key);
			curr_buff_pos[current_active_terminal] = 0;
		}
		else if (((key == 'l') || (key == 'L')) && (ctrl_flag == 1)) 
		{
			/*Handle Ctrl - L*/
			/* empty screen and delete current buffer */
			clear();
			for (i = 0; i < BUFFER_SIZE; i++)
			{
				input_buffer[current_active_terminal][i] = 0;

			}
			// Clear the buffer
			curr_buff_pos[current_active_terminal] = 0;
			
		}
		/*
		else if (((key == 'c') || (key == 'C')) && (ctrl_flag == 1))
		{
			// handles killing the current process
			send_eoi(KB_IRQ_LINE);
			enable_irq(KB_IRQ_LINE);
			halt(5); // ends this function early
		}
		*/
		else if ((!fn_key) && (curr_buff_pos[current_active_terminal] < BUFFER_SIZE)) 
		{
			/*prevent overflow, normal character keys*/
			if (key != 0)
			{
				input_buffer[current_active_terminal][curr_buff_pos[current_active_terminal]] = key;
				curr_buff_pos[current_active_terminal]++;
				putc(key);
			}
		}
	}

  /* Re-enable the interrupt line and send EOI.*/

	send_eoi(KB_IRQ_LINE);
	enable_irq(KB_IRQ_LINE);
	restore_flags(flags);
}

/*
 * get_key
 *   DESCRIPTION: looks up relevant key in relevant keymap based on flags
 *   INPUTS: scancode of pressed key
 *   OUTPUTS: associated char for key
 *   RETURN VALUE: char for key pressed
 *   SIDE EFFECTS: none
 */
unsigned char
get_key(unsigned char c)
{
	unsigned char retval;

	retval = keymap[c]; // default

	/* Since we have different keymap, we need to look up the key depending
	* one which keymap we should use.
	*/
	if (caps_lock_flag)
	{
		/*Only caps*/
		retval = caps_lock_keymap[c];
	}
	if (shift_flag)
	{
		/*Only shift*/
		retval = shift_keymap[c];
	}
	if (shift_flag && caps_lock_flag)
	{
		/*Caps and shift*/
		retval = shift_cl_keymap[c];
	}
	
	return retval;
}

// TERMINAL SYSCALLS

/*
 * terminal_open
 *   DESCRIPTION: initialized the buffers, flags and scroll position used by the terminal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: enables keyboard interrupts
 */
extern int32_t terminal_open()
{
	int i;

	// Initialize the keyboard
	keyboard_init();

	// init buffers
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		input_buffer[current_active_terminal][i] = 0;
		enter_buffer[i] = 0;
	}

	/*Initialize variables*/
	curr_screen_pos_y = 0;
	curr_screen_pos_x = 0;
	curr_buff_pos[current_active_terminal] = 0;
	caps_lock_flag = 0;
	shift_flag = 0;
	ctrl_flag = 0;
	alt_flag = 0;
	current_active_terminal = 0; // double check this init val

	return 0;
}

/*
 * terminal_read
 *   DESCRIPTION: copies nbytes of terminal buffer to given buffer pointer
 *   INPUTS: buf - buffer to copy to
 			 nbytes - num bytes to copy 
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes sucessfuly copied, -1 if error (null pointer)
 *   SIDE EFFECTS: none
 */
extern int32_t terminal_read(fd_t* file_desc, uint8_t* buf, uint32_t nbytes)
{
	uint32_t i;
	uint32_t cnt;
	// check buffer
	if (buf == NULL)
	{
		return -1;
	}
	
	int term_num = get_process_terminal();

	while (!enter_flag[term_num]);// spin until terminal sees enter

	if( strncmp((int8_t*)mouse_command,(int8_t*)"",32) ) {
		for(i = 0;i<32;i++) {	
			buf[i] = mouse_command[i];  
		}
		strncpy((int8_t*) mouse_command, (int8_t*)"",32);
		enter_flag[term_num] = 0;
		curr_buff_pos[current_active_terminal] = 0;
		for (i = 0; i < BUFFER_SIZE; i++)
		{
			// enter_buffer[i] = input_buffer[current_active_terminal][i];
			input_buffer[current_active_terminal][i] = 0;
		}
		return 32;
	}
	

	enter_flag[term_num] = 0;
	cnt = (nbytes > BUFFER_SIZE) ? BUFFER_SIZE : nbytes;
	// clear and copy current buffer to given buffer
	for (i = 0; i < cnt; i++)
	{
		// copy until the newline character, or nbytes
		buf[i] = enter_buffer[i];
		if (enter_buffer[i] == '\0')
		{
			break;
		}
	}
	buf[i] = '\n';
	return i + 1;
}

/*
 * terminal_write
 *   DESCRIPTION: writes nbytes of given buffer to screen
 *   INPUTS: buf - the buffer to put to screen
 			 nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes written
 *   SIDE EFFECTS: none
 */
extern int32_t terminal_write(fd_t* file_desc,const uint8_t* buf, uint32_t nbytes)
{
	int i;

	/* Basic sanity check*/
	if (buf == NULL)
	{
		return -1;
	}
	/* Put char by char to the scree. Note that we don't use puts() 
	* because we're not guaranteed NULL terminated char
	*/
	for (i = 0; i < nbytes; i++)
	{
		terminal_putc(((char*)buf)[i]);
		// sync_screen();
	}

	return nbytes;
}

/*
 * terminal_close
 *   DESCRIPTION: disables interrupts on keyboard and returns 0 for success
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for success
 *   SIDE EFFECTS: disables interrupts for keyboard.
 */
extern int32_t terminal_close()
{
	/* Disable IRQ line*/

	return 0;
}

/*
 * get_process_terminal
 *   DESCRIPTION: gets the terminal number attached to the currently scheduled process
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: the terminal number for the current process
 *   SIDE EFFECTS: none
 */
 int get_process_terminal()
 {
 	pcb_t* curr_pcb = get_pcb();
 	return curr_pcb->terminal_number;
 }

/*
 * clear_buffer
 *   DESCRIPTION: clear the buffer of the terminal
 *   INPUTS: termnum -- the terminal buffer to be cleared
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: buffer is cleared!
 */
void clear_buffer(uint32_t termnum){
	int i;
	curr_buff_pos[termnum] = 0;
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		// enter_buffer[i] = input_buffer[current_active_terminal][i];
		input_buffer[termnum][i] = 0;
	}
}
