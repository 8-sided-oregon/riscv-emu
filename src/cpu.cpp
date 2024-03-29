#include <array>
#include <memory>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <stdexcept>

#include "cpu.hpp"
#include "util.hpp"

/* CURRENTLY IMPLEMENTED
 * =====================
 * - RV64I Base Integer Instruction Set
 * - M Standard Extension for Integer Multiplication and Division
 * - A Standard Extension for Atomic Instructions
 * TODO: Implement Zicsr, F, and G extensions, as well as the privledged instruction set.
 * */

void Cpu::reserve(std::size_t addr, std::size_t inst) {
    for (auto it = reservations.begin(); it != reservations.end(); it++) {
        if (it->addr == addr) {
            it->inst = inst;
            return;
        }
    }
    reservations.push_back({.addr = addr, .inst = inst});
}

// returns true only when there is a reservation set for the address with the corresponding right instruction
std::optional<std::size_t> Cpu::invalidate(std::size_t addr) {
    for (auto it = reservations.begin(); it != reservations.end(); it++) {
        if (it->addr == addr) {
            std::size_t iter_inst = it->inst;
            reservations.erase(it);
            return {iter_inst};
        }
    }
    return {};
}

void Cpu::dump_regs() {
    for (std::size_t i = 0; i < registers.size(); i += 2)
        std::cout << std::format("x{}:\t0x{:016x}\tx{}:\t0x{:016x}\n", 
                i, static_cast<uint64_t>(registers[i]), 
                i + 1, static_cast<uint64_t>(registers[i + 1]));
}

void handle_op_im(uint32_t inst, Cpu &cpu) {
    enum funct3 {
        ADDI = 0b000,
        SLTI = 0b010,
        SLTIU = 0b011,
        XORI = 0b100,
        ORI = 0b110,
        ANDI = 0b111,
        SLLI = 0b001,
        SR = 0b101,
    };

    int64_t imm = get_i_imm(inst);
    
    uint8_t funct3 = (inst >> 12) & 0b111;
    std::size_t rd = (inst >> 7) & 0x1f;
    std::size_t rs1 = (inst >> 15) & 0x1f;
    uint8_t shamt = (inst >> 20) & 0x3f;

    if (rd == 0)
        return;

    switch (funct3) {
    case ADDI: {
        cpu.registers[rd] = imm + cpu.registers[rs1];
        break;
    }
    case SLTI: {
        cpu.registers[rd] = cpu.registers[rs1] < imm ? 1 : 0;
        break;
    }
    case SLTIU: {
        cpu.registers[rd] = static_cast<uint64_t>(cpu.registers[rs1]) < static_cast<uint64_t>(imm) ? 1 : 0;
        break;
    }
    case XORI: {
        cpu.registers[rd] = cpu.registers[rs1] ^ imm;
        break;
    }
    case ORI: {
        cpu.registers[rd] = cpu.registers[rs1] | imm;
        break;
    }
    case ANDI: {
        cpu.registers[rd] = cpu.registers[rs1] & imm;
        break;
    }
    case SLLI: {
        cpu.registers[rd] = cpu.registers[rs1] << shamt;
        break;
    }
    case SR: {
        if (((imm >> 30) & 1) == 0)
            // SRLI
            cpu.registers[rd] = static_cast<uint64_t>(cpu.registers[rs1]) >> shamt;
        else
            // SRAI
            cpu.registers[rd] = cpu.registers[rs1] >> shamt;
        break;
    }
    }
}

void handle_op_im_32(uint32_t inst, Cpu &cpu) {
    enum funct3 {
        ADDIW = 0b000,
        SLLIW = 0b001,
        SR = 0b101,
    };

    int64_t imm = get_i_imm(inst);
    
    uint8_t funct3 = (inst >> 12) & 0b111;
    std::size_t rd = (inst >> 7) & 0x1f;
    std::size_t rs1 = (inst >> 15) & 0x1f;
    uint8_t shamt = (inst >> 20) & 0x1f;

    switch (funct3) {
    case ADDIW: {
        cpu.registers[rd] = static_cast<int32_t>(cpu.registers[rs1]) + imm;
        break;
    }
    case SLLIW: {
        cpu.registers[rd] = static_cast<int32_t>(cpu.registers[rs1]) << shamt;
        break;
    }
    case SR: {
        if (((imm > 30) & 1) == 0)
            // SRLIW
            cpu.registers[rd] = static_cast<uint32_t>(cpu.registers[rs1]) >> shamt;
        else
            // SRAIW
            cpu.registers[rd] = static_cast<int32_t>(cpu.registers[rs1]) >> shamt;
        break;
    }
    }
}

