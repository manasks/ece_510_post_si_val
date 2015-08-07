
//
// simple encoding example for IA-32 validation project
//
// Project Part3: added multiple processing support
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <limits.h>    /* for PAGESIZE */


#include "ia32_encode.h"

// globals to aid debug to start
volatile char *mptr=0,*next_ptr=0,*mdptr=0, *comm_ptr=0;
int num_inst=0,i=0;
int target_ninstrs=MAX_DEF_INSTRS;
int nthreads=1,pid_task[MAX_THREADS],pid=0;

typedef struct { 
	volatile unsigned long *pointer_addr;
} test_i;

test_i test_info [NUM_PTRS];
//
// declarations for holding thread information
//
typedef volatile unsigned long *tptrs;

volatile unsigned long *mptr_threads[MAX_THREADS];
volatile unsigned long *mdptr_threads[MAX_THREADS];
volatile unsigned long *comm_ptr_threads[MAX_THREADS];



// 
// declarations for starting test
//
typedef int (*funct_t)();
funct_t start_test;
int executeit();

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

/*
 * simple routine to randomize numbers in a range
 */
int rand_range(int min_n, int max_n)
{
	return rand() % (max_n - min_n + 1) + min_n;
}


main(int argc, char *argv[])
{
	int seed=0;
	int ibuilt=0;



	/* process arguments here */

	printf("\nstarting seed = %d\n",seed);

	/* need number of instructions */

	/* need to pass in # of threads */


	if (nthreads > MAX_THREADS) {
		fprintf(stderr,"Sorry only built for %d threads over riding your %d\n", MAX_THREADS, nthreads);
		fflush(stderr);
		nthreads=MAX_THREADS;
	}

	srand(seed);

	/* allocate buffer to perform stores and loads to  */


	test_info[DATA].pointer_addr = mmap(
		(void *) 0,
		(MAX_DATA_BYTES+PAGESIZE-1) * nthreads,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED,
		0, 0
		);

	/* save the base address for debug like before */

	mdptr=(volatile char *)test_info[DATA].pointer_addr;

	if (((int *)test_info[DATA].pointer_addr) == (int *)-1) {
		perror("Couldn't mmap (MAX_DATA_BYTES)");
		exit(1);
	}


	
	/* allocate buffer to build instructions into */

	test_info[CODE].pointer_addr = mmap(
		(void *) 0,
		(MAX_INSTR_BYTES+PAGESIZE-1) * nthreads,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_SHARED,
		0, 0
		);

	/* keep a copy to the base here */

	mptr=(volatile char *)test_info[CODE].pointer_addr;

	if (((int *)test_info[CODE].pointer_addr) == (int *)-1) {
		perror("Couldn't mmap (MAX_INSTR_BYTES)");
		exit(1);
	}


	/* allocate buffer to build communications area into */

	test_info[COMM].pointer_addr = mmap(
		(void *) 0,
		(MAX_COMM_BYTES+PAGESIZE-1) * nthreads,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_SHARED,
		0, 0
		);

	comm_ptr=(volatile char *)test_info[COMM].pointer_addr;

	if (((int *)test_info[COMM].pointer_addr) == (int *)-1) {
		perror("Couldn't mmap (MAX_INSTR_BYTES)");
		exit(1);
	}

	/* make the standard output and stderrr unbuffered */

	setbuf(stdout, (char *) NULL);
	setbuf(stderr, (char *) NULL);


	/* start appropriate # of threads */

	for (i=0;i<nthreads;i++) 
	{
	
		next_ptr=(mptr+(i*MAX_INSTR_BYTES));          // init next_ptr
		fprintf(stderr,"T%d next_ptr=0x%lx\n",i,(unsigned long)next_ptr);
		mdptr_threads[i]=(tptrs)(mdptr+(i*MAX_DATA_BYTES));  // init threads data pointer
		mptr_threads[i]=(tptrs)next_ptr;                     // save ptr per thread
		comm_ptr_threads[i]=(tptrs)comm_ptr;                 // everyone gets the same for now


		/* use fork to start a new child process */

		if((pid=fork()) == 0) {

			fprintf(stderr,"T%d fork\n",i);
			fflush(stderr);

			//
			// NOTE:  you could set your sched_setaffinity here...better to make a subroutine to bind
			// 
			//
			ibuilt=build_instructions(mptr_threads[i],i);  // build instructions

			/* ok now that I built the critters, time to execute them */

			start_test=(funct_t) mptr_threads[i];
			executeit(start_test);
			fprintf(stderr,"T%d generation program complete, instructions generated: %d\n",i, ibuilt);
			fflush(stderr);

			break;
			
		}
		
                else if (pid_task[i] == -1) {
			perror("fork me failed");
			exit(1);
		} else { // this should be the parent 

			pid_task[i]=pid; // save pid

			fprintf(stderr,"child T%d started:\n",pid);
			fflush(stderr);

		}
	     
	} // end for nthreads


	// wait for threads to complete

	for (i=0;i<nthreads;i++) {
		waitpid(pid_task[i], NULL, 0);
	}


	// clean up the allocation before getting out

	munmap((caddr_t)mdptr,(MAX_DATA_BYTES+PAGESIZE-1)*nthreads);
	munmap((caddr_t)mptr,(MAX_INSTR_BYTES+PAGESIZE-1)*nthreads);
	munmap((caddr_t)comm_ptr,(MAX_COMM_BYTES+PAGESIZE-1)*nthreads);


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
int build_instructions(volatile char *next_ptr, int thread_id) {

	int num_inst=0;

	// example instruction generation..

	fprintf(stderr, "building instructions for T%d\n",thread_id);
	fflush(stderr);

	//	next_ptr=add_headeri(next_ptr);

	// INSERT YOUR CODE HERE AND NUKE MINE :-)

	// test move ecx into ebx
	next_ptr=build_mov_register_to_register(ISZ_4, REG_ECX ,REG_EBX, next_ptr);
	num_inst++;

	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);
	fflush(stderr);

	// test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_4, REG_EDX ,REG_EDI, next_ptr);
	num_inst++;

	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);
	fflush(stderr);

	// next_ptr=add_endi(next_ptr);

	// nuke when you do your add_endit,shouldn't do this, but for lab base purposes..
	next_ptr=build_ret(next_ptr);

	return (num_inst);
}
