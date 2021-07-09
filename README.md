# Yakumo Ran - CPU

### 简介

PPCA2021 first assignment, a simple 5-Pipeline RISC-V Simulator

Contributor: [@SiriusNEO](https://github.com/SiriusNEO)

> Yakumo Ran - CPU，顾名思义，使用八云蓝制作而成的五级流水CPU.
>
> 八云蓝，九尾妖狐，八云紫的式神。
>
> 据说妖兽的尾巴越多魔力越高，越长则聪明。其智力相当高，尤其擅长数学，据说连人类所无法想象程度的计算都能够在瞬间完成。
>
> 采用狐狸式神所制作的CPU运算能力大概也十分强大吧（笑）



### 文件结构

```C++
include.hpp //一些预定义与头文件
cpu_core.hpp //CPU类以及一些调度函数的定义
cpu_stages.cpp //5个stages的定义
basic_components.hpp //CPU基础元件的定义，"基础元件"内容见下文
advanced_components.hpp //CPU高级元件的定义，"高级元件"内容见下文
decoder.hpp //一个精巧的解码器
main.cpp //main函数
```



### CPU设计

采取模拟仿真的原则，以真实CPU的元件为准。

其中“分支预测器的类型”与“是否采用数据前传”作为CPU构造函数的参数，可供随时开关。

```C++
class CPU {
    //BASIC:: CPU 基础元件
    //ADVANCED:: CPU 特殊设计优化元件
    
    uint32_t pc; //电路中pc
    Decoder id; 
    //译码中心，存有一张完整的解码表，以及提供一个公有的解码接口

    BASIC::Registers regs; 
    //CPU内置的通用寄存器组，x0寄存器由于硬件原因永远为0
    BASIC::Memory mem; 
    //内存，支持不同字节个数的内存读取（byte, halfword, word）
    BASIC::Clock clock; 
    //全局时钟，一个cycle会tick一下，Stall等行为需要向其申请
    BASIC::ALU alu; 
    //运算中心，支持加减以及位运算。同时有overflow, neg, zero三个flag，用于一些比较操作
    BASIC::SignalBus bus; 
    //信号总线，ControlBUS，用于将控制、时序信号反馈给CPU
    BASIC::StageRegister ID, EX, MEM, WB; 
    //每个Stage的当前信息，相当于当前Stage操作的Input
    BASIC::StageRegister IF_ID, ID_EX, EX_MEM, MEM_WB; 
    //衔接指令寄存器，相当于当前Stage操作的Output

    ADVANCED::BranchPredictor<12, 6> predictor; 
    //分支预测器，模板参数为BHT大小、PHT大小
    ADVANCED::Bypass bypass; 
    //旁路，用于data forwarding，在EX和MEM阶段都设计了往回传的线路
    ADVANCED::HazardHandleType htype;
   	//应对hazard的方式，可以选择无脑Stall或者用bypass前传
    ADVANCED::ICache<16> cache;
    //指令cache，模板参数为cache大小
}
```



### 执行流程

```C++
passMessage
//将上个周期每一Stage的Output载入Input
5-Stage Work
/*并行工作
 *为了模拟并行，使用了random shuffle打乱每个循环的执行顺序
 *L/S操作由于需要读取内存，使用了三个周期模拟
 */
manage
//操作结束后的一些调度，包括判断是否需要Stall，MEM阶段的跳转等    
```



### Hazard 策略

##### Hazard 类型

- Structural Hazard

  两个不同的 Stage 需要访问同一资源。模拟不出现。

- Data Hazard

  关键 Hazard，需要比较精细的处理，下文会细谈。

- Control Hazard

  出现不确定性的跳转（分支）。

  策略是解码出来后如果为 `JAR` 这种无悬念跳转，可以直接跳转；

  否则根据分支预测器决定是否预先跳转，在下一个周期的 EX 检查是否跳转正确，
  
  跳转失败进行反悔操作。



##### 数据冒险

真并行处理的数据冒险类型要稍多一些。

| Cycle1   | Cycle2   | Cycle3    | Cycle4   | Cycle5 | Cycle6 |
| -------- | -------- | --------- | -------- | ------ | ------ |
| WB       |          |           |          |        |        |
| MEM      | WB       |           |          |        |        |
| EX       | MEM      | WB        |          |        |        |
| ***ID*** | ***EX*** | ***MEM*** | ***WB*** |        |        |
| IF       | ID       | EX        | MEM      |        |        |
|          | IF       | ID        | EX       |        |        |
|          |          | IF        | ID       |        |        |

可以看出 Cycle1 的 ID 操作只可能和 Cycle2、Cycle3 发生冒险。

当然，如果把 Cycle1 5个操作全部 shuffle，它也可能和 Cycle 1 的发生冒险。

> 实际工程中上升沿、下降沿的机制似乎可以保证 WB 先于 ID，
>
> 由于还没上系统课不是很清楚，本程序仍不保证
>
> 也就是说我们要处理这三个 Cycle 的情况
>
> 我们下文可以发现这种并行的状况会要求流水线多停一轮，或者说“重新解码”

- 情况 1：Cycle 1 冒险

| Cycle0 | Cycle1 | Cycle2 | Cycle3 |
| ------ | ------ | ------ | ------ |
|        | ID     | EX     |        |
| MEM    | WB     |        |        |

  在第一轮循环中 WB 应该比 ID 先执行，但由于 shuffle 的关系不能确定先后顺序。

  采用 Stall：当前周期结束后 WB 已计算完成，在 Cycle2 重新 ID 即可

  采用 Forwarding：Cycle0 的时候 MEM->WB 会前传到 ID，无需停

- 情况 2：Cycle 2 冒险

| Cycle0 | Cycle1 | Cycle2 | Cycle3 |
| ------ | ------ | ------ | ------ |
|        | ID     |        |        |
| EX     | MEM    | WB     |        |

   采用 Stall：停一周期，等 Cycle2 WB 计算完成，在 Cycle3 重新 ID

  采用 Forwarding：如果指令不访问内存，在 EX 阶段结束就能计算完成，Cycle0 时候 EX->MEM 会前     传；如果是Load，只能等 MEM 阶段计算完成，然后前传，在 Cycle2 重新计算 ID

- 情况 3：Cycle 3 冒险

| Cycle0 | Cycle1 | Cycle2 | Cycle3 |
| ------ | ------ | ------ | ------ |
|        | ID     |        |        |
| ID     | EX     | MEM    | WB     |

  这种冒险 Forwarding 必须要停一轮，因为不可能在 ID->EX 时候前传

 Stall 要连停两轮



##### 内存访问

使用三个周期模拟一次 Memory Access，

 具体情况是前两个周期相当于 Stall，第三个周期进行内存操作

在停止的两个周期中同时需要停止 EX, ID, IF，相当于停止一段流水线

具体流水线情况变为下表样式

| Cycle1 | Cycle2 | Cycle3 | Cycle4     | Cycle5     | Cycle6 | Cycle7 |
| ------ | ------ | ------ | ---------- | ---------- | ------ | ------ |
| IF     | ID     | EX     | MEM        | MEM        | MEM    | WB     |
|        | IF     | ID     | (EX Stall) | (EX Stall) | EX     | MEM    |
|        |        | IF     | (ID Stall) | (ID Stall) | ID     | EX     |
|        |        |        | (IF Stall) | (IF Stall) | IF     | ID     |
|        |        |        |            |            |        | IF     |



### 高级元件

##### Predictor

提供了四种类型 Predictor，可用于测试不同数据的预测特征

```
AT, ANT, BHT, TWOLEVEL
```

- AT (Always Taken)，永远预测跳转

- ANT (Always Not Taken)，永远预测不跳转

- BHT (Branch History Table)，采用两位饱和计数器，

  每次预测传入当前 pc（取后 BIT 位，节省内存同时也带来预测错误风险）

  ```C++
  00 01 10 11 //Weak -> Strong
  ```

- TWOLEVEL (Two-Levle Adaptive Predictor)，在 BHT 基础上加了 PHT (Pattern History Table)

  可以比较准确地预测出具有固定模式的跳转 (e.g. 001001001001001)

  PHT 的位数 (即历史记录条数) 也会影响预测效率

  具体设计如下：

  ```C++
  pc -> PHT //对于每个PC，找到其 PHT
  offset <- PHT[N-1:0] //截取PHT的后N位，这是一种模式（即一共可以存2^N）
  pc+offset -> BHT //读BHT，因此两层预测器的BHT要开得更大    
  ```

  

##### Bypass

设计了两条反线路

```C++
EX (last cycle) -> ID (this cycle)
MEM (last cycle) -> ID (this cycle)
```

对于 Load 操作，其只能在 MEM 阶段得出结果，因此需要通过 MEM -> ID

对于大部分计算操作，通过 EX -> ID 传递即可

注意由于 shuffle 的影响，旁路也需设计为两层，防止新旧数据覆盖



##### ICache

比较简单的指令寄存器

Fetch 时，如果 Cache 为空，会直接从内存读一片（cache大小）的空间

如果不为空，则从 Cache 中直接读取下一个指令（真实情况下，这个读取比访问内存要快很多）

此外，注意一些特殊情况的处理：

- 发生跳转时（跳转反悔也发生了跳转），指令寄存器清空，因为寄存器后面的指令没有意义了。
- 读完时要注意重新再读一片。