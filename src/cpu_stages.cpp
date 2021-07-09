//
// Created by SiriusNEO on 2021/7/2.
//

#include "cpu_core.hpp"

namespace RISC_V {

    void CPU::instructionFetch() {
        if (clock.isStall(IF_Stage)) return;
        if (cache.empty()) cache.load(mem.getPool()+pc);
        IF_ID.insCode = cache.get();
        IF_ID.pc = pc;
        pc += 4;
        /*
            IF_ID.insCode = mem.read(pc, 4);
            IF_ID.pc = pc;
            pc += 4;
        */
#ifdef DEBUG
                MINE("fetch result: " << std::hex << IF_ID.pc << ' ' << IF_ID.insCode);
#endif
    }

    void CPU::instructionDecode() {
        if (clock.isStall(ID_Stage)) return;
        id.decode(ID.insCode, ID_EX.IR);
        ID_EX.A = regs.read(ID_EX.IR.rs1);
        ID_EX.B = regs.read(ID_EX.IR.rs2);
        if (htype == ADVANCED::FORWARDING) {
            bypass.mux(ID_EX.IR.rs1, ID_EX.A);
            bypass.mux(ID_EX.IR.rs2, ID_EX.B);
        }
        ID_EX.pc = ID.pc; //pass EX old pc, store new pc in tarpc
        if (ID_EX.IR.ins == JAL || ID_EX.IR.ins == JALR || id.TypeTable[ID_EX.IR.opcode] == BType) {
            if (ID_EX.IR.ins == JAL) {
                bus.isJump = true;
                ID_EX.out = ID_EX.pc + 4;
                alu.input(ID_EX.pc, ID_EX.IR.imm, '+');
                bus.tarpc = alu.ALUOut;
            } else if (ID_EX.IR.ins == JALR) {
                bus.isJump = true;
                ID_EX.out = ID_EX.pc + 4;
                alu.input(ID_EX.A, ID_EX.IR.imm, '+');
                bus.tarpc = alu.ALUOut & ~1;
            } else {
                if (bus.isBranch) bus.delayFlag = true;
                bus.isBranch = true;
                alu.input(ID_EX.pc, ID_EX.IR.imm, '+');
                bus.tarpc = alu.ALUOut;
            }
        }
#ifdef DEBUG
                MINE("decode result: " << std::dec << insName[ID_EX.IR.ins] << " imm:" << ID_EX.IR.imm << " rd:" <<
                        ID_EX.IR.rd << " rs1: " << ID_EX.IR.rs1 << " A:" << ID_EX.A << " rs2: " << ID_EX.IR.rs2 <<  " B:" << ID_EX.B << " shamt:" << ID_EX.IR.shamt)
#endif
    }

