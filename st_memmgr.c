#include"st_memmgr.h"
#include<string.h>
#include<stdio.h>
#define MAX_ATTEMPTS 3

static struct MEMORY_BANKS st_memory;
static uint16_t peak_memory_utilization;
static uint16_t actual_memory_utilization;
static uint16_t lastRequestSize;
static uint8_t lastRequestStatus;

void printByteToBinary(uint8_t byte){
	uint8_t i;
	for(i=0;i<8;i++)
		printf("%d",((byte>>(7-i))&0x01)?1:0);
}

uint16_t calculateBlockOffset(uint16_t size){
	return (size % BLOCK_SIZE)?(((size - (size % BLOCK_SIZE))/BLOCK_SIZE)+1):(size / BLOCK_SIZE);
}

void printMemoryDescriptorInfo(MEMORY_DESCRIPTOR mem_desc){
	printf("\n+--------+--------+");
	printf("\n|");
	printByteToBinary(*(((uint8_t*)&mem_desc)+1));
	printf("|");
	printByteToBinary(*(((uint8_t*)&mem_desc)));
	printf("|[block status : %s][block size = %d bytes]",IS_ALLOCATED(mem_desc)?"allocated":"free",GET_BLOCK_OFFSET(mem_desc)*BLOCK_SIZE);
	printf("\n+--------+--------+");
}

/*clear up the entire managed dynamic memory space, makes the entire memory available for use*/
void st_memmgr_clear(){
	MEMORY_DESCRIPTOR *mem_desc;
	/*CLEAR ALL*/
	memset(&st_memory,0,sizeof(struct MEMORY_BANKS));

	/*BANK_0*/	
	mem_desc=(MEMORY_DESCRIPTOR*)st_memory.BANK_0;
	SET_BLOCK_OFFSET((*mem_desc),calculateBlockOffset(BANK_0_SIZE));

	/*BANK_1*/	
	mem_desc=(MEMORY_DESCRIPTOR*)st_memory.BANK_1;
	SET_BLOCK_OFFSET((*mem_desc),calculateBlockOffset(BANK_1_SIZE));

	peak_memory_utilization=0;
	actual_memory_utilization=0;
	lastRequestSize=0;
	lastRequestStatus=0;
}

/*attempts to allocate the requested number bytes, as a contigous chunk of memory, returns the pointer to the first byte on success
* or returns NULL on failure*/
void* st_memmgr_alloc(uint16_t size){
	MEMORY_DESCRIPTOR *mem_desc;
	uint8_t bank_visit_counter=0;
	uint16_t i;
	uint16_t cont_free_block_size;
	void* return_addr=NULL;

	size+=sizeof(MEMORY_DESCRIPTOR);	//account for the memory descriptor

	if(size > BANK_1_SIZE)	//if size is greater than the size of BANK_1 then allocation will fail
		goto RETURN_ALLOCATE;
	if(size<=MEM_REQUEST_SMALL)
		goto MEMORY_BANK_0;
	else
		goto MEMORY_BANK_1;
MEMORY_BANK_0:;
	bank_visit_counter++;
	if(bank_visit_counter==MAX_ATTEMPTS)	//all banks visited
		goto RETURN_ALLOCATE;
	i=0;
	cont_free_block_size=0;
	mem_desc=(MEMORY_DESCRIPTOR*)st_memory.BANK_0;	//start of BANK_0

	while(i<BANK_0_SIZE){	//search till the end of BANK_0 boundary
		if(IS_ALLOCATED((*mem_desc))){	//this bank is allocated
			i+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
			cont_free_block_size=0;
			continue;
		}else{
			cont_free_block_size+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			if(cont_free_block_size >= size)
				break; //found a free block large enough to satisfy memory request
			i+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
		}
	}

	if(i>=BANK_0_SIZE)	//failed to satisfy request in MEMORY_BANK_0
		goto MEMORY_BANK_1;

	/*free block found, allocate whatever is required and make the rest space as a free block*/
	if(calculateBlockOffset(size) < calculateBlockOffset(cont_free_block_size)){
		mem_desc=(MEMORY_DESCRIPTOR*)(((uint8_t*)NEXT_MEMORY_DESCRIPTOR(mem_desc))-cont_free_block_size);
		SET_ALLOCATED((*mem_desc));
		SET_BLOCK_OFFSET((*mem_desc),calculateBlockOffset(size));
		return_addr=((void*)mem_desc)+sizeof(MEMORY_DESCRIPTOR);	//actual data to be stored here

		/*make a new free block*/
		mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
		CLEAR_ALLOCATED((*mem_desc));
		SET_BLOCK_OFFSET((*mem_desc), (calculateBlockOffset(cont_free_block_size)-calculateBlockOffset(size)));
	}else{		
		mem_desc=(MEMORY_DESCRIPTOR*)(((uint8_t*)NEXT_MEMORY_DESCRIPTOR(mem_desc))-cont_free_block_size);
		SET_ALLOCATED((*mem_desc));
		SET_BLOCK_OFFSET((*mem_desc),calculateBlockOffset(size));
		return_addr=((void*)mem_desc)+sizeof(MEMORY_DESCRIPTOR);	//actual data to be stored here
	}
	goto RETURN_ALLOCATE;
MEMORY_BANK_1:;
	bank_visit_counter++;
	if(bank_visit_counter==MAX_ATTEMPTS)	//all banks visited
		goto RETURN_ALLOCATE;
	i=0;
	cont_free_block_size=0;
	mem_desc=(MEMORY_DESCRIPTOR*)st_memory.BANK_1;	//start of BANK_1

	while(i<BANK_1_SIZE){	//search till the end of BANK_0 boundary
		if(IS_ALLOCATED((*mem_desc))){	//this bank is allocated
			i+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
			cont_free_block_size=0;
			continue;
		}else{
			cont_free_block_size+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			if(cont_free_block_size >= size)
				break; //found a free block large enough to satisfy memory request
			i+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
		}
	}

	if(i>=BANK_1_SIZE)	//failed to satisfy request in MEMORY_BANK_1
		goto MEMORY_BANK_0;

	/*free block found, allocate whatever is required and make the rest space as a free block*/
	if(calculateBlockOffset(size) < calculateBlockOffset(cont_free_block_size)){
		mem_desc=(MEMORY_DESCRIPTOR*)(((uint8_t*)NEXT_MEMORY_DESCRIPTOR(mem_desc))-cont_free_block_size);
		SET_ALLOCATED((*mem_desc));
		SET_BLOCK_OFFSET((*mem_desc),calculateBlockOffset(size));
		return_addr=((void*)mem_desc)+sizeof(MEMORY_DESCRIPTOR);	//actual data to be stored here
		/*make a new free block*/
		mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
		CLEAR_ALLOCATED((*mem_desc));
		SET_BLOCK_OFFSET((*mem_desc), (calculateBlockOffset(cont_free_block_size)-calculateBlockOffset(size)));
	}else{		
		mem_desc=(MEMORY_DESCRIPTOR*)(((uint8_t*)NEXT_MEMORY_DESCRIPTOR(mem_desc))-cont_free_block_size);
		SET_ALLOCATED((*mem_desc));
		SET_BLOCK_OFFSET((*mem_desc),calculateBlockOffset(size));
		return_addr=((void*)mem_desc)+sizeof(MEMORY_DESCRIPTOR);	//actual data to be stored here
	}
	goto RETURN_ALLOCATE;

RETURN_ALLOCATE:;
	lastRequestSize=size;
	if(return_addr!=NULL){
		actual_memory_utilization+=size;
		lastRequestStatus=1;
		if(actual_memory_utilization>peak_memory_utilization)
			peak_memory_utilization=actual_memory_utilization;
	}else
		lastRequestStatus=0;
	return return_addr;
}	

