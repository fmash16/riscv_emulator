#ifndef OPCODES_H
#define OPCODES_H

#define LUI     0x37 
#define AUIPC   0x17 

#define JAL     0x6f 
#define JALR    0x67 

#define B_TYPE  0x63
    #define BEQ     0x0
    #define BNE     0x1
    #define BLT     0x4
    #define BGE     0x5
    #define BLTU    0x6
    #define BGEU    0x7

#define LOAD    0x03
    #define LB      0x0
    #define LH      0x1
    #define LW      0x2
    #define LD      0x3
    #define LBU     0x4
    #define LHU     0x5
    #define LWU     0x6

#define S_TYPE  0x23
    #define SB      0x0
    #define SH      0x1
    #define SW      0x2
    #define SD      0x3

#define I_TYPE  0x13
    #define ADDI    0x0
    #define SLLI    0x1
    #define SLTI    0x2
    #define SLTIU   0x3
    #define XORI    0x4
    #define SRI     0x5
        #define SRLI    0x00
        #define SRAI    0x20
    #define ORI     0x6
    #define ANDI    0x7

#define R_TYPE  0x33
    #define ADDSUB  0x0
        #define ADD     0x00
        #define SUB     0x20
    #define SLL     0x1
    #define SLT     0x2
    #define SLTU    0x3
    #define XOR     0x4
    #define SR      0x5
        #define SRL     0x00
        #define SRA     0x20
    #define OR      0x6
    #define AND     0x7

#define FENCE   0x0f

#define I_TYPE_64 0x1b
    #define ADDIW   0x0
    #define SLLIW   0x1
    #define SRIW    0x5
        #define SRLIW   0x00
        #define SRAIW   0x20

#define R_TYPE_64 0x3b
    #define ADDSUB   0x0
        #define ADDW    0x00
        #define MULW    0x01
        #define SUBW    0x20
    #define DIVW    0x4
    #define SLLW    0x1
    #define SRW     0x5
        #define SRLW   0x00
        #define DIVUW   0x01
        #define SRAW   0x20
    #define REMW    0x6
    #define REMUW   0x7

#define CSR 0x73
    #define ECALLBREAK    0x00     // contains both ECALL and EBREAK
    #define CSRRW   0x01
    #define CSRRS   0x02
    #define CSRRC   0x03
    #define CSRRWI  0x05
    #define CSRRSI  0x06
    #define CSRRCI  0x07

#endif
