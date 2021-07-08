#include "../includes/bus.h"

uint64_t bus_load(BUS* bus, uint64_t addr, uint64_t size) {
    return dram_load(&(bus->dram), addr, size);
}
void bus_store(BUS* bus, uint64_t addr, uint64_t size, uint64_t value) {
    dram_store(&(bus->dram), addr, size, value);
}
