//
// Created by SiriusNEO on 2021/6/28.
//

#ifndef RISC_V_SIMULATOR_CPU_CORE_HPP
#define RISC_V_SIMULATOR_CPU_CORE_HPP

#include "decoder.hpp"
#include "basic_components.hpp"
#include "advanced_components.hpp"

namespace RISC_V {

    class CPU {
    public:
        explicit CPU(ADVANCED::PredictorType _ptype, ADVANCED::HazardHandleType _htype):
        pc(0), regs(), mem(), htype(_htype), predictor(_ptype), dataHazard(0) {}

        void run() { //WB before ID
            while (EX.IR.ins != HALT) {
#ifdef DEBUG
                        MINE("tick: " << clock.tick)
#endif

                        passMessage();
                        int order[5] = {0, 1, 2, 3, 4};
                        std::random_shuffle(order, order+5); //simulate parallel
#ifdef DEBUG
                        for (int i = 0; i < 5; ++i) std::cout << order[i] << ' '; std::cout << '\n';
#endif
                        for (int i = 0; i < 5; ++i) (this->*stageFunc[order[i]])();
                        clock.clockIn();
                        manage();

#ifdef DEBUG
                        std::cout << "IF:" << clock.stall[0] << " ID:" << clock.stall[1] << " EX:" << clock.stall[2] << " MEM:" << clock.stall[3] << " WB:" << clock.stall[4] << '\n';
                        cache.display();
                        bypass.display();
                        regs.display();
#endif
                    }
                    std::cout << std::dec << (regs.read(FUNCTION_RETURN) & 255u) << '\n';
#ifdef LOCAL
                    /*std::cout << "* Performance *" << '\n' << "Total Tick: " << clock.tick << '\n';
                                        std::cout << "Data Hazard: " << dataHazard << '\n';
                                        predictor.display(); */

                    std::cout << 1.0*predictor.success/(predictor.success+predictor.wrong);
#endif
                }

        private:
            //BASIC:: CPU 基础元件
            //ADVANCED:: CPU 特殊设计优化元件

            uint32_t pc; //电路中pc
            Decoder id; //译码中心

            BASIC::Registers regs; //CPU内置的通用寄存器组
            BASIC::Memory mem; //内存
            BASIC::Clock clock; //调度时钟
            BASIC::ALU alu; //运算中心
            BASIC::SignalBus bus; //信号总线
            BASIC::StageRegister ID, EX, MEM, WB; //当前信息
            BASIC::StageRegister IF_ID, ID_EX, EX_MEM, MEM_WB; //衔接指令寄存器, INPUT_OUTPUT

            ADVANCED::BranchPredictor<12, 6> predictor; //分支预测器
            ADVANCED::Bypass bypass; //旁路，用于data forwarding
            ADVANCED::HazardHandleType htype;
            ADVANCED::ICache<16> cache;

            size_t dataHazard;

            void instructionFetch();
            void instructionDecode();
            void execute();
            void memoryAccess();
            void writeBack();

            void (CPU::*stageFunc[5])() = {&CPU::instructionFetch, &CPU::instructionDecode, &CPU::execute, &CPU::memoryAccess, &CPU::writeBack};

            void passMessage() {
                if (!clock.isNotUpdate(ID_Stage)) {
                    ID = IF_ID;
                    if (!clock.isStall(ID_Stage)) IF_ID.clear(); //stall: keep last data
                }
                if (!clock.isNotUpdate(EX_Stage)) {
                    EX = ID_EX;
                    if (!clock.isStall(EX_Stage)) ID_EX.clear();
                }
                if (!clock.isNotUpdate(MEM_Stage)) {
                    MEM = EX_MEM;
                    if (!clock.isStall(MEM_Stage)) EX_MEM.clear();
                }
                if (!clock.isNotUpdate(WB_Stage)) {
                    WB = MEM_WB;
                    if (!clock.isStall(WB_Stage)) MEM_WB.clear();
                }
            }

