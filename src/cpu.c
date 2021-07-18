#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../includes/cpu.h"
#include "../includes/opcodes.h"
#include "../includes/csr.h"

#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[31m"
#define ANSI_RESET   "\x1b[0m"

#define ADDR_MISALIGNED(addr) (addr & 0x3)


// print operation for DEBUG
void print_op(char* s) {
    printf("%s%s%s", ANSI_BLUE, s, ANSI_RESET);
}

void cpu_init(CPU *cpu) {
    cpu->regs[0] = 0x00;                    // register x0 hardwired to 0
    cpu->regs[2] = DRAM_BASE + DRAM_SIZE;   // Set stack pointer
    cpu->pc      = DRAM_BASE;               // Set program counter to the base address
}

uint32_t cpu_fetch(CPU *cpu) {
    uint32_t inst = bus_load(&(cpu->bus), cpu->pc, 32);
    return inst;
}

uint64_t cpu_load(CPU* cpu, uint64_t addr, uint64_t size) {
    return bus_load(&(cpu->bus), addr, size);
}

void cpu_store(CPU* cpu, uint64_t addr, uint64_t size, uint64_t value) {
    bus_store(&(cpu->bus), addr, size, value);
}

//=====================================================================================
// Instruction Decoder Functions
//=====================================================================================

uint64_t rd(uint32_t inst) {
    return (inst >> 7) & 0x1f;    // rd in bits 11..7
}
uint64_t rs1(uint32_t inst) {
    return (inst >> 15) & 0x1f;   // rs1 in bits 19..15
}
uint64_t rs2(uint32_t inst) {
    return (inst >> 20) & 0x1f;   // rs2 in bits 24..20
}

uint64_t imm_I(uint32_t inst) {
    // imm[11:0] = inst[31:20]
    return ((int64_t)(int32_t) (inst & 0xfff00000)) >> 20; // right shift as signed?
}
uint64_t imm_S(uint32_t inst) {
    // imm[11:5] = inst[31:25], imm[4:0] = inst[11:7]
    return ((int64_t)(int32_t)(inst & 0xfe000000) >> 20)
        | ((inst >> 7) & 0x1f); 
}
uint64_t imm_B(uint32_t inst) {
    // imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
    return ((int64_t)(int32_t)(inst & 0x80000000) >> 19)
        | ((inst & 0x80) << 4) // imm[11]
        | ((inst >> 20) & 0x7e0) // imm[10:5]
        | ((inst >> 7) & 0x1e); // imm[4:1]
}
uint64_t imm_U(uint32_t inst) {
    // imm[31:12] = inst[31:12]
    return (int64_t)(int32_t)(inst & 0xfffff999);
}
uint64_t imm_J(uint32_t inst) {
    // imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
    return (uint64_t)((int64_t)(int32_t)(inst & 0x80000000) >> 11)
        | (inst & 0xff000) // imm[19:12]
        | ((inst >> 9) & 0x800) // imm[11]
        | ((inst >> 20) & 0x7fe); // imm[10:1]
}

uint32_t shamt(uint32_t inst) {
    // shamt(shift amount) only required for immediate shift instructions
    // shamt[4:5] = imm[5:0]
    return (uint32_t) (imm_I(inst) & 0x1f); // TODO: 0x1f / 0x3f ?
}

uint64_t csr(uint32_t inst) {
    // csr[11:0] = inst[31:20]
    return ((inst & 0xfff00000) >> 20);
}

//=====================================================================================
//   Instruction Execution Functions
//=====================================================================================

void exec_LUI(CPU* cpu, uint32_t inst) {
    // LUI places upper 20 bits of U-immediate value to rd
    cpu->regs[rd(inst)] = (uint64_t)(int64_t)(int32_t)(inst & 0xfffff000);
    print_op("lui\n");
}

void exec_AUIPC(CPU* cpu, uint32_t inst) {
    // AUIPC forms a 32-bit offset from the 20 upper bits 
    // of the U-immediate
    uint64_t imm = imm_U(inst);
    cpu->regs[rd(inst)] = ((int64_t) cpu->pc + (int64_t) imm) - 4;
    print_op("auipc\n");
}

