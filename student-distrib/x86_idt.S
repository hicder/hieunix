#define SAVE_ALL \
	pusha; \
	pushfl;

#define RESTORE_ALL \
	popfl; \
	popa;

#define SAVE_ALL_REG \
	cld;\
	pushfl;\
	pushl %ebp;\
	pushl %eax;\
	pushl %edi;\
	pushl %esi;\
	pushl %edx;\
	pushl %ecx;\
	pushl %ebx;\

#define RESTORE_ALL_REG \
	popl %ebx;\
	popl %ecx;\
	popl %edx;\
	popl %esi;\
	popl %edi;\
	popl %eax;\
	popl %ebp;\
	popfl;\

#String to be displayed in each of the BSOD
.data 
string_divide_error: .string "divide_error!" 
string_debug: .string "debug!" 
string_nmi: .string "nmi!" 
string_int3: .string "int3!" 
string_overflow: .string "overflow!" 
string_bounds: .string "bounds!" 
string_invalid_op: .string "invalid_op!" 
string_device_not_available: .string "device_not_available!" 
string_coprocessor_segment_overrun: .string "coprocessor_segment_overrun!" 
string_invalid_TSS: .string "invalid_TSS!" 
string_segment_not_present: .string "segment_not_present!" 
string_stack_segment: .string "stack_segment!" 
string_general_protection: .string "general_protection!" 
string_page_fault: .string "page_fault!" 
string_coprocessor_error: .string "coprocessor_error!" 
string_simd_coprocessor_error: .string "simd_coprocessor_error!" 
string_alignment_check: .string "alignment_check!" 
string_spurious_interrupt_bug: .string "spurious_interrupt_bug!" 
string_machine_check: .string "machine_check!" 

#sys_call_table: 	.long halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn



.text

.align 4


# divide_error:
# description: handle divide-by-zero fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
	
.globl divide_error
divide_error:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_divide_error
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

divide_error_halt:
	hlt
	jmp divide_error_halt
	iret

# debug:
# description: handle debug fault, like break points.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl debug
debug:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_debug
	call printf
	addl $4, %esp
debug_halt:
	hlt
	jmp debug_halt
	iret

# non maskable interrupt:
# description: handle NMI, which will not be stopped by CLI.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl nmi
nmi:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_nmi
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

nmi_halt:
	hlt
	jmp nmi_halt
	iret

# int3:
# description: handle int3 instruction.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl int3
int3:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_int3
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

int3_halt:
	hlt
	jmp int3_halt
	iret

# overflow:
# description: handle overflow arithmetic error.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl overflow
overflow:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_overflow
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

overflow_halt:
	hlt
	jmp overflow_halt
	iret

# bounds:
# description: handle out of bound exception.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl bounds
bounds:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_bounds
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

bounds_halt:
	hlt
	jmp bounds_halt
	iret

# invalid_op:
# description: handle invalid opcode that x86 does not understand.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl invalid_op
invalid_op:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_invalid_op
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

invalid_op_halt:
	hlt
	jmp invalid_op_halt
	iret

# device_not_available:
# description: handle NA device access.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl device_not_available
device_not_available:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_device_not_available
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

device_not_available_halt:
	hlt
	jmp device_not_available_halt
	iret

# coprocessor_segment_overrun:
# description: handle coprocessor_segment_overrun fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell
.globl coprocessor_segment_overrun
coprocessor_segment_overrun:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_coprocessor_segment_overrun
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

coprocessor_segment_overrun_halt:
	hlt
	jmp coprocessor_segment_overrun_halt
	iret

# invalid_TSS:
# description: handle invalid task state fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl invalid_TSS
.globl invalid_TSS
invalid_TSS:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_invalid_TSS
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

invalid_TSS_halt:
	hlt
	jmp invalid_TSS_halt
	iret

# segment_not_present:
# description: handle invalid segment fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl segment_not_present
.globl segment_not_present
segment_not_present:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_segment_not_present
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

segment_not_present_halt:
	hlt
	jmp segment_not_present_halt
	iret

# stack_segment:
# description: handle stack segment fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl stack_segment
.globl stack_segment
stack_segment:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_stack_segment
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

stack_segment_halt:
	hlt
	jmp stack_segment_halt
	iret

# general_protection:
# description: handle general protection, such as access priviledged data fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl general_protection
.globl general_protection
general_protection:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_general_protection
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

general_protection_halt:
	hlt
	jmp general_protection_halt
	iret

# page_fault:
# description: handle invalid mem access fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl page_fault
.globl page_fault
page_fault:
 #first, clear screen
 	cli
	#call clear
 #print the error to the screen
	pushl $string_page_fault
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp
page_fault_halt:
	hlt
	jmp page_fault_halt
	iret

# coprocessor_error:
# description: handle coprocessor error fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl coprocessor_error
.globl coprocessor_error
coprocessor_error:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_coprocessor_error
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

coprocessor_error_halt:
	hlt
	jmp coprocessor_error_halt
	iret

# simd_coprocessor_error:
# description: handle simd coprocessor error fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl coprocessor_error
.globl simd_coprocessor_error
simd_coprocessor_error:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_simd_coprocessor_error
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

simd_coprocessor_error_halt:
	hlt
	jmp simd_coprocessor_error_halt
	iret

# alignment_check:
# description: handle alignment fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl coprocessor_error.globl alignment_check
.globl alignment_check
alignment_check:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_alignment_check
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

alignment_check_halt:
	hlt
	jmp alignment_check_halt
	iret

# spurious_interrupt_bug:
# description: handle spurious_interrupt_bug fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl coprocessor_error.globl alignment_check
.globl spurious_interrupt_bug
spurious_interrupt_bug:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_spurious_interrupt_bug
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

spurious_interrupt_bug_halt:
	hlt
	jmp spurious_interrupt_bug_halt
	iret

# machine_check:
# description: handle machine_check fault.
# input: none
# output: none
# return: none
# side effect: will halt forever and will not return to shell.globl coprocessor_error.globl alignment_check
.globl machine_check
machine_check:
    cli
 #first, clear screen
	call clear
 #print the error to the screen
	pushl $string_machine_check
	call printf
	addl $4, %esp
	pushl $256
	call halt
	addl $4, %esp

machine_check_halt:
	hlt
	jmp machine_check_halt
	iret

# handle_rtc:
# description: handle real time clock.
# input: none
# output: none
# return: none
# side effect: enable periodic timer interrupt
.globl handle_rtc
handle_rtc:
	SAVE_ALL
	call do_handle_rtc
	RESTORE_ALL
iret

# handle_keyboard:
# description: handle keyboard interrupt clock.
# input: none
# output: none
# return: none
# side effect: enable keyboard input
.globl handle_keyboard
handle_keyboard:
	SAVE_ALL
	#call clear
	call do_handle_keyboard
	RESTORE_ALL
iret

.globl handle_pit
handle_pit:
	cli
	SAVE_ALL_REG
	call do_handle_pit
	RESTORE_ALL_REG
	sti
iret

.globl handle_mouse
handle_mouse:
	cli
	SAVE_ALL
	#call clear
	call do_handle_mouse
	RESTORE_ALL
	sti
iret
