# 作业项目：带优先级的浮点数计算器，使用矩阵键盘做输入，数码管做输出

## 作业内容

## 开发记录
### 早期模糊记录（大约在2026年9月前期）

### 2026年10月16日
- （你大概率只能看到这一天的具体开发过程，顶多再加上10月17日，这确实是我在ddl前最后关头火速手搓的）
- 凌晨辗转反侧难以入眠，遂通过与GPT对话逐步完善了expression_receiver和expression_solver两个软件层架构，并且重点研究了expression_receiver层的状态机——四个状态，以及对所有按键情况下的边界行为
- 当天下午第一节课后学习了调度场算法，并指定了expression_solver的处理方案：表达式树叶子节点为数而非零节点是运算符（这里只有左结合运算符且没有括号）。人类键入表达式树的中序遍历而计算机处理的是表达式树的后序遍历。用到调度场算法。
- 火速开始开发。在开发过程中敲定了“receiver层立即发送操作数token或者算符token给solver层，发送结算命令时结算”的方案。
- 现已完成receiver层与entering_number、waiting_for_operand和showing_answer三大状态相关的各种操作
- 根据理论分析，当前运算模式下，depth of operator stack为2，depth of operand stack为3则足够。
- 在solver中需要保证operator和operand token交错到来。否则可能会出现不可预知的情况，比如让上述的栈溢出。
- 现在的工程取舍：如果某一次来的token和上一次来的token类型相同，那么就静默忽略。
- `commit 646c56e`状态下，showing_answer状态下先按下运算符再按下等于号的行为似乎有点错误。（太复杂了我不想细细解释了）
- 经过探讨，解决方案如下：规定在showing_answer状态下按下运算符按键之后，ans就降格为普通的左操作数，被推给solver，同时状态转为一般的waiting_for_operand；showing_answer状态下不按下运算符而直接按下按键，则相当于是在键入新的操作数覆盖旧的操作数，状态转移为entering_number且不推送任何token。
- 边界情况：showing_answer条件下，即使先按下operator然后按下等于号，那么按等于号时也是从waiting_for_operand跳转出去的，并且历史上只提交了一个token（就是之前的ans），行为仍然正确，这一轮ans被赋值为上一轮answer。
- 注意，showing_answer状态下的行为相当程度上证明了“第一个被推出去的token是operand”。一方面，该状态下按运算符会触发整个表达式的第一个token的发送，它是一个operand token；另一方面，该状态下直接按数字会转移到entering_number，后续会推出第一个token，它也是operand token。
- 既然waiting_for_operand和showing_answer已经累积了这么多不同，为了代码的可维护性，我们必须将它们分开了。

### 10月17日
- 解决了首位数字触发状态转移到entering_number__integer_part时数字被忽略的问题
- 公开化`expression_solver_init()`并让其显式出现在`expression_solver_solve`处负责扫尾
- 连续修复了若干GPT指出的小错误
- “基座架构”不改变，仍然是阻塞式检查按键 + ISR刷新数码管（这是一个月左右以前我自己跑通的足够好的基座）。我们只是加入了今天写的软件层。
- 构建了映射
- 决策：弃用display_float，使用display 32位整数的函数外加添加小数点的函数
- 决策：将核心状态机逻辑、对外暴露的接口`expression_receiver_process_input()`以及专门的显示信息处理函数耦合。
- 决策：状态机函数传回的显示参数是一个结构体
- 决策；弃用OVERFLOW，只显示Error
- 截止凌晨睡觉前的主要剩余工作：小事情是确定超出8个十进制位时的显示行为；大事情是完成整个显示逻辑