void exec_JAL(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_J(inst);
    cpu->regs[rd(inst)] = cpu->pc;
    /*print_op("JAL-> rd:%ld, pc:%lx\n", rd(inst), cpu->pc);*/
    cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("jal\n");
    if (ADDR_MISALIGNED(cpu->pc)) {
        fprintf(stderr, "JAL pc address misalligned");
        exit(0);
    }
}

void exec_JALR(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    uint64_t tmp = cpu->pc;
    cpu->pc = (cpu->regs[rs1(inst)] + (int64_t) imm) & 0xfffffffe;
    cpu->regs[rd(inst)] = tmp;
    /*print_op("NEXT -> %#lx, imm:%#lx\n", cpu->pc, imm);*/
    print_op("jalr\n");
    if (ADDR_MISALIGNED(cpu->pc)) {
        fprintf(stderr, "JAL pc address misalligned");
        exit(0);
    }
}

void exec_BEQ(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] == (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("beq\n");
}
void exec_BNE(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] != (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = (cpu->pc + (int64_t) imm - 4);
    print_op("bne\n");
}
void exec_BLT(CPU* cpu, uint32_t inst) {
    /*print_op("Operation: BLT\n");*/
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] < (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("blt\n");
}
void exec_BGE(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] >= (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("bge\n");
}
void exec_BLTU(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if (cpu->regs[rs1(inst)] < cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("bltu\n");
}
void exec_BGEU(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if (cpu->regs[rs1(inst)] >= cpu->regs[rs2(inst)])
        cpu->pc = (int64_t) cpu->pc + (int64_t) imm - 4;
    print_op("jal\n");
}
void exec_LB(CPU* cpu, uint32_t inst) {
    // load 1 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = (int64_t)(int8_t) cpu_load(cpu, addr, 8);
    print_op("lb\n");
}
void exec_LH(CPU* cpu, uint32_t inst) {
    // load 2 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = (int64_t)(int16_t) cpu_load(cpu, addr, 16);
    print_op("lh\n");
}
void exec_LW(CPU* cpu, uint32_t inst) {
    // load 4 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = (int64_t)(int32_t) cpu_load(cpu, addr, 32);
    print_op("lw\n");
}
void exec_LD(CPU* cpu, uint32_t inst) {
    // load 8 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = (int64_t) cpu_load(cpu, addr, 64);
    print_op("ld\n");
}
void exec_LBU(CPU* cpu, uint32_t inst) {
    // load unsigned 1 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = cpu_load(cpu, addr, 8);
    print_op("lbu\n");
}
void exec_LHU(CPU* cpu, uint32_t inst) {
    // load unsigned 2 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = cpu_load(cpu, addr, 16);
    print_op("lhu\n");
}
void exec_LWU(CPU* cpu, uint32_t inst) {
    // load unsigned 2 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = cpu_load(cpu, addr, 32);
    print_op("lwu\n");
}
void exec_SB(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 8, cpu->regs[rs2(inst)]);
    print_op("sb\n");
}
void exec_SH(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 16, cpu->regs[rs2(inst)]);
    print_op("sh\n");
}
void exec_SW(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 32, cpu->regs[rs2(inst)]);
    print_op("sw\n");
}
void exec_SD(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 64, cpu->regs[rs2(inst)]);
    print_op("sd\n");
}

void exec_ADDI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] + (int64_t) imm;
    print_op("addi\n");
}

void exec_SLLI(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] << shamt(inst);
    print_op("slli\n");
}

void exec_SLTI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < (int64_t) imm)?1:0;
    print_op("slti\n");
}

void exec_SLTIU(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < imm)?1:0;
    print_op("sltiu\n");
}

void exec_XORI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] ^ imm;
    print_op("xori\n");
}

void exec_SRLI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] >> imm;
    print_op("srli\n");
}

void exec_SRAI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = (int32_t)cpu->regs[rs1(inst)] >> imm;
    print_op("srai\n");
}

void exec_ORI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] | imm;
    print_op("ori\n");
}