void handle_op_op(uint32_t inst, Cpu &cpu) {
    enum funct {
        ADD = 0b0000000000,
        SUB = 0b0100000000,

        SLL =  0b001,
        SLT =  0b010,
        SLTU = 0b011,
        XOR =  0b100,

        SRL = 0b0000000101,
        SRA = 0b0100000101,

        OR =  0b110,
        AND = 0b111,

        MUL =    0b0000001000,
        MULH =   0b0000001001,
        MULHSU = 0b0000001010,
        MULHU =  0b0000001011,

        DIV =    0b0000001100,
        DIVU =   0b0000001101,
        REM =    0b0000001110,
        REMU =   0b0000001111,
    };

    uint16_t funct = ((inst >> 12) & 0b111) | ((inst >> 22) & 0b1111111000);
    std::size_t rd = (inst >> 7) & 0x1f;
    int64_t rs1 = cpu.registers[(inst >> 15) & 0x1f];
    int64_t rs2 = cpu.registers[(inst >> 20) & 0x1f];

    if (rd == 0)
        return;

    __uint128_t res;
    __int128_t res_s;

    switch (funct) {
    case ADD: {
        cpu.registers[rd] = rs1 + rs2;
        break;
    }
    case SUB: {
        cpu.registers[rd] = rs1 - rs2;
        break;
    }
    case SLT: {
        cpu.registers[rd] = rs1 < rs2 ? 1 : 0;
        break;
    }
    case SLTU: {
        cpu.registers[rd] = 
            static_cast<uint64_t>(rs1) < static_cast<uint64_t>(rs2) ? 1 : 0;
        break;
    }
    case AND: {
        cpu.registers[rd] = rs1 & rs2;
        break;
    }
    case OR: {
        cpu.registers[rd] = rs1 | rs2;
        break;
    }
    case XOR: {
        cpu.registers[rd] = rs1 ^ rs2;
        break;
    }
    case SLL: {
        cpu.registers[rd] = rs1 << (rs2 & 0x3f);
        break;
    }
    case SRL: {
        cpu.registers[rd] = static_cast<uint64_t>(rs1) >> (rs2 & 0x3f);
        break;
    }
    case SRA: {
        cpu.registers[rd] = rs1 >> (rs2 & 0x3f);
        break;
    }
    case MUL: {
        cpu.registers[rd] = rs1 * rs2;
        break;
    }
    case MULH: {
        res_s = static_cast<__int128_t>(rs1) * static_cast<__int128_t>(rs2);
        cpu.registers[rd] = res_s >> 64;
        break;
    }
    case MULHU: {
        // Double casting so we dont get sign extended to 128 bits
        res = static_cast<__uint128_t>(static_cast<uint64_t>(rs1)) *
            static_cast<__uint128_t>(static_cast<uint64_t>(rs2));
        cpu.registers[rd] = res >> 64;
        break;
    }
    case MULHSU: {
        res_s = static_cast<__int128_t>(rs1) * 
            static_cast<__uint128_t>(static_cast<uint64_t>(rs2));
        cpu.registers[rd] = res_s >> 64;
        break;
    }
    case DIV: {
        if (cpu.registers[rs2] == 0)
            cpu.registers[rd] = ~0;
        else if (rs2 == -1 && rs1 == INT64_MIN)
            cpu.registers[rd] = rs1;
        else
            cpu.registers[rd] = rs1 / rs2;
        break;
    }
    case DIVU: {
        if (cpu.registers[rs2] == 0)
            cpu.registers[rd] = ~0;
        else
            cpu.registers[rd] = static_cast<uint64_t>(rs1) / static_cast<uint64_t>(rs2);
        break;
    }
    // TODO: Check if behavior for REM is correct
    case REM: {
        if (rs2 == 0)
            cpu.registers[rd] = rs1;
        else if (rs2 == -1 && rs1 == INT64_MIN)
            cpu.registers[rd] = 0;
        else
            cpu.registers[rd] = rs1 % rs2;
        break;
    }
    case REMU: {
        if (rs2 == 0)
            cpu.registers[rd] = rs1;
        else
            cpu.registers[rd] = static_cast<uint64_t>(rs1) % static_cast<uint64_t>(rs2);
        break;
    }
    }
}

