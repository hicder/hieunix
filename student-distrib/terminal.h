#ifndef __TERMINAL_H_
#define __TERMINAL_H_

#include "types.h"
#include "syscalls.h"
#include "fs.h"
#include "mouse.h"
// keyboard defines
#define FIRST_BIT_SET 0x80
#define KB_PORT 0x60
#define KB_IRQ_LINE 1

#define NUM_COLS 80
#define NUM_ROWS 25

// terminal defines
#define BUFFER_SIZE 128
#define NUM_TERMINALS 3

#define SHIFT_MAKE 54
#define SHIFT_BREAK 182
#define LEFT_SHIFT_MAKE 42
#define LEFT_SHIFT_BREAK 170
#define CTRL_MAKE 29
#define CTRL_BREAK 157
#define CAPS_LOCK_MAKE 58 
#define CAPS_LOCK_BREAK 186
#define ALT_MAKE 56
#define ALT_BREAK 184

#define F1_MAKE 59
#define F2_MAKE 60
#define F3_MAKE 61

extern int current_active_terminal;
volatile unsigned char enter_flag[NUM_TERMINALS];
/* terminal syscalls */
extern int terminal_open();
extern int terminal_read(fd_t* file_desc, uint8_t* buf, uint32_t nbytes);
extern int terminal_write(fd_t* file_desc, const uint8_t* buf, uint32_t nbytes);
extern int terminal_close();

int get_process_terminal();

// keyboard interrupt handler
extern void do_handle_keyboard();
/*Keyboard init function*/
extern void keyboard_init();

/* Translate the received signal to correct key*/
extern unsigned char get_key(unsigned char c);
void clear_buffer(uint32_t termnum);
#endif
