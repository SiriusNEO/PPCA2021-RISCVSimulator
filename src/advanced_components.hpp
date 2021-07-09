//
// Created by SiriusNEO on 2021/7/2.
//

#ifndef RISC_V_SIMULATOR_ADVANCED_COMPONENTS_HPP
#define RISC_V_SIMULATOR_ADVANCED_COMPONENTS_HPP

#include "include.hpp"

namespace RISC_V {
    namespace ADVANCED {
        enum PredictorType {
            AT, ANT, BHT, TWOLEVEL
        };

        template<size_t BIT = 12, size_t N = 2> //N=6 best for superloop
        class BranchPredictor {
        private:
            PredictorType type;
        public:
            int wrong, success, nowpc;

            uint32_t bht[1 << (BIT + N - 2)], table[4][2]; //00, 01, 10, 11
            uint32_t pht[1 << BIT];

            explicit BranchPredictor(PredictorType _type) : type(_type), wrong(0), success(0) {
                table[0][0] = 0, table[0][1] = 1;
                table[1][0] = 2, table[1][1] = 3;
                table[2][0] = 0, table[2][1] = 1;
                table[3][0] = 2, table[3][1] = 3;
                memset(bht, 0, sizeof(bht));
                memset(pht, 0, sizeof(pht));
            }

            bool predict(uint32_t pc = 0) {
                nowpc = slice(pc, 0, BIT - 1);
                switch (type) {
                    case AT:
                        return true;
                    case ANT:
                        return false;
                    case BHT:
                        return bht[nowpc] > 1; //00 01, failure
                    case TWOLEVEL:
                        return bht[(nowpc << (N - 2)) + slice(pht[nowpc], 0, N - 1)];
                }
                return false;
            }

            void update(bool isJump = false) {
                if (type == BHT) bht[nowpc] = table[bht[nowpc]][isJump];
                else if (type == TWOLEVEL) {
                    uint32_t tar = (nowpc << (N - 2)) + slice(pht[nowpc], 0, N - 1);
                    bht[tar] = table[bht[tar]][isJump];
                    pht[nowpc] = (pht[nowpc] << 1) | isJump;
                }
                nowpc = 0;
            }

            void display() {
                std::cout << "* Predictor *" << '\n';
                std::cout << "Total:" << success+wrong << ", Success:" << success << ", Wrong:" << wrong << '\n';
                std::cout << "Success Rate:" << 1.0 * success / (success+wrong) << '\n';
            }
        };

        enum HazardHandleType {STALL, FORWARDING};
        class Bypass {
        private:
            uint32_t EXBypassRd[2], EXBypassVal[2], MEMBypassRd[2], MEMBypassVal[2]; //0 last-T, 1 this-T
        public:
            Bypass() {
                EXBypassRd[0] = EXBypassVal[0] = EXBypassRd[1] = EXBypassVal[1] = MEMBypassRd[0] = MEMBypassVal[0] = MEMBypassRd[1] = MEMBypassVal[1] = 0;
            }

            void mux(uint32_t rs, uint32_t& ret) { //only read last-T val
                if (EXBypassRd[0] == rs) ret = EXBypassVal[0];
                if (MEMBypassRd[0] == rs) ret = MEMBypassVal[0];
            }

            void send(uint32_t rd, uint32_t val, StageType stage) {
                if (!rd) return; //robust
                if (stage == EX_Stage) EXBypassRd[1] = rd, EXBypassVal[1] = val;
                else if (stage == MEM_Stage) MEMBypassRd[1] = rd, MEMBypassVal[1] = val;
            }

            void update(bool exStall, bool memStall) {
                if (!exStall) {
                    EXBypassRd[0] = EXBypassRd[1], EXBypassVal[0] = EXBypassVal[1];
                    EXBypassRd[1] = EXBypassVal[1] = 0;
                }
                if (!memStall) {
                    MEMBypassRd[0] = MEMBypassRd[1], MEMBypassVal[0] = MEMBypassVal[1];
                    MEMBypassRd[1] = MEMBypassVal[1] = 0;
                }
            }

            void clear() {
                EXBypassRd[0] = EXBypassVal[0] = EXBypassRd[1] = EXBypassVal[1] = MEMBypassRd[0] = MEMBypassVal[0] = MEMBypassRd[1] = MEMBypassVal[1] = 0;
            }

            void display() {
                std::cout << "EXOld: " << EXBypassRd[0] << ' ' << EXBypassVal[0] << ' ';
                std::cout << "MEMOld: " << MEMBypassRd[0] << ' ' << MEMBypassVal[0] << '\n';
                std::cout << "EXNew: " << EXBypassRd[1] << ' ' << EXBypassVal[1] << ' ';
                std::cout << "MEMNew: " << MEMBypassRd[1] << ' ' << MEMBypassVal[1] << '\n';
            }
        };

        template<size_t SIZE = 16>
        class ICache {
        private:
            uint32_t cache[SIZE<<2], pc;
        public:
            ICache():pc(SIZE<<2) {}
            void load(uint32_t* tar) {
                memcpy(cache, tar, sizeof(cache));
                pc = 0;
            }
            uint32_t get() {
                uint32_t ret = 0;
                pc += 4;
                for (int i = pc - 1; i >= signed(pc-4); --i)
                    ret = (ret << 8) + cache[i];
                return ret;
            }
            bool empty() {return pc >= (SIZE<<2);}
            void clear() {pc = (SIZE<<2);}
            void display() {
                std::cout << "Cache: ";
                for (int i = 0; i < (SIZE<<2); ++i) std::cout << std::hex << cache[i] << ' ';
                std::cout << '\n';
            }

        };
    }
}

#endif //RISC_V_SIMULATOR_ADVANCED_COMPONENTS_HPP
