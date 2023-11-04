#include "sim.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>

namespace {
    template <uint32_t N>
    uint32_t sign_extend(uint32_t in) {
        return in | (in & 1 << N ? ONES(31 - N) << (N + 1) : 0);
    }

    inline uint32_t imm_S(uint32_t word) {
        const uint32_t s = ((word >> 20 & ~ONES(5)) |
                            (word >> 7 & ONES(5)));
        return sign_extend<11>(s);
    }

    // {        a[31:25], 13'b0, a[11:7],  7'b0 } ->
    // { 19'b0, a[31],    a[7],  a[30:25], a[11:8], 1'b0 }
    inline uint32_t imm_B(uint32_t word) {
        const uint32_t s = ((word >> 19 & 1 << 12) |
                            (word << 4 & 1 << 11) |
                            (word >> 20 & ONES(6) << 5) |
                            (word >> 7 & ONES(4) << 1));
        return sign_extend<12>(s);
    }

    // {        a[31:20], a[19:12], 12'b0 } ->
    // { 11'b0, a[31],    a[19:12], a[20], a[30:21], 1'b0 }
    inline uint32_t imm_J(uint32_t word) {
        const uint32_t s = ((word >> 11 & 1 << 20) |
                            (word & ONES(8) << 12) |
                            (word >> 9 & 1 << 11) |
                            (word >> 20 & ONES(10) << 1));
        return sign_extend<20>(s);
    }
}

uint32_t simulator::fetch() {
    const uint32_t word = *reinterpret_cast<uint32_t *>(&mem[pc & ~ADDR_BASE]);
    prev_pc = pc;
    pc = pc + 4;
    return word;
}

void simulator::run() {
    for (;;) {
        /* We maintain reg[0] == 0 by assigning it before each instruction.
         * That way, we don't have to check if the target is reg[0] before
         * every register assignment. Assignments to reg[0] that do happen
         * in the execution of the previous instruction are automatically
         * discarded before the next instruction, and it always reads 0.
         */
        reg[0] = 0;
        const uint32_t word = fetch();
        instr_t ir(word);
        switch (ir.op) {
        case 0x33:
            ir.as.rtype.rd = word >> 7 & ONES(5);
            ir.as.rtype.f3 = word >> 12 & ONES(3);
            ir.as.rtype.rs1 = word >> 15 & ONES(5);
            ir.as.rtype.rs2 = word >> 20 & ONES(5);
            ir.as.rtype.f7 = word >> 25;
            run_R(ir);
            break;
        case 0x03:
        case 0x13:
        case 0x67:
            ir.as.itype.rd = word >> 7 & ONES(5);
            ir.as.itype.f3 = word >> 12 & ONES(3);
            ir.as.itype.rs1 = word >> 15 & ONES(5);
            ir.as.itype.imm = sign_extend<11>(word >> 20);
            /* We have determined that ir.op is one of 0x03, 0x13, or 0x67.
             * These 3 opcodes do 3 different kinds of operations, although
             * they're all encoded the same way. Instead of checking once
             * more which one of these 3 it is and dispatching based on that,
             * we can keep the handlers for the 3 cases in an array and call
             * the appropriate one by mapping the 3 opcodes to indices into
             * that array. ir.op >> 4 & 0x3 does the desired mapping.
             */
            std::invoke(itype_handler[ir.op >> 4 & 0x3], *this, ir);
            break;
        case 0x23:
            ir.as.stype.f3 = word >> 12 & ONES(3);
            ir.as.stype.rs1 = word >> 15 & ONES(5);
            ir.as.stype.rs2 = word >> 20 & ONES(5);
            ir.as.stype.imm = imm_S(word);
            run_S(ir);
            break;
        case 0x63:
            ir.as.stype.f3 = word >> 12 & ONES(3);
            ir.as.stype.rs1 = word >> 15 & ONES(5);
            ir.as.stype.rs2 = word >> 20 & ONES(5);
            ir.as.stype.imm = imm_B(word);
            run_B(ir);
            break;
        case 0x17:
        case 0x37:
            ir.as.utype.rd = word >> 7 & ONES(5);
            ir.as.utype.imm = word & ~ONES(12);
            /* The 2 opcodes here -- 0x17 and 0x37, do sufficiently similar
             * things that they can be handled by the same function.
             */
            run_U(ir);
            break;
        case 0x6f:
            ir.as.utype.rd = word >> 7 & ONES(5);
            ir.as.utype.imm = imm_J(word);
            run_J(ir);
            break;
        default:
            errmsg_illegal();
            exit(1);
        }
    }
}

