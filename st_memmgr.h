#ifndef _ST_MEMMGR_H_
#define _ST_MEMMGR_H_
#include<stdint.h>
#define BANK_0_SIZE (64 * 1)
#define BANK_1_SIZE (64 * 42)

#define TOTAL_SIZE (BANK_0_SIZE + BANK_1_SIZE)

#define MEM_REQUEST_SMALL 16
#define BLOCK_SIZE 2


typedef struct MEMORY_BANKS{
	uint8_t BANK_0[BANK_0_SIZE]; //upto MEM_REQUEST_SMALL size bytes
	uint8_t BANK_1[BANK_1_SIZE]; //upto MEM_REQUEST_MEDIUM size bytes
}__attribute__((packed)) MEMORY_BANKS;

typedef uint16_t MEMORY_DESCRIPTOR;

/*===========================HELPER MACRO DEFINES================================*/
#define IS_ALLOCATED(mem_desc) ((uint16_t)(mem_desc & (uint16_t)0x8000))
#define GET_BLOCK_OFFSET(mem_desc) ((uint16_t)(mem_desc & (uint16_t)0x7fff))
#define SET_ALLOCATED(mem_desc) (mem_desc|=(uint16_t)0x8000)
#define CLEAR_ALLOCATED(mem_desc) (mem_desc&=(uint16_t)0x7fff)
#define SET_BLOCK_OFFSET(mem_desc,offset) (mem_desc=(mem_desc & (uint16_t)0x8000) | (((uint16_t)0x7fff)&((uint16_t)offset)))
#define NEXT_MEMORY_DESCRIPTOR(mem_desc_ptr) ((MEMORY_DESCRIPTOR*)(((uint8_t*)mem_desc_ptr)+(GET_BLOCK_OFFSET((*mem_desc_ptr))*BLOCK_SIZE)))

/*clear up the entire managed dynamic memory space, makes the entire memory available for use*/
void st_memmgr_clear();

/*attempts to allocate the requested number bytes, as a contigous chunk of memory, returns the pointer to the first byte on success
* or returns NULL on failure*/
void* st_memmgr_alloc(uint16_t size);	

/*Frees the a previously allocated memory location*/
void st_memmgr_free(void* ptr);

/*prints the memory status*/
void st_memmgr_print_stat();

#endif
