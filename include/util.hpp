#pragma once

#include <cstdint>

union qword_u {
    uint64_t qword;
    int64_t qword_s;
    uint32_t dword;
    int32_t dword_s;
    uint16_t word;
    int16_t word_s;

    uint8_t bytes[8];
};

union dword_u {
    uint32_t dword;
    int32_t dword_s;
    uint16_t word;
    int16_t word_s;

    uint8_t bytes[4];
};

[[nodiscard]] constexpr int32_t get_i_imm(uint32_t inst) {
    int8_t sign_extended = inst >> 24;
    return static_cast<int32_t>(sign_extended) << 4 | inst >> 20;
}

[[nodiscard]] constexpr int32_t get_s_imm(uint32_t inst) {
    uint8_t data1 = (inst >> 7) & 0b11111;
    int8_t data2_se = (inst >> 25);
    return static_cast<int32_t>(data2_se) << 5 | data1;
}

[[nodiscard]] constexpr int32_t get_b_imm(uint32_t inst) {
    uint32_t data1 = (inst >> 7) & 0b11110;
    uint32_t data2 = ((inst >> 25) << 5);
    uint32_t eleventh = ((inst >> 7) & 1) << 10;
    int32_t twelth = 0u - (inst >> 31);
    return (twelth & ~0xfff) | eleventh | data2 | data1;
}

[[nodiscard]] constexpr int32_t get_u_imm(uint32_t inst) {
    return (inst >> 12) << 12;
}

[[nodiscard]] constexpr int32_t get_j_imm(uint32_t inst) {
    uint32_t eleventh = (inst & (1 << 20)) >> 9;
    uint32_t twentieth = (inst & (1 << 31)) >> 11;
    uint32_t data1 = (inst >> 20) & 0b11111111110;
    uint32_t data2 = (inst & (0b111111111 << 12));
    return twentieth | data2 | eleventh | data1;
}
