#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <format>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <cstdio>

#include "cpu.hpp"
#include "util.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " [instruction file]\n";
        return 1;
    }

    Cpu cpu{};

    // Loads instructions into memory from user supplied file
    {
        std::unique_ptr<FILE, void (*)(FILE *)> instruction_file{
            fopen(argv[1], "rb"),
            [](FILE *f) { 
                if (f != nullptr && fclose(f) == EOF) {
                    perror("error while closing instruction file");
                    throw std::runtime_error("error while closing instruction file");
                }
            }
        };

        if (instruction_file == nullptr) {
            perror("error while opening instruction file");
            return 1;
        }

        // copies instructions into memory
        std::vector<uint8_t> inst_buf(inst_buf_size);
        for (std::size_t cnt, ind = program_bgn; (cnt = fread(&inst_buf[0], 1, inst_buf.size(), &*instruction_file)) != 0; ind += cnt) {
            if (cnt + ind >= cpu.memory->size()) {
                std::cerr << "instruction file overflows memory, terminating...\n";
                return 1;
            }
            std::cout << cnt << "\n";
            std::copy_n(inst_buf.cbegin(), cnt, cpu.memory->begin() + ind);
        }

        if (ferror(&*instruction_file)) {
            perror("error while reading instruction file");
            return 1;
        }
    }
    
    // Fetch decode execute
    {
        for (;; cpu.pc += 4) {
            if (cpu.pc >= mem_size) {
                std::cerr << "invalid pc value: " << cpu.pc << "\n";
                return 1;
            }

            // fetch
            dword_u inst_bytes{};
            inst_bytes.bytes[0] = (*cpu.memory)[cpu.pc];
            inst_bytes.bytes[1] = (*cpu.memory)[cpu.pc+1];
            inst_bytes.bytes[2] = (*cpu.memory)[cpu.pc+2];
            inst_bytes.bytes[3] = (*cpu.memory)[cpu.pc+3];
            auto inst = inst_bytes.dword;

            std::cout << std::format("fetched: 0x{:08x} @ 0x{:08x}\n", inst, cpu.pc);

            if (inst == 0) {
                cpu.dump_regs();
                std::cerr << "invalid instruction at: 0x" << std::hex << cpu.pc << "\t\tvalue: " << inst << "\n";
                return 1;
            }

            // decode
            uint8_t opcode = inst & 0x7f;
            std::size_t rd = (inst >> 7) & 0x1f;
            std::size_t rs1 = (inst >> 15) & 0x1f;
            int32_t offset;

            // execute
            switch (opcode) {
            case OP_OP_IMM: {
                handle_op_im(inst, cpu);
                break;
            }
            case OP_OP: {
                handle_op_op(inst, cpu);
                break;
            }
            case OP_OP_IMM_32: {
                handle_op_im_32(inst, cpu);
                break;
            }
            case OP_LUI: {
                cpu.registers[rd] = inst & 0xfffff000;
                break;
            }
            case OP_AUIPC: {
                cpu.registers[rd] = (inst & 0xfffff000) + cpu.pc;
                break;
            }
            // TODO: Generate address misaligned exceptions for jump instructions
            case OP_JAL: {
                offset = get_j_imm(inst);
                if (rd != 0)
                    cpu.registers[rd] = cpu.pc + 4;
                cpu.pc += offset - 4;
                break;
            }
            case OP_JALR: {
                offset = get_i_imm(inst);
                if (rd != 0)
                    cpu.registers[rd] = cpu.pc + 4;
                cpu.registers[rs1] = (cpu.registers[rs1] + offset) & ~1;
                break;
            }
            case OP_BRANCH: {
                handle_op_branch(inst, cpu);
                break;
            }
            case OP_LOAD: {
                handle_op_load(inst, cpu);
                break;
            }
            case OP_STORE: {
                handle_op_store(inst, cpu);
                break;
            }
            case OP_SYSTEM: {
                auto rc = handle_op_system(inst, cpu);
                if (rc)
                    return rc.value();
                break;
            }
            case OP_AMO: {
                handle_op_amo(inst, cpu);
                break;
            }
            // FENCE (opcode=OP_MISC_MEM + funct3=FENCE) is handled as a noop
            // No hints are defined (for now atleast)
            }
        }
    }
}
