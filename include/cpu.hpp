#pragma once

#include <iostream>
#include <cstdint>
#include <array>
#include <memory>

constexpr std::size_t mem_size = 16 * 1024 * 1024; // 16 MiB
constexpr std::size_t inst_buf_size = 65536; // 64 KiB
constexpr std::size_t program_bgn = 0;

struct Cpu {
    std::array<int64_t, 32> registers;
    uint64_t pc{program_bgn};
    std::unique_ptr<std::array<uint8_t, mem_size>> memory{ new std::array<uint8_t, mem_size> };

    void dump_regs() {
        for (std::size_t i = 0; i < registers.size(); i += 2)
            std::cout << std::format("x{}:\t0x{:016x}\tx{}:\t0x{:016x}\n", 
                    i, static_cast<uint64_t>(registers[i]), 
                    i + 1, static_cast<uint64_t>(registers[i + 1]));
    }
};

enum opcodes {
    OP_LOAD = 0b0000011,
    OP_LOAD_FP = 0b0000111,
    OP_MISC_MEM = 0b0001111,
    OP_OP_IMM = 0b0010011,
    OP_AUIPC = 0b0010111,
    OP_OP_IMM_32 = 0b0011011,

    OP_STORE = 0b0100011,
    OP_STORE_FP = 0b0100111,
    OP_AMO = 0b0101111,
    OP_OP = 0b0110011,
    OP_LUI = 0b0110111,
    OP_OP_32 = 0b0111011,

    OP_MADD = 0b1000011,
    OP_MSUB = 0b1000111,
    OP_NMSUB = 0b1001011,
    OP_NMADD = 0b1001111,
    OP_OP_FP = 0b1010011,

    OP_BRANCH = 0b1100011,
    OP_JALR = 0b1100111,
    OP_JAL = 0b1101011,
    OP_SYSTEM = 0b1110011,
};

void handle_op_im(uint32_t inst, Cpu &cpu);
void handle_op_im_32(uint32_t inst, Cpu &cpu);
void handle_op_op(uint32_t inst, Cpu &cpu);
void handle_op_op_32(uint32_t inst, Cpu &cpu);
void handle_op_branch(uint32_t inst, Cpu &cpu);
void handle_op_load(uint32_t inst, Cpu &cpu);
void handle_op_store(uint32_t inst, Cpu &cpu);
std::optional<int> handle_op_system(uint32_t inst, Cpu &cpu);