/*Frees the a previously allocated memory location*/
void st_memmgr_free(void* ptr){
	ptr=ptr-sizeof(MEMORY_DESCRIPTOR);
	CLEAR_ALLOCATED((*((MEMORY_DESCRIPTOR*)ptr)));
	actual_memory_utilization-=((GET_BLOCK_OFFSET(*((MEMORY_DESCRIPTOR*)ptr)))*BLOCK_SIZE);
}

/*prints the memory status*/
void st_memmgr_print_stat(){
	uint16_t i;
	MEMORY_DESCRIPTOR *mem_desc;
	uint16_t free_space=0;
	uint16_t allocated_space=0;
	uint8_t allocation_overhead=0;
	uint16_t largest_free_block=0;
	uint16_t cont_free_space=0;
	/*BANK_0*/
	i=0;
	mem_desc=(MEMORY_DESCRIPTOR*)st_memory.BANK_0;
	printf("\n\n======================BANK 0==========================\n");
	while(i<BANK_0_SIZE){
		i+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
		printMemoryDescriptorInfo((*mem_desc));
		if(IS_ALLOCATED((*mem_desc))){
			allocated_space+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			allocation_overhead+=sizeof(MEMORY_DESCRIPTOR);
			cont_free_space=0;
		}
		else{	
			free_space+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			cont_free_space+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			if(largest_free_block < cont_free_space)
				largest_free_block=cont_free_space;
		}
		mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
	}



	/*BANK_1*/
	i=0;
	mem_desc=(MEMORY_DESCRIPTOR*)st_memory.BANK_1;
	printf("\n\n======================BANK 1==========================\n");
	while(i<BANK_1_SIZE){
		i+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
		printMemoryDescriptorInfo((*mem_desc));
		if(IS_ALLOCATED((*mem_desc))){
			allocated_space+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			allocation_overhead+=sizeof(MEMORY_DESCRIPTOR);
			cont_free_space=0;
		}
		else{	
			free_space+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			cont_free_space+=(GET_BLOCK_OFFSET((*mem_desc))*BLOCK_SIZE);
			if(largest_free_block < cont_free_space)
				largest_free_block=cont_free_space;

		}

		mem_desc=NEXT_MEMORY_DESCRIPTOR(mem_desc);
	}

	printf("\n=================================================================================================");
	printf("\nTotal Allocated Space : %d bytes",allocated_space);
	printf("\nTotal Free Space : %d bytes",free_space);
	printf("\nCurrent Allocation Overhead : %d bytes",allocation_overhead);
	printf("\nActual Allocation : %d bytes approx. (Internal Fragmentation taken into account)",allocated_space-allocation_overhead);
	printf("\nPeak Memory Utilization : %d bytes",peak_memory_utilization);
	printf("\nLargest Serviceable memory : %d bytes",largest_free_block);
	printf("\nlast requested size : %d bytes",lastRequestSize);
	printf("\nlast request status : %s\n",lastRequestStatus?"SUCCESS":"FAILED");
	printf("=================================================================================================\n");

}

