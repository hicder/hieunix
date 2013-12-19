/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "terminal.h" 
#define VIDEO 0xB8000
#define VIDEO_1 0xB8FA0
#define VIDEO_2 0xB9F40
#define VIDEO_3 0xBAEE0
#define NUM_COLS 80
#define NUM_ROWS 25
#define ATTRIB 0x7
#define NUM_TERMINALS 3


static int screen_x[NUM_TERMINALS];
static int screen_y[NUM_TERMINALS];
// static int old_screen_x; // used for putc and scrolling
static char* video_mem = (char *)VIDEO;
static char * curr_vid_mem = (char *)VIDEO_1;
extern pcb_t* term_curr_pcb[NUM_TERMINALS];



// extern int current_active_terminal
void
clear(void)
{
    int32_t i;
    for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)(curr_vid_mem + (i << 1)) = ' ';
        *(uint8_t *)(curr_vid_mem + (i << 1) + 1) = ATTRIB;
    }

    if(current_active_terminal == get_process_terminal()){
	   	for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
	        *(uint8_t *)(video_mem + (i << 1)) = ' ';
	        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
	    }
    }
    screen_x[get_process_terminal()] = 0;
    screen_y[get_process_terminal()] = 0;
}

/*
* change_video_mem
*	description: changes video_mem to point to the correct address for 
				 current terminal
*	input: terminal_number - the number of the terminal that we're changing to
*	output: none
*	return: none
*	side effect:
*/
void change_video_mem(int old_term_num, int new_term_num)
{
	// swap the video memory between 2 terminals
	uint32_t flags;
	cli_and_save(flags);
	if(old_term_num == new_term_num) return;
	curr_vid_mem = (char *)((uint8_t * )VIDEO + (new_term_num + 1) * 4000);
	memcpy(video_mem, curr_vid_mem, 4000);
	if(!term_curr_pcb[new_term_num]){
		execute((uint8_t *)"shell");
	}
	restore_flags(flags);
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output.
 * */
int32_t
printf(int8_t *format, ...)
{
	/* Pointer to the format string */
	int8_t* buf = format;

	/* Stack pointer for the other parameters */
	int32_t* esp = (void *)&format;
	esp++;

	while(*buf != '\0') {
		switch(*buf) {
			case '%':
				{
					int32_t alternate = 0;
					buf++;

format_char_switch:
					/* Conversion specifiers */
					switch(*buf) {
						/* Print a literal '%' character */
						case '%':
							putc('%');
							break;

						/* Use alternate formatting */
						case '#':
							alternate = 1;
							buf++;
							/* Yes, I know gotos are bad.  This is the
							 * most elegant and general way to do this,
							 * IMHO. */
							goto format_char_switch;

						/* Print a number in hexadecimal form */
						case 'x':
							{
								int8_t conv_buf[64];
								if(alternate == 0) {
									itoa(*((uint32_t *)esp), conv_buf, 16);
									puts(conv_buf);
								} else {
									int32_t starting_index;
									int32_t i;
									itoa(*((uint32_t *)esp), &conv_buf[8], 16);
									i = starting_index = strlen(&conv_buf[8]);
									while(i < 8) {
										conv_buf[i] = '0';
										i++;
									}
									puts(&conv_buf[starting_index]);
								}
								esp++;
							}
							break;

						/* Print a number in unsigned int form */
						case 'u':
							{
								int8_t conv_buf[36];
								itoa(*((uint32_t *)esp), conv_buf, 10);
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a number in signed int form */
						case 'd':
							{
								int8_t conv_buf[36];
								int32_t value = *((int32_t *)esp);
								if(value < 0) {
									conv_buf[0] = '-';
									itoa(-value, &conv_buf[1], 10);
								} else {
									itoa(value, conv_buf, 10);
								}
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a single character */
						case 'c':
							putc( (uint8_t) *((int32_t *)esp) );
							esp++;
							break;

						/* Print a NULL-terminated string */
						case 's':
							puts( *((int8_t **)esp) );
							esp++;
							break;

						default:
							break;
					}

				}
				break;

			default:
				putc(*buf);
				break;
		}
		buf++;
	}

	return (buf - format);
}

/* Output a string to the console */
int32_t
puts(int8_t* s)
{
	register int32_t index = 0;
	while(s[index] != '\0') {
		putc(s[index]);
		index++;
	}

	return index;
}

void
putc(uint8_t c)
{
	uint32_t flags;
	cli_and_save(flags);
	// old_screen_x = screen_x;
    if(c == '\n' || c == '\r') {
        screen_y[current_active_terminal]++;
        if (screen_y[current_active_terminal] == NUM_ROWS)
	    {
	    	screen_y[current_active_terminal]--;
	    	scroll_up(screen_x[current_active_terminal]);
	    }
        screen_x[current_active_terminal]=0;
    } else {
       	// write to current backup video memory
    	*(uint8_t *)(curr_vid_mem + ((NUM_COLS*screen_y[current_active_terminal] + screen_x[current_active_terminal]) << 1)) = c;
    	*(uint8_t *)(curr_vid_mem + ((NUM_COLS*screen_y[current_active_terminal] + screen_x[current_active_terminal]) << 1) + 1) = ATTRIB;
    
       	// write to video memory
    	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y[current_active_terminal] + screen_x[current_active_terminal]) << 1)) = c;
    	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y[current_active_terminal] + screen_x[current_active_terminal]) << 1) + 1) = ATTRIB;
    

        screen_x[current_active_terminal]++;
        // don't reset if at bottom right of screen, handled by scroll up
        if (!((screen_x[current_active_terminal] == NUM_COLS) && (screen_y[current_active_terminal] == (NUM_ROWS-1))))
        {
        	screen_y[current_active_terminal] = (screen_y[current_active_terminal] + (screen_x[current_active_terminal] / NUM_COLS)) % NUM_ROWS;
	        screen_x[current_active_terminal] %= NUM_COLS;
        }
        else
        {
        	scroll_up(NUM_COLS);
        }
    }
     restore_flags(flags);
    // else if ((screen_x[current_active_terminal] == NUM_COLS) && (screen_y[current_active_terminal] == (NUM_ROWS-1)))
    // {
    // 	scroll_up(NUM_COLS);
    // }
}



