#ifndef SIM_HPP
#define SIM_HPP

#include "common.hpp"
#include <cstdint>

struct instr_t {
    unsigned op: 7;
    union {
        struct {
            unsigned rd: 5;
            unsigned f3: 3;
            unsigned rs1: 5;
            unsigned rs2: 5;
            unsigned f7: 7;
        } rtype;
        struct {
            unsigned rd: 5;
            unsigned f3: 3;
            unsigned rs1: 5;
            unsigned imm: 32;
        } itype;
        struct {
            unsigned f3: 3;
            unsigned rs1: 5;
            unsigned rs2: 5;
            unsigned imm: 32;
        } stype;
        struct {
            unsigned rd: 5;
            unsigned imm: 32;
        } utype;
    } as;

    instr_t(uint32_t word) {
        op = word & ONES(7);
    }
};

class simulator {
    uint8_t *mem;
    uint32_t pc;
    uint32_t prev_pc;
    uint32_t reg[32];

    uint32_t fetch();
    void run_R(instr_t instr);
    void run_I_ld(instr_t instr);
    void run_I_alu(instr_t instr);
    void run_I_jmp(instr_t instr);
    void run_S(instr_t instr);
    void run_B(instr_t instr);
    void run_U(instr_t instr);
    void run_J(instr_t instr);
    void errmsg_illegal();

    typedef void (simulator::*handler_fn)(const instr_t);
    handler_fn itype_handler[3] = {
        &simulator::run_I_ld,
        &simulator::run_I_alu,
        &simulator::run_I_jmp,
    };
public:
    simulator(uint8_t *mem) : mem(mem), pc(ADDR_BASE) {}
    void run();
};

#endif
