#ifndef __MOUSE_H
#define __MOUSE_H
#include "lib.h"
#include "i8259.h"
#include "terminal.h"

// Header file for mouse.c
uint8_t mouse_command[32];
uint32_t selected[2];

void mouse_init();
char mouse_read();
inline void mouse_write(char a_write) ;
inline void mouse_wait(char a_type);

// Helper functions
void do_handle_mouse();
void update_cursor();
void update_attribute(uint32_t position,uint32_t white) ;
void select (uint32_t pos1,uint32_t pos2,uint32_t white);
void run_prog(uint32_t new_pos);
void reset_cursor() ; 
void remove_cursor();
#endif
