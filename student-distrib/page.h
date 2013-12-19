#ifndef __PAGE_H
#define __PAGE_H

#include "types.h"	
#include "x86_desc.h"

#define TABLE_SIZE 1024
#define PAGE_SIZE 4096
#define KERNEL_ADR 0x400000  
#define VIDEO 0xB8000
#define MAX_NUM_PROG 6
#define FIRST_PROG_ADR 0x800000
#define PROG_PAGE_SIZE 0x400000
#define PROG_PD_ENTRY 32

#define PRESENT  0x1
#define READ_WRITE 0x2
#define USER_SUPER 0x4
#define CACHE_DISABLE 0x10
#define _4MB_PAGE 0x80

#define pt_mask 0xFFFFF000

/* Function to initialize paging, and set
* up some page tables.
*/
extern void paging_init();
extern int32_t add_prog_page();
extern int32_t free_prog_page (uint32_t idx);
extern int32_t set_prog_page(uint32_t idx);
extern int32_t set_video_page (uint32_t idx);
int32_t add_new_pt(uint32_t vir, uint32_t physical);
#endif
