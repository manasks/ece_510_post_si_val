
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

main(int argc, char *argv[])
{

	int ibuilt=0,rc=0;

	/* allocate buffer to perform stores and loads to, and set permissions  */

	mdptr = (volatile char *)mmap(
		(void *) 0,
		(MAX_DATA_BYTES+PAGESIZE-1),
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED,
		0, 0
		);

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

	ibuilt=build_instructions();  	// build instructions

	/* ok now that I built the critters, time to execute them */

	start_test=(funct_t) mptr;
	executeit(start_test);


	fprintf(stderr,"generation program complete, %d instructions generated, and executed\n",ibuilt);

	// clean up the allocation before getting out

	munmap((caddr_t)mdptr,(MAX_DATA_BYTES+PAGESIZE-1));
	munmap((caddr_t)mptr,(MAX_INSTR_BYTES+PAGESIZE-1));


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

//
// Routine:  build_instructions
//
// Description:
//
// INPUTS: none yet
// 
// OUTPUT: returns the number of instructions built
// 
int build_instructions() {

	// example instruction generation..

	// test move ecx into ebx
	next_ptr=build_mov_register_to_register(ISZ_4, REG_ECX ,REG_EBX, next_ptr);
	num_inst++;

	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);


	// test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_4, REG_EDX ,REG_EDI, next_ptr);
	num_inst++;

	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	return (num_inst);
}