void
terminal_putc(uint8_t c)
{
	uint32_t flags;
	cli_and_save(flags);
	uint32_t terminal_num = get_process_terminal();
	uint8_t * terminal_vid_mem = ((uint8_t * )VIDEO + (terminal_num + 1) * 4000);

    if(c == '\n' || c == '\r') {
        screen_y[terminal_num]++;
        if (screen_y[terminal_num] == NUM_ROWS)
	    {
	    	screen_y[terminal_num]--;
	    	scroll_up(screen_x[terminal_num]);
	    }
        screen_x[terminal_num]=0;
    } else {
        *(uint8_t *)(terminal_vid_mem + ((NUM_COLS*screen_y[terminal_num] + screen_x[terminal_num]) << 1)) = c;
        *(uint8_t *)(terminal_vid_mem + ((NUM_COLS*screen_y[terminal_num] + screen_x[terminal_num]) << 1) + 1) = ATTRIB;
        if(current_active_terminal == terminal_num){
        	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y[terminal_num] + screen_x[terminal_num]) << 1)) = c;
        	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y[terminal_num] + screen_x[terminal_num]) << 1) + 1) = ATTRIB;
        }
        screen_x[terminal_num]++;
        // don't reset if at bottom right of screen, handled by scroll up
        if (!((screen_x[terminal_num] == NUM_COLS) && (screen_y[terminal_num] == (NUM_ROWS-1))))
        {
        	screen_y[terminal_num] = (screen_y[terminal_num] + (screen_x[terminal_num] / NUM_COLS)) % NUM_ROWS;
	        screen_x[terminal_num] %= NUM_COLS;
        }
        else
        {
        	scroll_up(NUM_COLS);
        }
    }
    restore_flags(flags);
}

/*
 * putc_buffer
 *   DESCRIPTION: copies the given char buffer to screen
 *   INPUTS: buf - char to copy
 			 nbytes - num bytes to copy
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
putc_buffer(char buf[128], int32_t nbytes)
{
    int i;
    /*Put char by char up to nbytes*/
    for (i = 0; i < nbytes; i++)
    {
    	putc(buf[i]);
    }
}

/*
 * put_packspace
 *   DESCRIPTION: acts as backspace, deletes character to left of cursor from screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
put_backspace()
{
	/* If screen_x goes out of screen, bring back to screen.*/
	if (screen_x[current_active_terminal] <= 0)
	{
		screen_x[current_active_terminal] += NUM_COLS;
		screen_y[current_active_terminal]--;
	}

	/* Clear the last line*/

	*(uint8_t *)(curr_vid_mem + ((NUM_COLS*screen_y[current_active_terminal] + (screen_x[current_active_terminal]-1)) << 1)) = (uint8_t)(' ');
	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y[current_active_terminal] + (screen_x[current_active_terminal]-1)) << 1)) = (uint8_t)(' ');

	screen_x[current_active_terminal]--;
	
}

/*
 * get_screen_y
 *   DESCRIPTION: returns the current y position of the screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: screen_y
 *   SIDE EFFECTS: none
 */
int get_screen_y()
{
	return screen_y[get_process_terminal()];
}

/*
 * get_screen_x
 *   DESCRIPTION: returns the current x position of the screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: screen_x
 *   SIDE EFFECTS: none
 */
int get_screen_x()
{
	return screen_x[get_process_terminal()];
}

/*
 * dec_screen_y
 *   DESCRIPTION: decrements screen_y by one for wraparounds
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: decrements screen_y by one
 */
void dec_screen_y()
{
	screen_y[get_process_terminal()]--;
}