void handle_op_op_32(uint32_t inst, Cpu &cpu) {
    enum funct {
        ADDW = 0b000,
        SUBW = 0b0100000000,
        SLLW = 0b001,
        SRLW = 0b101,
        SRAW = 0b0100000101,
        MULW = 0b0000001000,
        DIVW = 0b0000001100,
        DIVUW = 0b0000001101,
        REMW = 0b0000001110,
        REMUW = 0b0000001111,
    };

    uint16_t funct = ((inst >> 12) & 0b111) | ((inst >> 22) & 0b1111111000);
    std::size_t rd = (inst >> 7) & 0x1f;
    int32_t rs1 = cpu.registers[(inst >> 15) & 0x1f];
    int32_t rs2 = cpu.registers[(inst >> 20) & 0x1f];

    switch (funct) {
    case ADDW: {
        cpu.registers[rd] = rs1 + rs2;
        break;
    }
    case SUBW: {
        cpu.registers[rd] = rs1 - rs2;
        break;
    }
    case SLLW: {
        cpu.registers[rd] = rs1 << (rs2 & 0x3f);
        break;
    }
    case SRLW: {
        cpu.registers[rd] = static_cast<uint32_t>(rs1) >> (rs2 & 0x3f);
        break;
    }
    case SRAW: {
        cpu.registers[rd] = rs1 >> (rs2 & 0x3f);
        break;
    }
    case MULW: {
        cpu.registers[rd] = rs1 * rs2;
        break;
    }
    case DIVW: {
        if (rs2 == 0)
            cpu.registers[rd] = ~0;
        else if (rs2 == -1 && cpu.registers[rs1] == INT32_MIN)
            cpu.registers[rd] = rs1;
        else
            cpu.registers[rd] = rs1 / rs2;
        break;
    }
    case DIVUW: {
        if (rs2 == 0)
            cpu.registers[rd] = ~0;
        else
            cpu.registers[rd] = static_cast<uint32_t>(rs1) / static_cast<uint32_t>(rs2);
        break;
    }
    // TODO: Check if behavior for REM(U)W is correct
    case REMW: {
        if (rs2 == 0)
            cpu.registers[rd] = rs1;
        else if (rs2 == -1 && rs1 == INT32_MIN)
            cpu.registers[rd] = 0;
        else
            cpu.registers[rd] = rs1 % rs2;
        break;
    }
    case REMUW: {
        if (rs2 == 0)
            cpu.registers[rd] = rs1;
        else
            cpu.registers[rd] = static_cast<uint32_t>(rs1) % static_cast<uint32_t>(rs2);
        break;
    }
    }
}

void handle_op_branch(uint32_t inst, Cpu &cpu) {
    enum funct3 {
        BEQ = 0b000,
        BNE = 0b001,
        BLT = 0b100,
        BLTU = 0b110,
        BGE = 0b101,
        BGEU = 0b111,
    };

    auto imm = get_b_imm(inst);

    std::cout << std::format("b_imm: {}\n", imm);

    uint8_t funct3 = (inst >> 12) & 0b111;
    std::size_t rs1 = (inst >> 15) & 0x1f;
    std::size_t rs2 = (inst >> 20) & 0x1f;
    bool take_branch = false;

    switch (funct3) {
    case BEQ: {
        if (cpu.registers[rs1] == cpu.registers[rs2])
            take_branch = true;
        break;
    }
    case BNE: {
        if (cpu.registers[rs1] != cpu.registers[rs2])
            take_branch = true;
        break;
    }
    case BLT: {
        if (cpu.registers[rs1] < cpu.registers[rs2])
            take_branch = true;
        break;
    }
    case BGE: {
        if (cpu.registers[rs1] >= cpu.registers[rs2])
            take_branch = true;
        break;
    }
    case BLTU: {
        if (static_cast<uint64_t>(cpu.registers[rs1]) < static_cast<uint64_t>(cpu.registers[rs2]))
            take_branch = true;
        break;
    }
    case BGEU: {
        if (static_cast<uint64_t>(cpu.registers[rs1]) >= static_cast<uint64_t>(cpu.registers[rs2]))
            take_branch = true;
        break;
    }
    }

    if (take_branch)
        cpu.pc += imm - 4;
}

