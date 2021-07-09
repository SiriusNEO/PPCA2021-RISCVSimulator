//
// Created by cetc01 on 2021/6/28.
//

#ifndef RISC_V_SIMULATOR_DECODER_HPP
#define RISC_V_SIMULATOR_DECODER_HPP

#include "include.hpp"

namespace RISC_V {
        class Decoder {
        public:
            void decode(uint32_t insCode, Instruction &ret) {
                ret.init();
                ret.opcode = slice(insCode, 0, 6);
                if (insCode == 0x00008067) {// ret
                    ret.ins = JALR;
                    ret.rs1 = RETURN_ADDRESS;
                    ret.imm = 0;
                    return;
                }
                if (insCode == 0x0ff00513) {//end simulator
                    ret.ins = HALT;
                    return;
                }
                switch (TypeTable[ret.opcode]) {
                    case RType: {
                        ret.rd = slice(insCode, 7, 11);
                        ret.funct3 = slice(insCode, 12, 14);
                        ret.rs1 = slice(insCode, 15, 19);
                        ret.rs2 = slice(insCode, 20, 24);
                        ret.funct7 = slice(insCode, 25, 31);
                    }
                        break;
                    case IType: {
                        ret.rd = slice(insCode, 7, 11);
                        ret.funct3 = slice(insCode, 12, 14);
                        ret.rs1 = slice(insCode, 15, 19);
                        ret.imm = sext(slice(insCode, 20, 31), 11);
                        if (ret.opcode == 0b0010011 && (ret.funct3 == 0b101 || ret.funct3 == 0b001)) { //slli srli srai
                            ret.funct7 = slice(insCode, 25, 31);
                            ret.shamt = slice(insCode, 20, 24);
                        }
                    }
                        break;
                    case SType: {
                        ret.imm = sext((slice(insCode, 25, 31) << 5) + slice(insCode, 7, 11), 11);
                        ret.funct3 = slice(insCode, 12, 14);
                        ret.rs1 = slice(insCode, 15, 19);
                        ret.rs2 = slice(insCode, 20, 24);
                    }
                        break;
                    case BType: {
                        ret.imm = sext((slice(insCode, 31, 31) << 12) + (slice(insCode, 7, 7) << 11) +
                                       (slice(insCode, 25, 30) << 5) +
                                       (slice(insCode, 8, 11) << 1), 12);
                        ret.funct3 = slice(insCode, 12, 14);
                        ret.rs1 = slice(insCode, 15, 19);
                        ret.rs2 = slice(insCode, 20, 24);
                    }
                        break;
                    case UType: {
                        ret.rd = slice(insCode, 7, 11);
                        ret.imm = sext(slice(insCode, 12, 31) << 12, 31);
                    }
                        break;
                    case JType: {
                        ret.rd = slice(insCode, 7, 11);
                        ret.imm = sext((slice(insCode, 31, 31) << 20) + (slice(insCode, 12, 19) << 12) +
                                       (slice(insCode, 20, 20) << 11) +
                                       (slice(insCode, 21, 30) << 1), 20);
                    }
                        break;
                }
                ret.ins = InstructionTable[(Triple) {ret.opcode, ret.funct3, ret.funct7}];
            }

            std::map <uint32_t, InsFormatType> TypeTable = { //opcode -> type
                    {0b0110111, UType},
                    {0b0010111, UType},
                    {0b1101111, JType},
                    {0b1100011, BType},
                    {0b0000011, IType},
                    {0b0100011, SType},
                    {0b0010011, IType},
                    {0b0110011, RType}
            };

            std::map <Triple, InsType> InstructionTable = { //opcode func1 func3 -> ins
                    {(Triple) {0b0110111, 0, 0},             LUI},
                    {(Triple) {0b0010111, 0, 0},             AUIPC},
                    {(Triple) {0b1101111, 0, 0},             JAL},
                    {(Triple) {0b1100111, 0, 0},             JALR},
                    {(Triple) {0b1100011, 0b000, 0},         BEQ},
                    {(Triple) {0b1100011, 0b001, 0},         BNE},
                    {(Triple) {0b1100011, 0b100, 0},         BLT},
                    {(Triple) {0b1100011, 0b101, 0},         BGE},
                    {(Triple) {0b1100011, 0b110, 0},         BLTU},
                    {(Triple) {0b1100011, 0b111, 0},         BGEU},
                    {(Triple) {0b0000011, 0b000, 0},         LB},
                    {(Triple) {0b0000011, 0b001, 0},         LH},
                    {(Triple) {0b0000011, 0b010, 0},         LW},
                    {(Triple) {0b0000011, 0b100, 0},         LBU},
                    {(Triple) {0b0000011, 0b101, 0},         LHU},
                    {(Triple) {0b0100011, 0b000, 0},         SB},
                    {(Triple) {0b0100011, 0b001, 0},         SH},
                    {(Triple) {0b0100011, 0b010, 0},         SW},
                    {(Triple) {0b0010011, 0b000, 0},         ADDI},
                    {(Triple) {0b0010011, 0b010, 0},         SLTI},
                    {(Triple) {0b0010011, 0b011, 0},         SLTIU},
                    {(Triple) {0b0010011, 0b100, 0},         XORI},
                    {(Triple) {0b0010011, 0b110, 0},         ORI},
                    {(Triple) {0b0010011, 0b111, 0},         ANDI},
                    {(Triple) {0b0010011, 0b001, 0},         SLLI},
                    {(Triple) {0b0010011, 0b101, 0},         SRLI},
                    {(Triple) {0b0010011, 0b101, 0b0100000}, SRAI},
                    {(Triple) {0b0110011, 0b000, 0},         ADD},
                    {(Triple) {0b0110011, 0b000, 0b0100000}, SUB},
                    {(Triple) {0b0110011, 0b001, 0},         SLL},
                    {(Triple) {0b0110011, 0b010, 0},         SLT},
                    {(Triple) {0b0110011, 0b011, 0},         SLTU},
                    {(Triple) {0b0110011, 0b100, 0},         XOR},
                    {(Triple) {0b0110011, 0b101, 0},         SRL},
                    {(Triple) {0b0110011, 0b101, 0b0100000}, SRA},
                    {(Triple) {0b0110011, 0b110, 0},         OR},
                    {(Triple) {0b0110011, 0b111, 0},         AND},
            };
        };
}

#endif //RISC_V_SIMULATOR_DECODER_HPP
