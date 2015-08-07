/*
 * Description:
 *
 *
 * References: Intel 64 and IA-32 Architectures Software Developers Manual (SDM)
 *
 * GENERAL Instruction Format
 *
 * -----------------------------------------------------------------
 * | Instruction    |   Opcode | ModR/M | Displacement | Immediate |
 * | Prefixes       |          |        |              |           |
 * -----------------------------------------------------------------
 *
 *  7  6  5   3  2   0
 * --------------------
 * | Mod | Reg* | R/M |
 * --------------------
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <limits.h>    /* for PAGESIZE */

/*
 * definitions we need to support these functions.
 *
 * see Table 2-2 in SDM for register/MODRM encode usage
 *
 *
 */
#define PREFIX_16BIT   0x66
#define BASE_MODRM     0xc0
#define REG_SHIFT      0x3
#define MODRM_SHIFT    0x6
#define RM_SHIFT       0x0
#define REG_MASK       0x7
#define RM_MASK        0x7
#define MOD_MASK       0x3

// register defs based on MOD RM table
#define REG_EAX        0x0
#define REG_ECX        0x1
#define REG_EDX        0x2
#define REG_EBX        0x3
#define REG_ESP        0x4
#define REG_EBP        0x5
#define REG_ESI        0x6
#define REG_EDI        0x7

// register defs based for x86_64, requires REX extension
#define REG_R8        0x0
#define REG_R9        0x1
#define REG_R10       0x2
#define REG_R11       0x3
#define REG_R12       0x4
#define REG_R13       0x5
#define REG_R14       0x6
#define REG_R15       0x7

// byte offset
#define BYTE1_OFF      0x1
#define BYTE2_OFF      0x2
#define BYTE3_OFF      0x3
#define BYTE4_OFF      0x4

// ISIZE (instruction size in bytes, for move example 2byte = 16bit)

#define ISZ_1         0x1
#define ISZ_2         0x2
#define ISZ_4         0x4
#define ISZ_8         0x8

// x86_64 support defines
#define REX_PREFIX    0x40
#define REX_W         0x8
#define REX_R         0x4
#define REX_X         0x2
#define REX_B         0x1

// code generation defines

#define MAX_THREADS     4
#define MAX_DEF_INSTRS  10
#define MAX_INSTR_BYTES (3*PAGESIZE)   // allocate 3  PAGES for instruction
#define MAX_DATA_BYTES  (10*PAGESIZE)  // allocate 10 PAGES for data
#define MAX_COMM_BYTES  (PAGESIZE)     // allocate 1  PAGE for communications

// information sharing between tasks
#define NUM_PTRS 3
#define CODE 0
#define DATA 1
#define COMM 2

/*
 * Function: build_mov_register_to_register
 *
 * Description:
 *
 * Inputs: 
 *
 *  short mov_size               :  size of the move being requested
 *  int   src_reg                :  register source encoding 
 *  int   dest_reg               :  destination reg of move
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_mov_register_to_register(short mov_size, int src_reg, int dest_reg, volatile char *tgt_addr)
{
	// for 16 bit mode we need to treat it special because it requires a prefix

	if (mov_size == 2) {
		(*tgt_addr ++) = PREFIX_16BIT;
	}

	// now lets look at each size and determine which opcode required

	switch(mov_size)  {

	case 1: 
		(*(short *) tgt_addr) = ((BASE_MODRM) + (dest_reg << REG_SHIFT) + src_reg) << 8 | 0x8a;
		 tgt_addr += BYTE2_OFF;
		 break;

	case 2:  // can overload this case because same opcode, but already set prefix
	case 4: 
		(*(short *) tgt_addr) = ((BASE_MODRM) + (dest_reg << REG_SHIFT) + src_reg) << 8 | 0x8b;
		 tgt_addr += BYTE2_OFF;
		 break;


	default:
		 fprintf(stderr,"ERROR: Incorrect size (%d) passed to register to register move\n", mov_size);
		 return (NULL);

	}
			
        return(tgt_addr);
}



/*
 * Function: build_ret
 *
 * Description:
 *
 * Inputs: 
 *
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_ret(volatile char *tgt_addr)
{

	(*(char *) tgt_addr) = 0xc3;
	tgt_addr += BYTE1_OFF;
			
        return(tgt_addr);
}


/*
 * Function: build_pusha
 *
 * Description:
 *
 * builds pusha (all genp regs)
 *
 * Inputs: 
 *
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_pusha(volatile char *tgt_addr)
{

	(*(char *) tgt_addr) = 0x60;
	tgt_addr += BYTE1_OFF;
			
        return(tgt_addr);
}


/*
 * Function: build_pushf
 *
 * Description:
 *
 * builds pushf (push eflags regs on stack for safe keeping)
 *
 * Inputs: 
 *
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_pushf(volatile char *tgt_addr)
{

	(*(char *) tgt_addr) = 0x9c;
	tgt_addr += BYTE1_OFF;
			
        return(tgt_addr);
}


/*
 * Function: build_popa
 *
 * Description:
 *
 * builds popa (all genp regs from stack)
 *
 * Inputs: 
 *
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_popa(volatile char *tgt_addr)
{

	(*(char *) tgt_addr) = 0x61;
	tgt_addr += BYTE1_OFF;
			
        return(tgt_addr);
}

/*
 * Function: build_popf
 *
 * Description:
 *
 * builds popf (pop all eflages from stack)
 *
 * Inputs: 
 *
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_popf(volatile char *tgt_addr)
{

	(*(char *) tgt_addr) = 0x9d;
	tgt_addr += BYTE1_OFF;
			
        return(tgt_addr);
}

/*
 * Function: build_push_reg
 *
 * Description:
 *
 * builds push register
 *
 * Inputs: 
 *
 *  int reg_index                :  index of register
 *  int x86_64f                  :  flag to indicate if we need to extend to rex format
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_push_reg(int reg_index, int x86_64f, volatile char *tgt_addr)
{

	if (x86_64f) {
		// this is a quick hack for REX_B prefix

		(*(char *) tgt_addr)=(REX_PREFIX | REX_B);
		tgt_addr += BYTE1_OFF;
	}

	(*(char *) tgt_addr) = 0x50+reg_index;;
	tgt_addr += BYTE1_OFF;
			
        return(tgt_addr);
}

/*
 * Function: build_pop_reg
 *
 * Description:
 *
 * builds pop register
 *
 * Inputs: 
 *
 *  int reg_index                :  index of register
 *  int x86_64f                  :  flag to indicate if we need to extend to rex format
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */


static inline volatile char *build_pop_reg(int reg_index, int x86_64f, volatile char *tgt_addr)
{

	if (x86_64f) {
		// this is a quick hack for REX_B prefix

		(*(char *) tgt_addr)=(REX_PREFIX | REX_B);
		tgt_addr += BYTE1_OFF;
	}

	(*(char *) tgt_addr) = 0x58+reg_index;;
	tgt_addr += BYTE1_OFF;
			
        return(tgt_addr);
}

