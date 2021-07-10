#include "../includes/csr.h"
#include <stdint.h>

uint64_t csr_read(CPU* cpu, uint64_t csr) {
    return (uint64_t)(uint32_t)cpu->csr[csr];
}

void csr_write(CPU* cpu, uint64_t csr, uint64_t value) {
    cpu->csr[csr] = value;
}