    void CPU::execute() {
        if (clock.isStall(EX_Stage)) return;
        EX_MEM = EX;
        bus.branchHit = false;
        switch (EX.IR.ins) {
            case LUI:{
                EX_MEM.out = EX.IR.imm;
            }break;
            case AUIPC:{
                alu.input(EX.pc, EX.IR.imm, '+');
                EX_MEM.out = alu.ALUOut;
            }break;
            case BEQ:{
                alu.input(EX.A, EX.B, '-');
                if (alu.zero) bus.branchHit = true;
            }break;
            case BNE:{
                alu.input(EX.A, EX.B, '-');
                if (!alu.zero) bus.branchHit = true;
            }break;
            case BLT:{
                alu.input(EX.A, EX.B, '-');
                if (alu.neg) bus.branchHit = true;
            }break;
            case BGE:{
                alu.input(EX.A, EX.B, '-');
                if (!alu.neg) bus.branchHit = true;
            }break;
            case BLTU:{
                alu.input(EX.A, EX.B, '-', false);
                if (alu.neg) bus.branchHit = true;
            }break;
            case BGEU:{
                alu.input(EX.A, EX.B, '-', false);
                if (!alu.neg) bus.branchHit = true;
            }break;
            case LB:case LH:case LW:case LBU:case LHU:case SB:case SH:case SW:{
                alu.input(EX.A, EX.IR.imm, '+');
                EX_MEM.out = alu.ALUOut;
                bus.memoryAccess = true;
            }
            case ADDI:{
                alu.input(EX.A, EX.IR.imm, '+');
                EX_MEM.out = alu.ALUOut;
            }break;
            case SLTI:{
                alu.input(EX.A, EX.IR.imm, '-');
                EX_MEM.out = alu.neg;
            }break;
            case SLTIU:{
                alu.input(EX.A, EX.IR.imm, '-', false);
                EX_MEM.out = alu.neg;
            }break;
            case XORI:{
                alu.input(EX.A, EX.IR.imm, '^');
                EX_MEM.out = alu.ALUOut;
            }break;
            case ORI:{
                alu.input(EX.A, EX.IR.imm, '|');
                EX_MEM.out = alu.ALUOut;
            }break;
            case ANDI:{
                alu.input(EX.A, EX.IR.imm, '&');
                EX_MEM.out = alu.ALUOut;
            }break;
            case SLLI:{
                alu.input(EX.A, EX.IR.shamt, '<');
                EX_MEM.out = alu.ALUOut;
            }break;
            case SRLI:{
                alu.input(EX.A, EX.IR.shamt, '>', false);
                EX_MEM.out = alu.ALUOut;
            }break;
            case SRAI:{
                alu.input(EX.A, EX.IR.shamt, '>', true);
                EX_MEM.out = alu.ALUOut;
            }break;
            case ADD:{
                alu.input(EX.A, EX.B, '+');
                EX_MEM.out = alu.ALUOut;
            }break;
            case SUB:{
                alu.input(EX.A, EX.B, '-');
                EX_MEM.out = alu.ALUOut;
            }break;
            case SLL:{
                alu.input(EX.A, EX.B, '<');
                EX_MEM.out = alu.ALUOut;
            }break;
            case SLT:{
                alu.input(EX.A, EX.B, '-');
                EX_MEM.out = alu.neg;
            }break;
            case SLTU:{
                alu.input(EX.A, EX.B, '-', false);
                EX_MEM.out = alu.neg;
            }break;
            case XOR:{
                alu.input(EX.A, EX.B, '^');
                EX_MEM.out = alu.ALUOut;
            }break;
            case SRL:{
                alu.input(EX.A, EX.B, '>', false);
                EX_MEM.out = alu.ALUOut;
            }break;
            case SRA:{
                alu.input(EX.A, EX.B, '>', true);
                EX_MEM.out = alu.ALUOut;
            }break;
            case OR:{
                alu.input(EX.A, EX.B, '|');
                EX_MEM.out = alu.ALUOut;
            }break;
            case AND:{
                alu.input(EX.A, EX.B, '&');
                EX_MEM.out = alu.ALUOut;
            }break;
        }
        if (!isLoad(EX.IR.ins))
            bypass.send(EX.IR.rd, EX_MEM.out, EX_Stage);
        if (bus.branchHit) {
            alu.input(EX.pc, EX.IR.imm, '+');
            EX_MEM.pc = alu.ALUOut;
        }
        else EX_MEM.pc = EX.pc + 4;
        if (bus.isBranch && predictor.nowpc > 0) {
            predictor.update(bus.branchHit);
            if (bus.branchHit ^ bus.predictHit) { //Wrong Predict
                predictor.wrong++;
                pc = EX_MEM.pc; //EX_MEM.pc is the correct pc
                IF_ID.clear(); //clear pipeline, last IF, this ID is meaningless
                ID.clear();
                ID_EX.clear();
                bus.jumpInfoClear();
                cache.clear();
            }
            else {
                predictor.success++;
                if (!bus.delayFlag) bus.isBranch = false;
                else bus.delayFlag = false;
            }
        }
#ifdef DEBUG
                MINE("execute result: " << insName[EX.IR.ins] << " imm:" << EX.IR.imm
                << " rs1:" << EX.IR.rs1 << " A:" << EX.A << " rs2:" << EX.IR.rs2 << " B:" << EX.B << " ALUOut:" << EX_MEM.out << " pc:" << EX_MEM.pc)
#endif
    }

    void CPU::memoryAccess() {
        if (clock.isStall(MEM_Stage)) return;
        MEM_WB = MEM;
        switch (MEM.IR.ins) {
            case LB:
                MEM_WB.out = mem.reads(MEM.out, 1);
                break;
            case LH:
                MEM_WB.out = mem.reads(MEM.out, 2);
                break;
            case LW:
                MEM_WB.out = mem.reads(MEM.out, 4);
                break;
            case LBU:
                MEM_WB.out = mem.read(MEM.out, 1);
                break;
            case LHU:
                MEM_WB.out = mem.read(MEM.out, 2);
                break;
            case SB:
                mem.write(MEM.out, MEM.B, 1);
                break;
            case SH:
                mem.write(MEM.out, MEM.B, 2);
                break;
            case SW:
                mem.write(MEM.out, MEM.B, 4);
                break;
        }
        bypass.send(MEM_WB.IR.rd, MEM_WB.out, MEM_Stage);
#ifdef DEBUG
                MINE("memory access result: " << insName[MEM.IR.ins] << " rd: " << MEM.IR.rd << " output:" << MEM_WB.out)
#endif
    }

    void CPU::writeBack() {
        if (clock.isStall(WB_Stage)) return;
        regs.write(WB.IR.rd, WB.out);
#ifdef DEBUG
                MINE("write back result: " << insName[WB.IR.ins] << " rd: " << WB.IR.rd << " output:" << WB.out)
#endif
    }
}