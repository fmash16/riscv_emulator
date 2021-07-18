#ifndef DRAM_H
#define DRAM_H

#include <stdint.h>

#define DRAM_SIZE 1024*1024*1
#define DRAM_BASE 0x80000000

typedef struct DRAM {
	uint8_t mem[DRAM_SIZE];     // Dram memory of DRAM_SIZE
} DRAM;

uint64_t dram_load(DRAM* dram, uint64_t addr, uint64_t size);
//uint64_t dram_load_8(DRAM* dram, uint64_t addr);
//uint64_t dram_load_16(DRAM* dram, uint64_t addr);
//uint64_t dram_load_32(DRAM* dram, uint64_t addr);
//uint64_t dram_load_64(DRAM* dram, uint64_t addr);

void dram_store(DRAM* dram, uint64_t addr, uint64_t size, uint64_t value);
//void dram_store_8(DRAM* dram, uint64_t addr, uint64_t value);
//void dram_store_16(DRAM* dram, uint64_t addr, uint64_t value);
//void dram_store_32(DRAM* dram, uint64_t addr, uint64_t value);
//void dram_store_64(DRAM* dram, uint64_t addr, uint64_t value);

#endif
