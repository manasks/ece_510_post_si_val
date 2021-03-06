
//
// simple encoding example for IA-32 validation project
//
// project_part2
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "ia32_encode.h"


#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

// globals to aid debug to start
volatile char *mptr=0,*next_ptr=0, *mdptr=0;
int num_inst=0;

// declarations for starting test
typedef int (*funct_t)();     				
funct_t start_test;
int executeit();

long data_addr;    // starting addr of data buffer in memory
main(int argc, char *argv[])
{
	
	int ibuilt=0,rc=0;
	int inst_count=0, seed;
	/* allocate buffer to perform stores and loads to, and set permissions  */

	mdptr = (volatile char *)mmap(
		(void *) 0,
		(MAX_DATA_BYTES+PAGESIZE-1),
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED,
		0, 0
		);
	data_addr=(long)mdptr;  			// base addr for data access 
		
	if(mdptr == MAP_FAILED) { 
		printf("data mptr allocation failed\n"); 
		exit(1); 
	}


	/* allocate buffer to build instructions into, and set permissions to allow execution of this memory area */

	mptr = (volatile char *)mmap(
		(void *) 0,
		(MAX_INSTR_BYTES+PAGESIZE-1),
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_SHARED,
		0, 0
		);

	if(mptr == MAP_FAILED) { 
		printf("instr  mptr allocation failed\n"); 
		exit(1); 
	}

	next_ptr=mptr;                  // init next_ptr
	
	
	if (argc!=3 ) {
		fprintf(stderr," Command line arguments error: Expecting instruction count and seed no\n  "); 
		exit(1); 
	}
		
	// read the command line arguments 
	inst_count= atoi(argv[1]);     // number of random instructions to be generated 
	seed=atoi(argv[2]);				// seed number
	
	ibuilt=build_instructions(inst_count,seed);  	// build instructions

	/* Execute the instructions */

	start_test=(funct_t) mptr;
	executeit(start_test);

	fprintf(stderr,"generation program complete, %d instructions generated, and executed\n",ibuilt);

	// clean up the allocation before getting out

	munmap((caddr_t)mdptr,(MAX_DATA_BYTES+PAGESIZE-1));
	munmap((caddr_t)mptr,(MAX_INSTR_BYTES+PAGESIZE-1));


}


//
// Routine:  build_instructions
//
// Description: Creates a stack frame and lays down random instructions
//
// INPUTS: 
//	int inst_count : number of random instructions to be generated 
//  int seed       :  Seed number 
// 
// OUTPUT: returns the number of instructions built
// 
int build_instructions(int inst_count,int seed) {
	int i;
	srand(seed);
	
	next_ptr= add_headeri(next_ptr);							//encode Enter inst
	
	// Save the registers 
	next_ptr= build_push_reg(REG_EBX,0,next_ptr);						
	next_ptr= build_push_reg(REG_R12,1,next_ptr);						
	next_ptr= build_push_reg(REG_R13,1,next_ptr);						
	next_ptr= build_push_reg(REG_R14,1,next_ptr);						
	next_ptr= build_push_reg(REG_R15,1,next_ptr);						
	
	// set the base addr for data access from memory
	next_ptr=build_mov_immediate_to_register(ISZ_8,1, data_addr ,REG_EAX,0, next_ptr);				
	
	
	for (i=0; i<inst_count; i++)
	{
		int inst_type,inst_size;
		int REG1,REG2; 
		int x86_64f,disp,addr32;	
		short immediate;
		short  mem_displacement;
		
		
		
		// Generate the randomized inputs within the specified range 
		inst_type= rand_generator(1,4);
		REG1= rand_generator(1,7);					// Generate random registers 
		REG2= rand_generator(1,7);
		x86_64f= rand_generator(0,1);				// indicates whether to use r8-r15 registers 
		inst_size= rand_generator(1,8);				// specifies the inst size
		immediate=rand_generator(20,500);			// generates random immediate value within 20 - 100
		addr32= rand_generator(0,1);				// indicates whether to use 32 bit memory address or 64 bits
		disp= rand_generator (0,2); 				// specifies DISP_0,DISP_32 or DISP_32
		mem_displacement= rand_generator (10,100);   // generates random memory displacement value within 10- 100
		
		/* Constrain the randomized inputs */
		
		// set the displacement type depending on the random no generated							
		if(disp==0) {
			disp=DISP_0;
		} 
		 else if (disp==1) {
			disp=DISP_8;
		}
		else {
		disp=DISP_32; }
		
		// Map the instruction size generated to valid instruction size
		if (inst_size==3 ) {
			inst_size=8; }
		if (inst_size==5 ) {
			inst_size=8; }
		if (inst_size==6 ) {
			inst_size=1; }			
		if (inst_size==7 ) {
			inst_size=4; }
		
		// set the memory addr type depending on the random no generated
						
		if (addr32==0){
			addr32=ADDR_32;
			}
		else {
		 addr32=ADDR_64;
		}
		 
		//for ISZ_8 set memory address as 64 bits  
		if (inst_size==8) {
			addr32=ADDR_64;
		}
		
		// Map register no 4 and 5 to 1 and 3
		if ( REG1==5 || 4 ) {
			REG1=1; }
		
		if (REG2==5 || 4) {
			REG2= 3; }
		
		// for reg to mem and mem to reg instructions set mem address as 64 bits (RAX) 
		if (inst_type== 3 || 4) {
			addr32=ADDR_64;
		}
		
		switch(inst_type)
		{
			case 1: // Move Reg to register
					next_ptr=build_mov_register_to_register(inst_size, REG1 ,REG2,x86_64f, next_ptr);
					num_inst++;
					break;
			case 2: // 	Move Immediate to register 
					next_ptr=build_mov_immediate_to_register(inst_size, 0,immediate ,REG1,x86_64f, next_ptr);
					num_inst++;
					break;
			case 3:  //Move Register to memory
					next_ptr=build_mov_register_to_memory(inst_size,addr32, REG1,REG_EAX,disp,mem_displacement, next_ptr);
					num_inst++;
					break;
			case 4: // Move memory to register
					next_ptr=build_mov_memory_to_register(inst_size,addr32, REG1 ,REG_EAX,disp,mem_displacement, next_ptr);
					num_inst++;
					break;
	}	
	
}

// Restore the saved registers
	next_ptr= build_pop_reg(REG_R15,1,next_ptr);
	next_ptr= build_pop_reg(REG_R14,1,next_ptr);
	next_ptr= build_pop_reg(REG_R13,1,next_ptr);
	next_ptr= build_pop_reg(REG_R12,1,next_ptr);					
	next_ptr= build_pop_reg(REG_EBX,0,next_ptr);
	
// Release the stack frame and return to the caller 	
	next_ptr= add_endi(next_ptr);
	
	return (num_inst);
}


/*
 * Function: executeit
 * 
 * Description:
 *
 * This function will start executing at the function address passed into it 
 * and return an integer return value that will be used to indicate pass(0)/fail(1)
 *
 * INTPUTs:  funct_t start_addr :      function pointer 
 *
 * Returns:  int                :      0 for pass, 1 for fail
 */   
int executeit(funct_t start_addr) 
{
	volatile int i,rc=0;
	i=0;
	rc=(*start_addr)();
	return(0);
}


/*
 * Function: rand_generator
 * 
 * Description:
 * Generates a random number within the specified range 
 *
 * INTPUTs:  
 * int min_n               : Lower limit for the range 
 * int max_n			   : Upper limit for the range
 * Returns:  random number within the specified range 
 */  

int rand_generator( int min_n,int max_n)
{
	return rand () % (max_n-min_n +1) + min_n;
	
}
