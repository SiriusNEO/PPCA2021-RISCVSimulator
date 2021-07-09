#include "cpu_core.hpp"

int main() {
    using namespace RISC_V;
    using namespace ADVANCED;

#ifdef DEBUG
    BASIC::ALU alu;
    alu.input(0xbc8f0000, 1, '>', true);
    std::cout << std::hex << alu.ALUOut << '\n';
    std::cout << "Hello, RISCV!" << std::endl;
    //freopen("testcases/array_test1.data", "r", stdin);
    //freopen("sample.data", "r", stdin);
    freopen("myout.txt", "w", stdout);
#endif
    srand(time(0));

    CPU Yakumo_Ran(TWOLEVEL, FORWARDING);
    Yakumo_Ran.run();
    return 0;
}