            void manage() {
                //bypass update
                if (clock.memOver() && htype == ADVANCED::FORWARDING)
                    bypass.update(clock.isStall(EX_Stage), clock.isStall(MEM_Stage));

                //memory 3 cycle
                if (bus.memoryAccess) {
                    bus.memoryAccess = false;
                    clock.memSet(2);
                    clock.stallRequest(MEM_Stage, 2); //left 2 cycle
                    //because mem is stalled, the following stages are required to be stalled.
                    clock.stallRequest(IF_Stage, 2);
                    clock.stallRequest(ID_Stage, 2);
                    clock.stallRequest(EX_Stage, 2);
                }

                //jump
                if (bus.isJump) { //jump, clear the IF
                    pc = bus.tarpc;
                    bus.isJump = false;
                    IF_ID.clear();
                    cache.clear();
                }
                else if (bus.isBranch && !predictor.nowpc) {
                    bus.predictHit = predictor.predict(ID_EX.pc); //the pc fetch by IF
                    if (bus.predictHit) pc = bus.tarpc, IF_ID.clear(), cache.clear(); //predict: jump
                }

                //data hazard
                if (htype == ADVANCED::STALL) hazardStallStrategy();
                else if (htype == ADVANCED::FORWARDING) hazardForwardingStrategy();
            }

            void hazardStallStrategy() {
                if (!clock.isNotUpdate(ID_Stage)) {
                    bool isHazard = false;
                    if (MEM_WB.IR.rd && (MEM_WB.IR.rd == ID_EX.IR.rs1 || MEM_WB.IR.rd == ID_EX.IR.rs2)) {
                        clock.stallRequest(EX_Stage, 2);
                        clock.stallRequest(ID_Stage);
                        clock.notUpdateRequest(ID_Stage, 2);
                        clock.stallRequest(IF_Stage, 2);
                        isHazard = true;
                    }
                    if (WB.IR.rd && (WB.IR.rd == ID_EX.IR.rs1 || WB.IR.rd == ID_EX.IR.rs2)) {
                        clock.stallRequest(EX_Stage);
                        clock.notUpdateRequest(ID_Stage);
                        clock.stallRequest(IF_Stage);
                        isHazard = true;
                    }
                    if (EX_MEM.IR.rd && (EX_MEM.IR.rd == ID_EX.IR.rs1 || EX_MEM.IR.rd == ID_EX.IR.rs2)) {
                        clock.stallRequest(EX_Stage, 3);
                        clock.stallRequest(ID_Stage, 2);
                        clock.notUpdateRequest(ID_Stage, 3);
                        clock.stallRequest(IF_Stage, 3);
                        isHazard = true;
                    }
                    dataHazard += isHazard;
                }
            }

            void hazardForwardingStrategy() {
                if (!clock.isNotUpdate(ID_Stage)) { //avoid repeat stall
                    bool isHazard = false;
                    //Wait a cycle because it is in MEM
                    if (isLoad(MEM_WB.IR.ins) && MEM_WB.IR.rd && (MEM_WB.IR.rd == ID_EX.IR.rs1 || MEM_WB.IR.rd == ID_EX.IR.rs2)) {
                        clock.stallRequest(EX_Stage);
                        clock.notUpdateRequest(ID_Stage);
                        clock.stallRequest(IF_Stage);
                        isHazard = true;
                    }
                    //EX_MEM
                    if (EX_MEM.IR.rd && (EX_MEM.IR.rd == ID_EX.IR.rs1 || EX_MEM.IR.rd == ID_EX.IR.rs2)) {
                        clock.stallRequest(EX_Stage);
                        clock.notUpdateRequest(ID_Stage);
                        clock.stallRequest(IF_Stage);
                        isHazard = true;
                    }
                    dataHazard += isHazard;
                }
            }
    };
}

#endif //RISC_V_SIMULATOR_CPU_CORE_HPP