void simulator::run_R(instr_t ir) {
    const uint32_t rd = ir.as.rtype.rd;
    const uint32_t rs1 = ir.as.rtype.rs1;
    const uint32_t rs2 = ir.as.rtype.rs2;
    const uint32_t f3 = ir.as.rtype.f3;
    const uint32_t f7 = ir.as.rtype.f7;
    switch (f3) {
#define ROP(op) ({ reg[rd] = reg[rs1] op reg[rs2]; break; })
    case 0:
        reg[rd] = f7 & 0x20 ? reg[rs1] - reg[rs2] : reg[rs1] + reg[rs2];
        break;
    case 1:
        reg[rd] = reg[rs1] << (reg[rs2] & ONES(5));
        break;
    case 2:
        reg[rd] = static_cast<int32_t>(reg[rs1]) < static_cast<int32_t>(reg[rs2]);
        break;
    case 3: ROP(<);
    case 4: ROP(^);
    case 5:
        reg[rd] = f7 & 0x20 ? static_cast<int32_t>(reg[rs1]) >> (reg[rs2] & ONES(5))
                            : reg[rs1] >> (reg[rs2] & ONES(5));
        break;
    case 6: ROP(|);
    case 7: ROP(&);
#undef ROP
    default:
        assert(0);
        break;
    }
}

void simulator::run_I_ld(const instr_t ir) {
    const uint32_t rd = ir.as.itype.rd;
    const uint32_t rs1 = ir.as.itype.rs1;
    const uint32_t f3 = ir.as.itype.f3;
    const uint32_t addr = reg[rs1] + ir.as.itype.imm;
    const uint32_t off = addr & ~ADDR_BASE;
    /* There are 5 types of loads -- 2 each for byte- and halfword-
     * sizes, and one for word-size. 2 instructions are required for
     * byte and halfword loads because the msb of the register could
     * be either zero-extended, or sign-extended. This doesn't matter
     * for word-sized load because a whole register's worth of data
     * is read from memory, and there's no extension involved.
     */
    switch (f3) {
    case 0:
        // lb rd, imm(rs1)
        reg[rd] = static_cast<int8_t>(mem[off]);
        break;
    case 1:
        // lh rd, imm(rs1)
        reg[rd] = *reinterpret_cast<int16_t *>(&mem[off]);
        break;
    case 2:
        // lw rd, imm(rs1)
        reg[rd] = *reinterpret_cast<uint32_t *>(&mem[off]);
        break;
    case 4:
        // lbu rd, imm(rs1)
        reg[rd] = mem[off];
        break;
    case 5:
        // lhu rd, imm(rs1)
        reg[rd] = *reinterpret_cast<uint16_t *>(&mem[off]);
        break;
    default:
        errmsg_illegal();
        exit(1);
    }
}

void simulator::run_I_alu(const instr_t ir) {
    const uint32_t rd = ir.as.itype.rd;
    const uint32_t rs1 = ir.as.itype.rs1;
    const uint32_t f3 = ir.as.itype.f3;
    const uint32_t imm = ir.as.itype.imm;
    switch (f3) {
#define IOP(op) ({ reg[rd] = reg[rs1] op imm; break; })
    case 0: IOP(+);
    case 1: IOP(<<);
    case 2:
        reg[rd] = static_cast<int32_t>(reg[rs1]) < static_cast<int32_t>(imm);
        break;
    case 3: IOP(<);
    case 4: IOP(^);
    case 5:
        reg[rd] = imm & 0x400 ? static_cast<int32_t>(reg[rs1]) >> (imm & ~0x400) : reg[rs1] >> imm;
        break;
    case 6: IOP(|);
    case 7: IOP(&);
#undef IOP
    default:
        assert(0);
        break;
    }
}