void exec_ANDI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] & imm;
    print_op("andi\n");
}

void exec_ADD(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] =
        (uint64_t) ((int64_t)cpu->regs[rs1(inst)] + (int64_t)cpu->regs[rs2(inst)]);
    print_op("add\n");
}

void exec_SUB(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] =
        (uint64_t) ((int64_t)cpu->regs[rs1(inst)] - (int64_t)cpu->regs[rs2(inst)]);
    print_op("sub\n");
}

void exec_SLL(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] << (int64_t)cpu->regs[rs2(inst)];
    print_op("sll\n");
}

void exec_SLT(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < (int64_t) cpu->regs[rs2(inst)])?1:0;
    print_op("slt\n");
}

void exec_SLTU(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < cpu->regs[rs2(inst)])?1:0;
    print_op("slti\n");
}

void exec_XOR(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] ^ cpu->regs[rs2(inst)];
    print_op("xor\n");
}

void exec_SRL(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] >> cpu->regs[rs2(inst)];
    print_op("srl\n");
}

void exec_SRA(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int32_t)cpu->regs[rs1(inst)] >> 
        (int64_t) cpu->regs[rs2(inst)];
    print_op("sra\n");
}

void exec_OR(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] | cpu->regs[rs2(inst)];
    print_op("or\n");
}

void exec_AND(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] & cpu->regs[rs2(inst)];
    print_op("and\n");
}

void exec_FENCE(CPU* cpu, uint32_t inst) {
    print_op("fence\n");
}

void exec_ECALL(CPU* cpu, uint32_t inst) {}
void exec_EBREAK(CPU* cpu, uint32_t inst) {}

void exec_ECALLBREAK(CPU* cpu, uint32_t inst) {
    if (imm_I(inst) == 0x0)
        exec_ECALL(cpu, inst);
    if (imm_I(inst) == 0x1)
        exec_EBREAK(cpu, inst);
    print_op("ecallbreak\n");
}


void exec_ADDIW(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] + (int64_t) imm;
    print_op("addiw\n");
}

// TODO
void exec_SLLIW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] <<  shamt(inst));
    print_op("slliw\n");
}
void exec_SRLIW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] >>  shamt(inst));
    print_op("srliw\n");
}
void exec_SRAIW(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] >> (uint64_t)(int64_t)(int32_t) imm);
    print_op("sraiw\n");
}
void exec_ADDW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] 
            + (int64_t) cpu->regs[rs2(inst)]);
    print_op("addw\n");
}
void exec_MULW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] 
            * (int64_t) cpu->regs[rs2(inst)]);
    print_op("mulw\n");
}
void exec_SUBW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] 
            - (int64_t) cpu->regs[rs2(inst)]);
    print_op("subw\n");
}
void exec_DIVW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] 
            / (int64_t) cpu->regs[rs2(inst)]);
    print_op("divw\n");
}
void exec_SLLW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] <<  cpu->regs[rs2(inst)]);
    print_op("sllw\n");
}
void exec_SRLW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] >>  cpu->regs[rs2(inst)]);
    print_op("srlw\n");
}
void exec_DIVUW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] / (int64_t) cpu->regs[rs2(inst)];
    print_op("divuw\n");
}
void exec_SRAW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] >>  (uint64_t)(int64_t)(int32_t) cpu->regs[rs2(inst)]);
    print_op("sraw\n");
}
void exec_REMW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int64_t)(int32_t) (cpu->regs[rs1(inst)] 
            % (int64_t) cpu->regs[rs2(inst)]);
    print_op("remw\n");
}
void exec_REMUW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] % (int64_t) cpu->regs[rs2(inst)];
    print_op("remuw\n");
}

