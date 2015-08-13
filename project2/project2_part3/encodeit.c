
//
// simple encoding example for IA-32 validation project
//
// Project Part3: added multiple processing support
//
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <limits.h>    /* for PAGESIZE */
#include <sched.h>

#include "ia32_encode.h"

// globals to aid debug to start
volatile char *mptr=0,*next_ptr=0,*mdptr=0, *comm_ptr=0;
int num_inst=0,i=0;
int target_ninstrs=MAX_DEF_INSTRS;
int nthreads,pid_task[MAX_THREADS],pid=0;
int temp=0;
typedef struct { 
	volatile unsigned long *pointer_addr;
} test_i;

test_i test_info [NUM_PTRS];
//
// declarations for holding thread information
//
typedef volatile unsigned long *thread_ptrs;

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

long data_addr;    // starting addr of data buffer in memory
FILE *fp;			// file pointer to dump out log file 
main(int argc, char *argv[])
{
	int seed,inst_count;
	int ibuilt=0;
	char *filename;
		
	/* process arguments here */
	if (argc!=5 ) {
	fprintf(stderr," Command line arguments error: Expecting seed # ,instruction count,no of threads and file name of the log file\n  "); 
	exit(1); 
	}
		
	// read the command line arguments 
	seed=atoi(argv[1]);				// seed number
	inst_count= atoi(argv[2]);      // number of random instructions to be generated 
	nthreads=atoi(argv[3]);		    //  number of threads 
	filename=argv[4];				//	name of the log file to be generated
	
	
	fp= fopen(filename, "w");
	printf("\nstarting seed = %d\n",seed);

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

	data_addr=(long)mdptr;  
	
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
	int x;
		next_ptr=(mptr+(i*MAX_INSTR_BYTES));          // init next_ptr
		fprintf(stderr,"T%d next_ptr=0x%lx\n",i,(unsigned long)next_ptr);
		mdptr_threads[i]=(thread_ptrs)(mdptr+(i*MAX_DATA_BYTES));  // init threads data pointer
		mptr_threads[i]=(thread_ptrs)next_ptr;                     // save ptr per thread
		comm_ptr_threads[i]=(thread_ptrs)comm_ptr;                 // everyone gets the same for now


		/* use fork to start a new child process */

	if((pid=fork()) == 0) {
			
		fprintf(stderr,"T%d fork\n",i);
		fflush(stderr);
		//Bind the thread to a specific CPU
			x= bind_to_CPU(i,getpid());
			//printf("PID=%d\n\n",getpid());
			if( x!=0) {

			exit(1);
			}
							
		//Start building instructions
		ibuilt=build_instructions(mptr_threads[i],i,inst_count,seed);  // build instructions

		/* ok now that I built the critters, time to execute them */
		start_test=(funct_t) mptr_threads[i];
		executeit(start_test);
		fprintf(stderr,"T%d generation program complete, instructions generated: %d and Executed\n",i, ibuilt);
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

	fclose(fp);				// close the log file 

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
int build_instructions(volatile char *next_ptr, int thread_id,int inst_count,int seed) {

	int num_inst=0;
	
	fprintf(stderr, "building instructions for T%d\n",thread_id);
	fflush(stderr);

	int i;
	
	next_ptr= add_headeri(next_ptr);							//encode Enter inst
	
	// Save the registers 
	next_ptr= build_push_reg(REG_EBX,0,next_ptr);						
	next_ptr= build_push_reg(REG_EAX,0,next_ptr);						
	next_ptr= build_push_reg(REG_ECX,0,next_ptr);						
	next_ptr= build_push_reg(REG_EDX,0,next_ptr);						
	next_ptr= build_push_reg(REG_R12,1,next_ptr);						
	next_ptr= build_push_reg(REG_R13,1,next_ptr);						
	next_ptr= build_push_reg(REG_R14,1,next_ptr);						
	next_ptr= build_push_reg(REG_R15,1,next_ptr);						
	
	// set the base addr for data access from memory
	next_ptr=build_mov_immediate_to_register(ISZ_8,1, data_addr ,REG_EAX,0, next_ptr);	

	for (  i=0; i<inst_count; i++)
	{
		int inst_type,inst_size;
		int REG1,REG2; 
		int x86_64f,disp,addr32;	
		short immediate;
		short  mem_displacement;
		short lock_en;
		
		
		// Generate the randomized inputs within the specified range 
		inst_type= rand_generator(1,9);
		REG1= rand_generator(1,7);					// Generate random registers 
		REG2= rand_generator(1,7);
		x86_64f= rand_generator(0,1);				// indicates whether to use r8-r15 registers 
		inst_size= rand_generator(1,8);				// specifies the inst size
		immediate=rand_generator(20,500);			// generates random immediate value within 20 - 100
		addr32= rand_generator(0,1);				// indicates whether to use 32 bit memory address or 64 bits
		disp= rand_generator (0,2); 				// specifies DISP_0,DISP_32 or DISP_32
		mem_displacement= rand_generator (10,100);   // generates random memory displacement value within 10- 100
		lock_en= rand_generator(0,1);				 // flag to add LOCK prefix to the instruction
		
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
		if (inst_type== 3 || 4 || 5) {
			addr32=ADDR_64;
		}
				
			
		switch(inst_type)
		{
			case 1: // Move Reg to register
					next_ptr=build_mov_register_to_register(inst_size, REG1 ,REG2,x86_64f, next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d: Build instruction MOV REG to reg:Inst_size=%d\t src_reg=%d\t dest_reg=%d\t\t\t",thread_id,inst_size,REG1,REG2);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;
			case 2: // 	Move Immediate to register 
					next_ptr=build_mov_immediate_to_register(inst_size, 0,immediate ,REG1,x86_64f, next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction MOV Immediate to reg:Inst_size=%d\t Immediate value =%d\t dest_reg=%d\t\t", thread_id,inst_size,immediate,REG1);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;
			case 3: //Move Register to memory
					next_ptr=build_mov_register_to_memory(inst_size,addr32, REG1,REG_EAX,disp,mem_displacement, next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction MOV Register to Memory :Inst_size=%d\t Src_reg =%d\t mem_reg=%d\tDisplacement=%d \t", thread_id,inst_size,REG1,REG_EAX,mem_displacement);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;
			case 4: // Move memory to register
					next_ptr=build_mov_memory_to_register(inst_size,addr32, REG1 ,REG_EAX,disp,mem_displacement, next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction MOV Memory to Register :Inst_size=%d\t mem_reg =%d\t dest_reg=%d\tDisplacement=%d \t",thread_id, inst_size,REG1,REG_EAX,mem_displacement);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;
			case 5: // XADD
					next_ptr=build_xadd(inst_size, addr32, REG1,REG_EAX, x86_64f,lock_en, next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction XADD :Inst_size=%d\t scr_reg =%d\t dest_reg=%d\tLOCK enable=%d\t\t",thread_id, inst_size,REG1,REG_EAX,lock_en);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;
			case 6: // XCHG
					next_ptr=build_xchg(inst_size, addr32, REG1,REG_EAX, x86_64f,lock_en, next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction XCHG :Inst_size=%d\t scr_reg =%d\t dest_reg=%d\tLOCK enable=%d\t\t", thread_id,inst_size,REG1,REG_EAX,lock_en);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;
			case 7:
					next_ptr=build_mfence(next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction MFENCE\t\t",thread_id);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;

			case 8:
					next_ptr=build_lfence(next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction LFENCE \t\t",thread_id);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;

			case 9:
					next_ptr=build_sfence(next_ptr);
					num_inst++;
					fprintf(fp,"Thread %d:Build instruction SFENCE \t\t",thread_id);
					fprintf(fp,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fprintf(stderr,"Thread %d:\t\tnext ptr is now 0x%lx\n",thread_id, (long) next_ptr);
					fflush(stderr);
					break;



	}	
 
}

// Restore the saved registers
	next_ptr= build_pop_reg(REG_R15,1,next_ptr);
	next_ptr= build_pop_reg(REG_R14,1,next_ptr);
	next_ptr= build_pop_reg(REG_R13,1,next_ptr);
	next_ptr= build_pop_reg(REG_R12,1,next_ptr);					
	next_ptr= build_pop_reg(REG_EDX,0,next_ptr);
	next_ptr= build_pop_reg(REG_ECX,0,next_ptr);
	next_ptr= build_pop_reg(REG_EAX,0,next_ptr);
	next_ptr= build_pop_reg(REG_EBX,0,next_ptr);
	
// Release the stack frame and return to the caller 	
	next_ptr= add_endi(next_ptr);
	return (num_inst);
}


/*
 * Function: bind_to_CPU
 * 
 * Description:
 * Binds the thread to a particular CPU 
 *
 * INTPUTs:  
 * int i               : Thread ID
 * int pid			   : Process ID of the thread
 * Returns: Int		   : 0 for success, -1 for fail 
 */ 
int bind_to_CPU(int i,int pid)
{	
		cpu_set_t mask;
		
        CPU_ZERO( &mask );		//initializes all the bits in the mask to zero
		
        CPU_SET( i, &mask );	//sets only the bit corresponding to cpu.
		
		if( sched_setaffinity( pid, sizeof(mask), &mask ) == -1 )	
		{
	      fprintf(fp,"ERROR: Unable to set CPU Affinity.\n");
		  return(-1);
		}
		else {
		fprintf(fp," Assigned child process T%d to CPU_%d\n",pid,i);
		return(0);
}	
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



