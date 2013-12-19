/* debug.h - Useful macros for debugging
 * vim:ts=4 noexpandtab
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include "types.h"
#include "lib.h"

#ifndef ASM

#ifdef DEBUG

#define ASSERT(EXP)            \
do {                           \
	if(!(EXP)) {               \
		printf(__FILE__ ":%u: Assertion `" #EXP "\' failed.\n", __LINE__);  \
	}                          \
} while(0)

#define debugf(...)            \
do {                           \
	printf(__FILE__ ":%u: ", __LINE__);    \
	printf(__VA_ARGS__);       \
} while(0)

#else
#define ASSERT(EXP)            \
	while(0)
#define debugf(...)            \
	while(0)
#endif

#endif




extern void test_read_dentry_by_name ( uint32_t* adr) ;

extern void test_read_dentry_by_index (uint32_t* adr) ;

extern void test_read_data () ;

extern void test_read_dir ();

extern void test_read_file () ;


#endif /* _DEBUG_H */