void handle_op_load(uint32_t inst, Cpu &cpu) {
    enum funct3 {
        LB = 0b000,
        LH = 0b001,
        LW = 0b010,
        LD = 0b011,
        LBU = 0b100,
        LHU = 0b101,
        LWU = 0b110,
    };

    uint16_t funct3 = (inst >> 12) & 0b111;
    std::size_t rd = (inst >> 7) & 0x1f;
    std::size_t rs1 = (inst >> 15) & 0x1f;
    auto imm = get_i_imm(inst);
    auto address = (cpu.registers[rs1] + imm) % mem_size;

    qword_u loaded;
    switch (funct3) {
    case LB: {
        cpu.registers[rd] = static_cast<int8_t>((*cpu.memory)[address]);
        break;
    }
    case LH: {
        loaded.bytes[0] = (*cpu.memory)[address];
        loaded.bytes[1] = (*cpu.memory)[address+1];
        cpu.registers[rd] = loaded.word_s;
        break;
    }
    case LW: {
        loaded.bytes[0] = (*cpu.memory)[address];
        loaded.bytes[1] = (*cpu.memory)[address+1];
        loaded.bytes[2] = (*cpu.memory)[address+2];
        loaded.bytes[3] = (*cpu.memory)[address+3];
        cpu.registers[rd] = loaded.dword_s;
        break;
    }
    case LD: {
        loaded.bytes[0] = (*cpu.memory)[address];
        loaded.bytes[1] = (*cpu.memory)[address+1];
        loaded.bytes[2] = (*cpu.memory)[address+2];
        loaded.bytes[3] = (*cpu.memory)[address+3];
        loaded.bytes[4] = (*cpu.memory)[address+4];
        loaded.bytes[5] = (*cpu.memory)[address+5];
        loaded.bytes[6] = (*cpu.memory)[address+6];
        loaded.bytes[7] = (*cpu.memory)[address+7];
        cpu.registers[rd] = loaded.qword_s;
        break;
    }
    case LBU: {
        cpu.registers[rd] = (*cpu.memory)[address];
        break;
    }
    case LHU: {
        loaded.bytes[0] = (*cpu.memory)[address];
        loaded.bytes[1] = (*cpu.memory)[address+1];
        cpu.registers[rd] = loaded.word;
        break;
    }
    case LWU: {
        loaded.bytes[0] = (*cpu.memory)[address];
        loaded.bytes[1] = (*cpu.memory)[address+1];
        loaded.bytes[2] = (*cpu.memory)[address+2];
        loaded.bytes[3] = (*cpu.memory)[address+3];
        cpu.registers[rd] = loaded.dword;
        break;
    }
    }
}

void handle_op_store(uint32_t inst, Cpu &cpu) {
    enum funct3 {
        SB = 0b000,
        SH = 0b001,
        SW = 0b010,
        SD = 0b011,
    };

    uint16_t funct3 = (inst >> 12) & 0b111;
    std::size_t rs1 = (inst >> 15) & 0x1f;
    std::size_t rs2 = (inst >> 20) & 0x1f;
    auto imm = get_s_imm(inst);
    auto address = (cpu.registers[rs1] + imm) % mem_size;

    qword_u storing{};
    storing.qword = cpu.registers[rs2];

    switch (funct3) {
    case SD:
        (*cpu.memory)[address + 7] = storing.bytes[7];
        (*cpu.memory)[address + 6] = storing.bytes[6];
        (*cpu.memory)[address + 5] = storing.bytes[5];
        (*cpu.memory)[address + 4] = storing.bytes[4];
        [[fallthrough]];
    case SW:
        (*cpu.memory)[address + 3] = storing.bytes[3];
        (*cpu.memory)[address + 2] = storing.bytes[2];
        [[fallthrough]];
    case SH:
        (*cpu.memory)[address + 1] = storing.bytes[1];
        [[fallthrough]];
    case SB:
        (*cpu.memory)[address] = storing.bytes[0];
    }
}