/* Convert a number to its ASCII representation, with base "radix" */
int8_t*
itoa(uint32_t value, int8_t* buf, int32_t radix)
{
	static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int8_t *newbuf = buf;
	int32_t i;
	uint32_t newval = value;

	/* Special case for zero */
	if(value == 0) {
		buf[0]='0';
		buf[1]='\0';
		return buf;
	}

	/* Go through the number one place value at a time, and add the
	 * correct digit to "newbuf".  We actually add characters to the
	 * ASCII string from lowest place value to highest, which is the
	 * opposite of how the number should be printed.  We'll reverse the
	 * characters later. */
	while(newval > 0) {
		i = newval % radix;
		*newbuf = lookup[i];
		newbuf++;
		newval /= radix;
	}

	/* Add a terminating NULL */
	*newbuf = '\0';

	/* Reverse the string and return */
	return strrev(buf);
}

/* In-place string reversal */
int8_t*
strrev(int8_t* s)
{
	register int8_t tmp;
	register int32_t beg=0;
	register int32_t end=strlen(s) - 1;

	while(beg < end) {
		tmp = s[end];
		s[end] = s[beg];
		s[beg] = tmp;
		beg++;
		end--;
	}

	return s;
}

/* String length */
uint32_t
strlen(const int8_t* s)
{
	register uint32_t len = 0;
	while(s[len] != '\0')
		len++;

	return len;
}

/* Optimized memset */
void*
memset(void* s, int32_t c, uint32_t n)
{
	c &= 0xFF;
	asm volatile("                  \n\
			.memset_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memset_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memset_aligned \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memset_top     \n\
			.memset_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     stosl           \n\
			.memset_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memset_done    \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%edx       \n\
			jmp     .memset_bottom  \n\
			.memset_done:           \n\
			"
			:
			: "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/* Optimized memset_word */
void*
memset_word(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosw           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/* Optimized memset_dword */
void*
memset_dword(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosl           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/* Optimized memcpy */
void*
memcpy(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			.memcpy_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memcpy_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memcpy_aligned \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memcpy_top     \n\
			.memcpy_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     movsl           \n\
			.memcpy_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memcpy_done    \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%edx       \n\
			jmp     .memcpy_bottom  \n\
			.memcpy_done:           \n\
			"
			:
			: "S"(src), "D"(dest), "c"(n)
			: "eax", "edx", "memory", "cc"
			);

	return dest;
}

/* Optimized memmove (used for overlapping memory areas) */
void*
memmove(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			cmp     %%edi, %%esi    \n\
			jae     .memmove_go     \n\
			leal    -1(%%esi, %%ecx), %%esi    \n\
			leal    -1(%%edi, %%ecx), %%edi    \n\
			std                     \n\
			.memmove_go:            \n\
			rep     movsb           \n\
			"
			:
			: "D"(dest), "S"(src), "c"(n)
			: "edx", "memory", "cc"
			);

	return dest;
}

/*
 * scroll_up
 *   DESCRIPTION: scrolls the screen up and keeps the cursor at the bottom of the screen
 *   INPUTS: current buffer size in terminal
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: deletes topmost video memory line, moves rest of vmem up by a row,
				   keeps screen_y at bottom
 */
void
scroll_up(int x_pos)
{
	int i;
	/* Shift screen up by 1 row*/
	memmove(curr_vid_mem, (curr_vid_mem+(NUM_COLS << 1)), ((NUM_ROWS-1)*NUM_COLS) << 1);
	if(current_active_terminal == get_process_terminal()){
		memmove(video_mem, (video_mem+(NUM_COLS << 1)), ((NUM_ROWS-1)*NUM_COLS) << 1);		
	}
	/* Clear bottom row*/
	for (i = x_pos; i > 0; i--)
	{
		put_backspace();
	}
	screen_y[get_process_terminal()] = NUM_ROWS-1;
}

/* Standard strncmp */
int32_t
strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
{
	int32_t i;
	for(i=0; i<n; i++) {
		if( (s1[i] != s2[i]) ||
				(s1[i] == '\0') /* || s2[i] == '\0' */ ) {

			/* The s2[i] == '\0' is unnecessary because of the short-circuit
			 * semantics of 'if' expressions in C.  If the first expression
			 * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
			 * s2[i], then we only need to test either s1[i] or s2[i] for
			 * '\0', since we know they are equal. */

			return s1[i] - s2[i];
		}
	}
	return 0;
}

/* Standard strcpy */
int8_t*
strcpy(int8_t* dest, const int8_t* src)
{
	int32_t i=0;
	while(src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}

	dest[i] = '\0';
	return dest;
}

/* Standard strncpy */
int8_t*
strncpy(int8_t* dest, const int8_t* src, uint32_t n)
{
	int32_t i=0;
	while(src[i] != '\0' && i < n) {
		dest[i] = src[i];
		i++;
	}

	while(i < n) {
		dest[i] = '\0';
		i++;
	}

	return dest;
}

void
test_interrupts(void)
{
	int32_t i;
	for (i=0; i < NUM_ROWS*NUM_COLS; i++) {
		video_mem[i<<1]++;
	}
}
