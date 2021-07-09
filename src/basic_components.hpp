//
// Created by SiriusNEO on 2021/6/29.
//

#ifndef RISC_V_SIMULATOR_BASIC_COMPONENTS_HPP
#define RISC_V_SIMULATOR_BASIC_COMPONENTS_HPP

#include "include.hpp"

namespace RISC_V {
    namespace BASIC {
        class Clock {
        public:
            int stall[5], memtick, notUpdate[5];
            size_t tick;

            Clock() : tick(0), memtick(0) {
                memset(stall, 0, sizeof(stall));
                memset(notUpdate, 0, sizeof(notUpdate));
            }

            void clockIn() {
                tick++;
                for (int i = 0; i < 5; ++i) {
                    if (stall[i]) stall[i]--;
                    if (!memtick && notUpdate[i]) notUpdate[i]--;
                }
                if (memtick > 0) memtick--;
            }

            void stallRequest(StageType stage, size_t ticks = 1) {
                stall[stage] += ticks;
            }

            void notUpdateRequest(StageType stage, size_t ticks = 1) {
                notUpdate[stage] += ticks;
            }

            bool isNotUpdate(StageType stage) {
                return notUpdate[stage];
            }

            bool isStall(StageType stage) {
                return stall[stage];
            }

            void memSet(size_t ticks) {
                memtick = ticks;
            }

            bool memOver() { return memtick <= 0; }
        };

        class Registers {
        private:
            uint32_t regs[REG_N];
        public:
            Registers() {
                memset(regs, 0, sizeof(regs));
            }

            void display() const {
                for (int i = 0; i < REG_N; ++i)
                    std::cout << std::dec << "x[" << i << "]:" << regs[i] << '\n';
                //std::cout << '\n';
            }

            void write(size_t pos, uint32_t val) { if (pos) regs[pos] = val; }

            uint32_t read(size_t pos) const {
                return regs[pos];
            }
        };

        struct StageRegister {
            Instruction IR;
            uint32_t insCode, out, pc, A, B;

            StageRegister() : IR(), insCode(0), out(0), pc(0), A(0), B(0) {}

            bool operator == (const StageRegister& obj) const {
                return IR == obj.IR && insCode == obj.insCode && out == obj.out && pc == obj.pc
                && A == obj.A && B == obj.B;
            }

            void clear() {
                IR.init();
                insCode = out = pc = A = B = 0;
            }
        };

        class ALU {
        public:
            uint32_t ALUOut;
            bool neg, overflow, zero;

            ALU() = default;

            void input(uint32_t input1, uint32_t input2, char op, bool sign = true) {
                switch (op) {
                    case '+':
                        ALUOut = input1 + input2;
                        break;
                    case '-':
                        ALUOut = input1 - input2;
                        break;
                    case '<':
                        ALUOut = input1 << input2;
                        break;
                    case '>':
                        ALUOut = (sign) ? sext(input1 >> input2, 31 - input2) : (input1 >> input2);
                        break;
                    case '&':
                        ALUOut = input1 & input2;
                        break;
                    case '|':
                        ALUOut = input1 | input2;
                        break;
                    case '^':
                        ALUOut = input1 ^ input2;
                        break;
                }
                zero = (ALUOut == 0);
                if (!sign) neg = (input1 < input2); //unsigned, fix sign-bit
                else neg = (signed(ALUOut) < 0);
            }
        };

        class Memory {
        private:
            uint32_t memPool[MEM_SIZE];
            size_t siz;
        public:
            Memory() : siz(0) {
                memset(memPool, 0, sizeof(memPool));
                std::string buffer;
                while (std::cin >> buffer) {
                    std::stringstream trans;
                    if (buffer[0] == '@') {
                        trans << std::hex << buffer.substr(1);
                        trans >> siz;
                    } else {
                        trans << std::hex << buffer;
                        trans >> memPool[siz++];
                        //std::cout << memPool[siz-1] << '\n';
                    }
                }
#ifdef DEBUG
                MINE("build finish. size: " << siz)
#endif
            }

            uint32_t * getPool() {return memPool;}

            size_t size() const { return siz; }

            uint32_t read(size_t pos, size_t bytes = 1) const {
                uint32_t ret = 0;
                for (int i = pos + bytes - 1; i >= signed(pos); --i)
                    ret = (ret << 8) + memPool[i];
                return ret;
            }

            int32_t reads(size_t pos, size_t bytes) const {
                int32_t ret = 0;
                for (int i = pos + bytes - 1; i >= signed(pos); --i)
                    ret = (ret << 8) + memPool[i];
                return ret;
            }

            void write(size_t pos, uint32_t val, size_t bytes = 1) {
                for (int i = pos; i < pos + bytes; ++i)
                    memPool[i] = slice(val, 0, 7), val >>= 8;
            }
        };

        class SignalBus {
        public:
            bool isJump, isBranch, branchHit, predictHit, memoryAccess, delayFlag;
            uint32_t tarpc;
            void jumpInfoClear() {
                isJump = isBranch = branchHit = predictHit = 0;
            }
        };
    }
}

#endif //RISC_V_SIMULATOR_BASIC_COMPONENTS_HPP