std::optional<int> handle_op_system(uint32_t inst, Cpu &cpu) {
    uint8_t funct12 = inst >> 20;

    if (funct12 == 1) // EBREAK
        return std::nullopt;

    // ECALL
    std::cout << std::format("ecall @ 0x{:08x}\n", cpu.pc);
    cpu.dump_regs();

    if (cpu.registers[10] == 1) {
        std::cout << "exit syscall: x10 = 1\n";
        return cpu.registers[11];
    }

    return std::nullopt;
}

template<typename T, typename UT>
void handle_amo_gen(std::size_t funct5, std::size_t rd, std::size_t rs1, std::size_t rs2, Cpu &cpu) {
    enum funct5_vals {
        LR      = 0b00010,
        SC      = 0b00011,
        AMOSWAP = 0b00001,
        AMOADD  = 0b00000,
        AMOXOR  = 0b00100,
        AMOAND  = 0b01100,
        AMOOR   = 0b01000,
        AMOMIN  = 0b10000,
        AMOMAX  = 0b10100,
        AMOMINU = 0b11000,
        AMOMAXU = 0b11100,
    };
    
    std::size_t addr = cpu.registers[rs1];

    if (addr % sizeof(T) != 0)
        throw std::runtime_error("AMO address misalignment");

    T *addr_ptr = (T *)&(*cpu.memory)[addr];
    cpu.registers[rd] = *addr_ptr;

    switch (funct5) {
    // Load reserved. Registers a reservation set and loads 
    case LR: {
        std::cout << "LR\n";
        cpu.contexts[cpu.cur_hart].last_lr = cpu.pc;
        cpu.reserve(addr, cpu.pc);
        break;
    }
    // Store conditional. If reservation set is maintained, store rs2 into [rs1], then write 0 into rd. Else, 
    // write 1 into rd.
    case SC: {
        std::cout << "SC\n";
        auto& last_lr = cpu.contexts[cpu.cur_hart].last_lr;

        auto inv = cpu.invalidate(addr);

        if (last_lr == std::nullopt || inv == std::nullopt || *inv != *last_lr) {
            cpu.registers[rd] = 1;
            return;
        }

        last_lr = std::nullopt;
        *addr_ptr = cpu.registers[rs2];
        cpu.registers[rd] = 0;
        break;
    }
    case AMOSWAP: {
        *addr_ptr = cpu.registers[rs2];
        break;
    }
    case AMOADD: {
        *addr_ptr = cpu.registers[rd] + cpu.registers[rs2];
        break;
    }
    case AMOXOR: {
        *addr_ptr = cpu.registers[rd] ^ cpu.registers[rs2];
        break;
    }
    case AMOAND: {
        *addr_ptr = cpu.registers[rd] & cpu.registers[rs2];
        break;
    }
    case AMOOR: {
        *addr_ptr = cpu.registers[rd] | cpu.registers[rs2];
        break;
    }
    case AMOMIN: {
        if (cpu.registers[rs2] < cpu.registers[rd]) {
            *addr_ptr = cpu.registers[rs2];
        }
        break;
    }
    case AMOMAX: {
        if (cpu.registers[rs2] > cpu.registers[rd]) {
            *addr_ptr = cpu.registers[rs2];
        }
        break;
    }
    case AMOMINU: {
        if (static_cast<UT>(cpu.registers[rs2]) < static_cast<UT>(cpu.registers[rd])) {
            *addr_ptr = cpu.registers[rs2];
        }
        break;
    }
    case AMOMAXU: {
        if (static_cast<UT>(cpu.registers[rs2]) > static_cast<UT>(cpu.registers[rd])) {
            *addr_ptr = cpu.registers[rs2];
        }
        break;
    }
    }
}

void handle_op_amo(uint32_t inst, Cpu &cpu) {

    std::size_t rd = (inst >> 7) & 0b11111;
    uint8_t width = (inst >> 12) & 0b111;
    std::size_t rs1 = (inst >> 15) & 0b11111;
    std::size_t rs2 = (inst >> 20) & 0b11111;
    uint8_t funct5 = (inst >> 27) & 0b11111;

    // WORD
    if (width == 2) {
        handle_amo_gen<int32_t, uint32_t>(funct5, rd, rs1, rs2, cpu);
    }
    // DWORD
    else if (width == 3) {
        handle_amo_gen<int64_t, uint64_t>(funct5, rd, rs1, rs2, cpu);
    }
}
