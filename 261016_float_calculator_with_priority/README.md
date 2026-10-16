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
