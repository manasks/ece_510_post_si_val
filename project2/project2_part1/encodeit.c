
//
// simple encoding example for IA-32 validation project
//

#include <stdio.h>
#include <stdlib.h>
#include "ia32_encode.h"

// globals to aid debug to start
volatile char *mptr=0,*next_ptr=0;
int num_inst=0;

main(int argc, char *argv[])
{

	int ibuilt=0;


	/* allocate buffer to build instructions into */

	mptr=malloc(MAX_INSTR_BYTES);
	next_ptr=mptr;                  // init next_ptr

	ibuilt=build_instructions();  	// build instructions
	fprintf(stderr,"generation program complete, instructions generated: %d\n",ibuilt);
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

	// Register to Register Instructions

	// 1. test move ecx into ebx
	next_ptr=build_mov_register_to_register(ISZ_4, REG_ECX ,REG_EBX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 2. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_4, REG_EDX ,REG_EDI, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 3. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_2, REG_ECX ,REG_EDI, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 4. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_2, REG_EAX ,REG_EDX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 5. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_4, REG_EBX ,REG_EAX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 6. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_1, REG_AL ,REG_BL, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 7. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_1, REG_DH ,REG_CH, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 8. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_2, REG_EDX ,REG_ECX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 9. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_2, REG_EDX ,REG_EDI, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 10. test move edx into edi
	next_ptr=build_mov_register_to_register(ISZ_1, REG_AL ,REG_BL, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

 
	// Immediate to Register Instructions

	// 1. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x1111 ,REG_EAX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 2. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x7501 ,REG_ECX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 3. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x1378 ,REG_EDX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 4. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x8721 ,REG_EBX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 5. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x28 ,REG_AL, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 6. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x82 ,REG_CL, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 7. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x37 ,REG_DH, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 8. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x92 ,REG_BH, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 9. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x1334 ,REG_ECX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 10. test move ecx into ebx
	next_ptr=build_mov_immediate_to_register(ISZ_4, 0x56 ,REG_DL, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);


	// Register tp Memory Instructions

	// 1. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_1, REG_EAX, 0x0 ,REG_ECX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 2. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_1, REG_EDX, 0x0 ,REG_EBX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 3. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_2, REG_EAX, 0x12 ,REG_EDX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 4. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_2, REG_EDX, 0x49 ,REG_ECX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 5. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_4, REG_EAX, 0x1782 ,REG_EBX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 6. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_4, REG_ECX, 0x8753 ,REG_EAX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 7. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_1, REG_ECX, 0x0 ,REG_EAX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 8. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_2, REG_EBX, 0x0 ,REG_EAX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 9. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_4, REG_EDX, 0x0 ,REG_EAX, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);

	// 10. test move ecx into ebx
	next_ptr=build_mov_register_to_memory(ISZ_1, REG_EAX, 0x0 ,REG_EDI, next_ptr);
	num_inst++;
	fprintf(stderr,"next ptr is now 0x%lx\n", (long) next_ptr);




	return (num_inst);
}
