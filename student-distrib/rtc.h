#ifndef __RTC_H
#define __RTC_H

#define MASTER_ADDR 0x20
#define SLAVE_ADDR 0xA0
#define MASTER_DATA 0x21
#define SLAVE_DATA 0xA1
#define RTC_ADDR 0x70
#define RTC_DATA 0x71
#define RTC_IRQ_NUM 8
#include "types.h"
#include "syscalls.h"
#include "fs.h"
/* Handle RTC interrupt*/
extern void do_handle_rtc();

/* Initialize RTC and enable periodic interrupt*/
extern void init_rtc();

/* Open RTC and set frequency to 2 Hz */
extern int rtc_open(fd_t* file_desc);

/* return 0, stop or reset */
extern int rtc_close(fd_t* file_desc);

/* Change RTC interrupt frequency */
extern int rtc_write(fd_t* file_desc, const uint8_t* buf, uint32_t nbytes);

/* Return 0 after a RTC interrupt occurs */
extern int rtc_read(fd_t* file_desc, uint8_t* buf, uint32_t nbytes);


#endif