// CSR instructions
void exec_CSRRW(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = csr_read(cpu, csr(inst));
    csr_write(cpu, csr(inst), cpu->regs[rs1(inst)]);
    print_op("csrrw\n");
}
void exec_CSRRS(CPU* cpu, uint32_t inst) {
    csr_write(cpu, csr(inst), cpu->csr[csr(inst)] | cpu->regs[rs1(inst)]);
    print_op("csrrs\n");
}
void exec_CSRRC(CPU* cpu, uint32_t inst) {
    csr_write(cpu, csr(inst), cpu->csr[csr(inst)] & !(cpu->regs[rs1(inst)]) );
    print_op("csrrc\n");
}
void exec_CSRRWI(CPU* cpu, uint32_t inst) {
    csr_write(cpu, csr(inst), rs1(inst));
    print_op("csrrwi\n");
}
void exec_CSRRSI(CPU* cpu, uint32_t inst) {
    csr_write(cpu, csr(inst), cpu->csr[csr(inst)] | rs1(inst));
    print_op("csrrsi\n");
}
void exec_CSRRCI(CPU* cpu, uint32_t inst) {
    csr_write(cpu, csr(inst), cpu->csr[csr(inst)] & !rs1(inst));
    print_op("csrrci\n");
}

// AMO_W
void exec_LR_W(CPU* cpu, uint32_t inst) {}  
void exec_SC_W(CPU* cpu, uint32_t inst) {}  
void exec_AMOSWAP_W(CPU* cpu, uint32_t inst) {}  
void exec_AMOADD_W(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp + (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoadd.w\n");
} 
void exec_AMOXOR_W(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp ^ (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoxor.w\n");
} 
void exec_AMOAND_W(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp & (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoand.w\n");
} 
void exec_AMOOR_W(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp | (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoor.w\n");
} 
void exec_AMOMIN_W(CPU* cpu, uint32_t inst) {} 
void exec_AMOMAX_W(CPU* cpu, uint32_t inst) {} 
void exec_AMOMINU_W(CPU* cpu, uint32_t inst) {} 
void exec_AMOMAXU_W(CPU* cpu, uint32_t inst) {} 

// AMO_D TODO
void exec_LR_D(CPU* cpu, uint32_t inst) {}  
void exec_SC_D(CPU* cpu, uint32_t inst) {}  
void exec_AMOSWAP_D(CPU* cpu, uint32_t inst) {}  
void exec_AMOADD_D(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp + (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoadd.w\n");
} 
void exec_AMOXOR_D(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp ^ (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoxor.w\n");
} 
void exec_AMOAND_D(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp & (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoand.w\n");
} 
void exec_AMOOR_D(CPU* cpu, uint32_t inst) {
    uint32_t tmp = cpu_load(cpu, cpu->regs[rs1(inst)], 32);
    uint32_t res = tmp | (uint32_t)cpu->regs[rs2(inst)];
    cpu->regs[rd(inst)] = tmp;
    cpu_store(cpu, cpu->regs[rs1(inst)], 32, res);
    print_op("amoor.w\n");
} 
void exec_AMOMIN_D(CPU* cpu, uint32_t inst) {} 
void exec_AMOMAX_D(CPU* cpu, uint32_t inst) {} 
void exec_AMOMINU_D(CPU* cpu, uint32_t inst) {} 
void exec_AMOMAXU_D(CPU* cpu, uint32_t inst) {} 

int cpu_execute(CPU *cpu, uint32_t inst) {
    int opcode = inst & 0x7f;           // opcode in bits 6..0
    int funct3 = (inst >> 12) & 0x7;    // funct3 in bits 14..12
    int funct7 = (inst >> 25) & 0x7f;   // funct7 in bits 31..25

    cpu->regs[0] = 0;                   // x0 hardwired to 0 at each cycle

    /*printf("%s\n%#.8lx -> Inst: %#.8x <OpCode: %#.2x, funct3:%#x, funct7:%#x> %s",*/
            /*ANSI_YELLOW, cpu->pc-4, inst, opcode, funct3, funct7, ANSI_RESET); // DEBUG*/
    printf("%s\n%#.8lx -> %s", ANSI_YELLOW, cpu->pc-4, ANSI_RESET); // DEBUG

    switch (opcode) {
        case LUI:   exec_LUI(cpu, inst); break;
        case AUIPC: exec_AUIPC(cpu, inst); break;

        case JAL:   exec_JAL(cpu, inst); break;
        case JALR:  exec_JALR(cpu, inst); break;

        case B_TYPE:
            switch (funct3) {
                case BEQ:   exec_BEQ(cpu, inst); break;
                case BNE:   exec_BNE(cpu, inst); break;
                case BLT:   exec_BLT(cpu, inst); break;
                case BGE:   exec_BGE(cpu, inst); break;
                case BLTU:  exec_BLTU(cpu, inst); break;
                case BGEU:  exec_BGEU(cpu, inst); break;
                default: ;
            } break;

        case LOAD:
            switch (funct3) {
                case LB  :  exec_LB(cpu, inst); break;  
                case LH  :  exec_LH(cpu, inst); break;  
                case LW  :  exec_LW(cpu, inst); break;  
                case LD  :  exec_LD(cpu, inst); break;  
                case LBU :  exec_LBU(cpu, inst); break; 
                case LHU :  exec_LHU(cpu, inst); break; 
                case LWU :  exec_LWU(cpu, inst); break; 
                default: ;
            } break;

        case S_TYPE:
            switch (funct3) {
                case SB  :  exec_SB(cpu, inst); break;  
                case SH  :  exec_SH(cpu, inst); break;  
                case SW  :  exec_SW(cpu, inst); break;  
                case SD  :  exec_SD(cpu, inst); break;  
                default: ;
            } break;

        case I_TYPE:  
            switch (funct3) {
                case ADDI:  exec_ADDI(cpu, inst); break;
                case SLLI:  exec_SLLI(cpu, inst); break;
                case SLTI:  exec_SLTI(cpu, inst); break;
                case SLTIU: exec_SLTIU(cpu, inst); break;
                case XORI:  exec_XORI(cpu, inst); break;
                case SRI:   
                    switch (funct7) {
                        case SRLI:  exec_SRLI(cpu, inst); break;
                        case SRAI:  exec_SRAI(cpu, inst); break;
                        default: ;
                    } break;
                case ORI:   exec_ORI(cpu, inst); break;
                case ANDI:  exec_ANDI(cpu, inst); break;
                default:
                    fprintf(stderr, 
                            "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct7:0x%x\n"
                            , opcode, funct3, funct7);
                    return 0;
            } break;

        case R_TYPE:  
            switch (funct3) {
                case ADDSUB:
                    switch (funct7) {
                        case ADD: exec_ADD(cpu, inst);
                        case SUB: exec_ADD(cpu, inst);
                        default: ;
                    } break;
                case SLL:  exec_SLL(cpu, inst); break;
                case SLT:  exec_SLT(cpu, inst); break;
                case SLTU: exec_SLTU(cpu, inst); break;
                case XOR:  exec_XOR(cpu, inst); break;
                case SR:   
                    switch (funct7) {
                        case SRL:  exec_SRL(cpu, inst); break;
                        case SRA:  exec_SRA(cpu, inst); break;
                        default: ;
                    }
                case OR:   exec_OR(cpu, inst); break;
                case AND:  exec_AND(cpu, inst); break;
                default:
                    fprintf(stderr, 
                            "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct7:0x%x\n"
                            , opcode, funct3, funct7);
                    return 0;
            } break;

        case FENCE: exec_FENCE(cpu, inst); break;

        case I_TYPE_64:
            switch (funct3) {
                case ADDIW: exec_ADDIW(cpu, inst); break;
                case SLLIW: exec_SLLIW(cpu, inst); break;
                case SRIW : 
                    switch (funct7) {
                        case SRLIW: exec_SRLIW(cpu, inst); break;
                        case SRAIW: exec_SRLIW(cpu, inst); break;
                    } break;
            } break;

        case R_TYPE_64:
            switch (funct3) {
                case ADDSUB:
                    switch (funct7) {
                        case ADDW:  exec_ADDW(cpu, inst); break;
                        case SUBW:  exec_SUBW(cpu, inst); break;
                        case MULW:  exec_MULW(cpu, inst); break;
                    } break;
                case DIVW:  exec_DIVW(cpu, inst); break;
                case SLLW:  exec_SLLW(cpu, inst); break;
                case SRW:
                    switch (funct7) {
                        case SRLW:  exec_SRLW(cpu, inst); break;
                        case SRAW:  exec_SRAW(cpu, inst); break;
                        case DIVUW: exec_DIVUW(cpu, inst); break;
                    } break;
                case REMW:  exec_REMW(cpu, inst); break;
                case REMUW: exec_REMUW(cpu, inst); break;
                default: ;
            } break;

        case CSR:
            switch (funct3) {
                case ECALLBREAK:    exec_ECALLBREAK(cpu, inst); break;
                case CSRRW  :  exec_CSRRW(cpu, inst); break;  
                case CSRRS  :  exec_CSRRS(cpu, inst); break;  
                case CSRRC  :  exec_CSRRC(cpu, inst); break;  
                case CSRRWI :  exec_CSRRWI(cpu, inst); break; 
                case CSRRSI :  exec_CSRRSI(cpu, inst); break; 
                case CSRRCI :  exec_CSRRCI(cpu, inst); break; 
                default:
                    fprintf(stderr, 
                            "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct7:0x%x\n"
                            , opcode, funct3, funct7);
                    return 0;
            } break;

        case AMO_W:
            switch (funct7 >> 2) { // since, funct[1:0] = aq, rl
                case LR_W      :  exec_LR_W(cpu, inst); break;  
                case SC_W      :  exec_SC_W(cpu, inst); break;  
                case AMOSWAP_W :  exec_AMOSWAP_W(cpu, inst); break;  
                case AMOADD_W  :  exec_AMOADD_W(cpu, inst); break; 
                case AMOXOR_W  :  exec_AMOXOR_W(cpu, inst); break; 
                case AMOAND_W  :  exec_AMOAND_W(cpu, inst); break; 
                case AMOOR_W   :  exec_AMOOR_W(cpu, inst); break; 
                case AMOMIN_W  :  exec_AMOMIN_W(cpu, inst); break; 
                case AMOMAX_W  :  exec_AMOMAX_W(cpu, inst); break; 
                case AMOMINU_W :  exec_AMOMINU_W(cpu, inst); break; 
                case AMOMAXU_W :  exec_AMOMAXU_W(cpu, inst); break; 
                default:
                    fprintf(stderr, 
                            "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct7:0x%x\n"
                            , opcode, funct3, funct7);
                    return 0;
            } break;

        case 0x00:
            return 0;

        default:
            fprintf(stderr, 
                    "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct3:0x%x\n"
                    , opcode, funct3, funct7);
            return 0;
            /*exit(1);*/
    }
    return 1;
}

void dump_registers(CPU *cpu) {
    char* abi[] = { // Application Binary Interface registers
        "zero", "ra",  "sp",  "gp",
          "tp", "t0",  "t1",  "t2",
          "s0", "s1",  "a0",  "a1",
          "a2", "a3",  "a4",  "a5",
          "a6", "a7",  "s2",  "s3",
          "s4", "s5",  "s6",  "s7",
          "s8", "s9", "s10", "s11",
          "t3", "t4",  "t5",  "t6",
    };

    /*for (int i=0; i<8; i++) {*/
        /*printf("%4s| x%02d: %#-8.2lx\t", abi[i],    i,    cpu->regs[i]);*/
        /*printf("%4s| x%02d: %#-8.2lx\t", abi[i+8],  i+8,  cpu->regs[i+8]);*/
        /*printf("%4s| x%02d: %#-8.2lx\t", abi[i+16], i+16, cpu->regs[i+16]);*/
        /*printf("%4s| x%02d: %#-8.2lx\n", abi[i+24], i+24, cpu->regs[i+24]);*/
    /*}*/

    for (int i=0; i<8; i++) {
        printf("   %4s: %#-13.2lx  ", abi[i],    cpu->regs[i]);
        printf("   %2s: %#-13.2lx  ", abi[i+8],  cpu->regs[i+8]);
        printf("   %2s: %#-13.2lx  ", abi[i+16], cpu->regs[i+16]);
        printf("   %3s: %#-13.2lx\n", abi[i+24], cpu->regs[i+24]);
    }
}