void simulator::run_I_jmp(const instr_t ir) {
    const uint32_t rd = ir.as.itype.rd;
    const uint32_t rs1 = ir.as.itype.rs1;
    const uint32_t imm = ir.as.itype.imm;

    reg[rd] = prev_pc + 4;
    pc = reg[rs1] + imm;
}

void simulator::run_I_err(const instr_t ir) {
    errmsg_illegal();
    exit(1);
}

void simulator::run_S(instr_t ir) {
    const uint32_t f3 = ir.as.stype.f3;
    const uint32_t rs1 = ir.as.stype.rs1;
    const uint32_t rs2 = ir.as.stype.rs2;
    const uint32_t imm = ir.as.stype.imm;
    const uint32_t addr = reg[rs1] + imm;
    const uint32_t off = addr & ~ADDR_BASE;
    /* There are 3 types of stores -- byte-, halfword-, and word-sized. Unlike
     * load, there are no signed/unsigned variants, since stores to memory
     * write exactly as many bytes as stipulated, and there's no sign/zero
     * extension involved.
     */
    switch (f3) {
    case 0:
        // sb rs2, imm(rs1)
        mem[off] = static_cast<uint8_t>(reg[rs2]);
        break;
    case 1:
        // sh rs2, imm(rs1)
        *reinterpret_cast<uint16_t *>(&mem[off]) = static_cast<uint16_t>(reg[rs2]);
        break;
    case 2:
        // sw rs2, imm(rs1)
        *reinterpret_cast<uint32_t *>(&mem[off]) = reg[rs2];
        break;
    default:
        errmsg_illegal();
        exit(1);
    }
}

void simulator::run_B(instr_t ir) {
    const uint32_t f3 = ir.as.stype.f3;
    const uint32_t rs1 = ir.as.stype.rs1;
    const uint32_t rs2 = ir.as.stype.rs2;
    const uint32_t imm = ir.as.stype.imm;
    const uint32_t tgt = prev_pc + imm;
    switch (f3) {
#define BOPU(op) ({                           \
        pc = reg[rs1] op reg[rs2] ? tgt : pc; \
        break;                                \
    })
#define BOPS(op) ({                                     \
        pc = static_cast<int32_t>(reg[rs1]) op          \
             static_cast<int32_t>(reg[rs2]) ? tgt : pc; \
        break;                                          \
    })
    case 0: BOPU(==);
    case 1: BOPU(!=);
    case 4: BOPS(<);
    case 5: BOPS(>=);
    case 6: BOPU(<);
    case 7: BOPU(>=);
#undef BOPU
#undef BOPS
    default:
        errmsg_illegal();
        exit(1);
    }
}

void simulator::run_U(instr_t ir) {
    const uint32_t op = ir.op;
    const uint32_t rd = ir.as.utype.rd;
    const uint32_t imm = ir.as.utype.imm;

    reg[rd] = imm + (op & 0x20 ? 0 : prev_pc);
}

void simulator::run_J(instr_t ir) {
    const uint32_t rd = ir.as.utype.rd;
    const uint32_t imm = ir.as.utype.imm;
    const uint32_t tgt = prev_pc + imm;

    reg[rd] = prev_pc + 4;
    pc = tgt;
}

void simulator::errmsg_illegal() {
    const uint32_t word = *reinterpret_cast<uint32_t *>(&mem[prev_pc & ~ADDR_BASE]);
    fprintf(stderr, "%s: error: Illegal instruction 0x%08x ", prog_name, word);
    fprintf(stderr, "at address 0x%08x\n", prev_pc);
}
