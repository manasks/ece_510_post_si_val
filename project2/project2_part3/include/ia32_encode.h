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
 */
#define PREFIX_32BIT_ADDR		 0x67				// prefix to specify 32 bit address
#define PREFIX_16BIT  			 0x66
#define BASE_MODRM    			 0xc0
#define BASE_MODRM_MEM_DISP0	 0x00	
#define BASE_MODRM_MEM_DISP8 	 0x40	
#define BASE_MODRM_MEM_DISP32	 0x80					
#define REG_SHIFT     		 	 0x3
#define MODRM_SHIFT   			 0x6
#define RM_SHIFT      			 0x0
#define REG_MASK      			 0x7
#define RM_MASK       			 0x7
#define MOD_MASK     	    	 0x3
#define DISP_0					 0				
#define DISP_8					 8
#define DISP_32					 32	
#define STACK_SIZE				 2048
#define PREFIX_LOCK  			 0xf0

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


//Memory Address Size (in bits)

#define ADDR_32        32
#define ADDR_64        64


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
 *	int   x86_64f				 :  flag which indicates to use 64bit r8-r15 registers
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_mov_register_to_register(short mov_size, int src_reg, int dest_reg,int x86_64f, volatile char *tgt_addr)
{
	// for 16 bit mode we need to treat it special because it requires a prefix

	if (mov_size == 2) {
		(*tgt_addr ++) = PREFIX_16BIT;
	}

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
		 
	case 8:  // Move 64 bit register to 64 bit register
		if (x86_64f) {					//use R8-R15 registers
		(*(char *) tgt_addr)=(REX_PREFIX | REX_W| REX_R);
		tgt_addr += BYTE1_OFF;
		}
		else {
		(*tgt_addr ++) = REX_PREFIX | REX_W ;			// specify 64 bit operand( use RAX/RBX/RCX.. registers)
         }
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
 * Function: build_mov_immediate_to_register
 *
 * Description:
 *
 * Inputs: 
 *
 *  short mov_size               :  size of the move being requested
 *  int movabs					 :  flag to indicate move 64 bit absolute value to register
 *  long  immediate              :  immediate value to be moved
 *  int   dest_reg               :  destination reg of move
 *	int   x86_64f				 :  indicates to use 64bit r8-r15 registers
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_mov_immediate_to_register(short mov_size,int movabs, long immediate, int dest_reg, int x86_64f,volatile char *tgt_addr)
{
	
	switch(mov_size)  {

	case 1: // move 8 bit immediate value to 8 bit register
			(*(short *) tgt_addr) = ((BASE_MODRM) +  dest_reg) << 8 | 0xc6; 
			tgt_addr += BYTE2_OFF;
			(*(char *) tgt_addr) = immediate;
			tgt_addr += BYTE1_OFF;	
			break;

	case 2:  // move 16 bit immediate value to 16 bit register
			(*tgt_addr ++) = PREFIX_16BIT;
			(*(short *) tgt_addr) = ((BASE_MODRM) +  dest_reg) << 8 | 0xc7; 
		    tgt_addr += BYTE2_OFF;
			(*(short *) tgt_addr) = immediate;
		    tgt_addr += BYTE2_OFF;
			break;
			
		
	case 4: // move 32 bit immediate value to 32 bit register
			(*(short *) tgt_addr) = ((BASE_MODRM) +  dest_reg) << 8 | 0xc7; 
			tgt_addr += BYTE2_OFF;
			(*(int *) tgt_addr) = immediate;
			tgt_addr += BYTE4_OFF;
			break;

	case 8: // move 64 bit immediate value to 64 bit register
			if (x86_64f) {					//use R8-R15 registers
				(*(char *) tgt_addr)=(REX_PREFIX | REX_W| REX_B);
				tgt_addr += BYTE1_OFF;
				}
			else {
				(*tgt_addr ++) = REX_PREFIX | REX_W ;			// specify 64 bit operand( use RAX/RBX/RCX.. registers)
				}
			
			if (movabs) {
				(*tgt_addr ++) = 0xb8;   						 //move abs
				(*(long *) tgt_addr) = immediate;
				tgt_addr += BYTE4_OFF;
				tgt_addr += BYTE4_OFF;
				}
			else {
				(*(short *) tgt_addr) = ((BASE_MODRM) +  dest_reg) << 8 | 0xc7; 
				tgt_addr += BYTE2_OFF;
				(*(int *) tgt_addr) = immediate;
				tgt_addr += BYTE4_OFF;
				}
			break;	 
	default:
			fprintf(stderr,"ERROR: Incorrect size (%d) passed for immediate to register move\n", mov_size);
			return (NULL);
	}
			
    return(tgt_addr);
}


/*
 * Function: build_mov_register_to_memory
 *
 * Description:
 *
 * Inputs: 
 *
 *  short mov_size               :  size of the move being requested
 *  short   addr_size            :  size of memory address (32 bits or 64 bits)
 *  int   src		        	 :  Source reg of move
 *  int mem_reg					 :  register which contain the memory address
 *  short disp_size              :  indicates the size of memory displacement (DISP_0,DISP_8 or DISP_32)
 *  long displacement 			 :  Specifies the value of memory displacement
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_mov_register_to_memory(short mov_size, short addr_size, int src, int mem_reg, short disp_size,long displacement ,volatile char *tgt_addr)
{
	char base_modrm_mem;

	if (disp_size==0) {
		base_modrm_mem=BASE_MODRM_MEM_DISP0;
		}
	
	else if (disp_size==8) {
		base_modrm_mem=BASE_MODRM_MEM_DISP8;
		}
	else if (disp_size==32) {
		base_modrm_mem=BASE_MODRM_MEM_DISP32;
		}
		
	else {
			fprintf(stderr,"ERROR: Incorrect  Displacement size (%d) passed for register to memory move\n", disp_size);
			return (NULL);
		}
	
	if( addr_size!=32 && addr_size!=64 ) {
		fprintf(stderr,"ERROR: Incorrect  address size (%d) passed for register to memory move\n", addr_size);
		return (NULL);
	}

	switch(mov_size)  {

	case 1: if (addr_size==32) { 
				(*tgt_addr ++) = PREFIX_32BIT_ADDR;				//if address size is 32, append  prefix 0x67
				}
				
			(*(short *) tgt_addr) = ((base_modrm_mem) + (src << REG_SHIFT) + mem_reg) << 8 | 0x88;
			tgt_addr += BYTE2_OFF;	
			
				if (disp_size==8) {
						(*(char *) tgt_addr) = displacement;
						tgt_addr += BYTE1_OFF;
				 }
			    if (disp_size==32) {
						(*(int *) tgt_addr) = displacement;
						tgt_addr += BYTE4_OFF;
				 }
			break;
			
	case 2: if (addr_size==32) {
			(*tgt_addr ++) = PREFIX_32BIT_ADDR;
			}
			
			(*tgt_addr ++) = PREFIX_16BIT;				// append prefix 0x66 to select 16 bit register 
			(*(short *) tgt_addr) = ((base_modrm_mem) + (src << REG_SHIFT) + mem_reg) << 8 | (0x89) ;
			 tgt_addr += BYTE2_OFF;
			
			if (disp_size==8) {
				(*(char *) tgt_addr) = displacement;
				tgt_addr += BYTE1_OFF;
				 }
			if (disp_size==32) {
				(*(int *) tgt_addr) = displacement;
				tgt_addr += BYTE4_OFF;
				 }
			break;
		
	case 4: if (addr_size==32) { 
				(*tgt_addr ++) = PREFIX_32BIT_ADDR;				//if address size is 32, append  prefix 0x67
				}
				
			(*(short *) tgt_addr) = ((base_modrm_mem) + (src << REG_SHIFT) + mem_reg) << 8 | 0x89;
			tgt_addr += BYTE2_OFF;	
			if (disp_size==8) {
				(*(char *) tgt_addr) = displacement;
				tgt_addr += BYTE1_OFF;
			 }
		    if (disp_size==32) {
				(*(int *) tgt_addr) = displacement;
				tgt_addr += BYTE4_OFF;
			 }
			break;
	 
	case 8: if (addr_size==64) {							
			(*tgt_addr ++) = REX_PREFIX | REX_W ;			// specify 64 bit operand
			(*(short *) tgt_addr) = ((base_modrm_mem) + (src << REG_SHIFT) + mem_reg) << 8 | (0x89) ;
			 tgt_addr += BYTE2_OFF;	
			}	
			if (disp_size==8) {
				(*(char *) tgt_addr) = displacement;
				tgt_addr += BYTE1_OFF;
			 }
		    if (disp_size==32) {
				(*(int *) tgt_addr) = displacement;
				tgt_addr += BYTE4_OFF;
			 }
			 break;	 

	default:
			 fprintf(stderr,"ERROR: Incorrect size (%d) passed for register to memory move\n", mov_size);
			 return (NULL);

	}
	return(tgt_addr);
}

/*
 * Function: build_mov_memory_to_register
 *
 * Description:
 *
 * Inputs: 
 *
 *  short mov_size               :  size of the move being requested
 *  short   addr_size            :  size of memory address (32 bits or 64 bits)
 *  int   mem_reg		         :  register which contain the memory address
 *  int dest_reg			     :  Destination register 
 *  short disp_size              :  indicates the size of memory displacement (DISP_0,DISP_8 or DISP_32)
 *  long displacement 			 :  Specifies the value of memory displacement
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_mov_memory_to_register(short mov_size, short addr_size, int dest_reg, int mem_reg, short disp_size,long displacement ,volatile char *tgt_addr)
{
	char base_modrm_mem;

	if (disp_size==0) {
		base_modrm_mem=BASE_MODRM_MEM_DISP0;
		}
	
	else if (disp_size==8) {
		base_modrm_mem=BASE_MODRM_MEM_DISP8;
		}
	else if (disp_size==32) {
		base_modrm_mem=BASE_MODRM_MEM_DISP32;
		}
		
	else {
			fprintf(stderr,"ERROR: Incorrect  Displacement size (%d) passed for register to memory move\n", disp_size);
			return (NULL);
		}
	
	if( addr_size!=32 && addr_size!=64 ) {
	fprintf(stderr,"ERROR: Incorrect  address size (%d) passed for register to memory move\n", addr_size);
				return (NULL);
	}

	switch(mov_size)  {

	case 1: if (addr_size==32) { 
				(*tgt_addr ++) = PREFIX_32BIT_ADDR;				//if address size is 32, append  prefix 0x67
				}
				
			(*(short *) tgt_addr) = ((base_modrm_mem) + (dest_reg << REG_SHIFT) + mem_reg) << 8 | 0x8a;
			tgt_addr += BYTE2_OFF;	
			
			if (disp_size==8) {
				(*(char *) tgt_addr) = displacement;
				tgt_addr += BYTE1_OFF;
			 }
		    if (disp_size==32) {
				(*(int *) tgt_addr) = displacement;
				tgt_addr += BYTE4_OFF;
			 }
			break;
			
	case 2: if (addr_size==32) {
				(*tgt_addr ++) = PREFIX_32BIT_ADDR;
				}
			
			(*tgt_addr ++) = PREFIX_16BIT;				// append prefix 0x66 to select 16 bit register 
			(*(short *) tgt_addr) = ((base_modrm_mem) + (dest_reg << REG_SHIFT) + mem_reg) << 8 | (0x8b) ;
			 tgt_addr += BYTE2_OFF;
			
			if (disp_size==8) {
				(*(char *) tgt_addr) = displacement;
				tgt_addr += BYTE1_OFF;
			 }
		    if (disp_size==32) {
				(*(int *) tgt_addr) = displacement;
				tgt_addr += BYTE4_OFF;
			 }
			break;
		
	case 4: if (addr_size==32) { 
				(*tgt_addr ++) = PREFIX_32BIT_ADDR;				//if address size is 32, append  prefix 0x67
				}
				
			(*(short *) tgt_addr) = ((base_modrm_mem) + (dest_reg << REG_SHIFT) + mem_reg) << 8 | 0x8b;
			tgt_addr += BYTE2_OFF;	
			if (disp_size==8) {
				(*(char *) tgt_addr) = displacement;
				tgt_addr += BYTE1_OFF;
			 }
		    if (disp_size==32) {
				(*(int *) tgt_addr) = displacement;
				tgt_addr += BYTE4_OFF;
		    }
			break;
		 
	case 8: if (addr_size==64) {							
			(*tgt_addr ++) = REX_PREFIX | REX_W ;			// specify 64 bit operand
			(*(short *) tgt_addr) = ((base_modrm_mem) + (dest_reg << REG_SHIFT) + mem_reg) << 8 | (0x8b) ;
			 tgt_addr += BYTE2_OFF;	
			}	
			
			 if (disp_size==8) {
				(*(char *) tgt_addr) = displacement;
				tgt_addr += BYTE1_OFF;
			}
		    if (disp_size==32) {
				(*(int *) tgt_addr) = displacement;
				tgt_addr += BYTE4_OFF;
			 }
	    	 break;	 

	default:
			 fprintf(stderr,"ERROR: Incorrect size (%d) passed for register to memory move\n", mov_size);
			 return (NULL);

	}
	return(tgt_addr);
}


/*
 * Function: add_headeri
 *
 * Description:
 * Encodes ENTER instruction to set up the stack frame
 *
 * Inputs: 
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *add_headeri(volatile char *tgt_addr)
{
	(*tgt_addr ++) = 0xc8;						// opcode for ENTER inst
	(*(short *) tgt_addr) = STACK_SIZE; 	    // stack frame size 
	tgt_addr += BYTE2_OFF;
	tgt_addr++;
	return(tgt_addr);
	
}

/*
 * Function: add_endi
 *
 * Description:
 * Encodes LEAVE and RET instruction to release the stack frame and return to the caller
 *
 * Inputs: 
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *add_endi(volatile char *tgt_addr)
{

	//encode LEAVE instruction
	(*tgt_addr ++) = 0xc9;		

	//encode RET instruction 
	(*tgt_addr ++) = 0xc3;
		
	return(tgt_addr);
	
}

/*
 * Function: build_push_reg
 *
 * Description:
 * builds push register
 *
 * Inputs: 
 *
 *  int reg_index                :  index of register
 *  int x86_64f                  :  flag to indicate if we need to extend to rex format
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
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


/* Function: build_xadd
 *
 * Description:
 * This function builds the xadd instruction. 
 *
 * Inputs: 
 *
 *  short mov_size               :  size of the operands
 *  short   addr_size            :  size of memory address (32 bits or 64 bits)
 *  int  src_reg		         :  Source register
 *  int mem_reg				     :  register which contain the memory address 
 *  int x86_64f		             :  Flag to indicate the use of R8-R15 registers
 *  int lock_en		 			 :  flag to indicate the use of lock prefix
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_xadd(short mov_size, int addr_size, int src_reg,int mem_reg, int x86_64f,int lock_en, volatile char *tgt_addr)
{
	if (lock_en) {
		(*tgt_addr ++) = PREFIX_LOCK;
		}
	if (addr_size==32) {
		(*tgt_addr ++) = PREFIX_32BIT_ADDR;
		}
		
	
	switch( mov_size) {
	
	case 1: 
			(*(short *) tgt_addr)= 0xc00f;
			tgt_addr += BYTE2_OFF;
			break;
			
	case 2 : 
			(*tgt_addr ++) = PREFIX_16BIT;
				
	case 4: 
			(*(short *) tgt_addr)= 0xc10f;
			tgt_addr += BYTE2_OFF;
			break;
			
	case 8:		
			if (x86_64f) {
			(*tgt_addr ++)=(REX_PREFIX | REX_W |REX_R);
			}
			else {
			(*tgt_addr ++)=(REX_PREFIX | REX_W );
			}
			(*(short *) tgt_addr)= 0xc10f;
			tgt_addr += BYTE2_OFF;
			break;
	
	default :
	
			printf("incorrect mov_size passed to xadd instruction :%d\n",mov_size);
			
	}
	
	(*tgt_addr ++)= ((BASE_MODRM_MEM_DISP0) + (src_reg << REG_SHIFT) + mem_reg);
		
	
	 return(tgt_addr);
}

/* Function: build_xchg
 *
 * Description:
 * This function builds the xchg instruction. 
 *
 * Inputs: 
 *
 *  short mov_size               :  size of the operands
 *  short   addr_size            :  size of memory address (32 bits or 64 bits)
 *  int  src_reg		         :  Source register
 *  int mem_reg				     :  register which contain the memory address 
 *  int x86_64f		             :  Flag to indicate the use of R8-R15 registers
 *  int lock_en		 			 :  flag to indicate the use of lock prefix
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 *  returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_xchg(short mov_size, int addr_size, int src_reg,int mem_reg, int x86_64f,int lock_en, volatile char *tgt_addr)
{
	if (lock_en) {
		(*tgt_addr ++) = PREFIX_LOCK;
		}
	if (addr_size==32) {
		(*tgt_addr ++) = PREFIX_32BIT_ADDR;
		}
		
	switch( mov_size) {
	
	case 1: 
			(*(char *) tgt_addr)= 0x86;							//opcode for 8 bit register to memory exhg
			tgt_addr += BYTE1_OFF;
			break;
			
	case 2 : 
			(*tgt_addr ++) = PREFIX_16BIT;
				
	case 4: 
			(*(char *) tgt_addr)= 0x87;
			tgt_addr += BYTE1_OFF;
			break;
			
	case 8:		
			if (x86_64f) {
			(*tgt_addr ++)=(REX_PREFIX | REX_W |REX_R);
			}
			else {
			(*tgt_addr ++)=(REX_PREFIX | REX_W );
			}
			(*(char *) tgt_addr)= 0x87;
			tgt_addr += BYTE1_OFF;
			break;
	
	default :
	
			printf("incorrect mov_size passed to xadd instruction :%d\n",mov_size);
			
	}
	
	(*tgt_addr ++)= ((BASE_MODRM_MEM_DISP0) + (src_reg << REG_SHIFT) + mem_reg);
			
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
 * Function: build_mfence
 *
 * Description:
 * builds MFENCE instruction
 *
 * Inputs: 
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 * returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_mfence(volatile char *tgt_addr)
{
	(*(short *) tgt_addr) = 0xae0f;
	tgt_addr += BYTE2_OFF;
	(*tgt_addr++)=0xf0;
    return(tgt_addr);
}



/*
 * Function: build_sfence
 *
 * Description:
 * builds SFENCE instruction
 *
 * Inputs: 
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 * returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_sfence(volatile char *tgt_addr)
{
	(*(short *) tgt_addr) = 0xae0f;
	tgt_addr += BYTE2_OFF;
	(*tgt_addr++)=0xf8;
    return(tgt_addr);
}



/*
 * Function: build_lfence
 *
 * Description:
 * builds LFENCE instruction
 *
 * Inputs: 
 *  volatile char *tgt_addr      :  starting memory address of where to store instruction
 *
 * Output: 
 * returns adjusted address after encoding instruction
 *
 */
static inline volatile char *build_lfence(volatile char *tgt_addr)
{
	(*(short *) tgt_addr) = 0xae0f;
	tgt_addr += BYTE2_OFF;
	(*tgt_addr++)=0xe8;
    return(tgt_addr);
}


	
	
