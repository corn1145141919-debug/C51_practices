# 作业项目：不用固定引脚和硬件，使用软件实现 UART 串口功能

## 作业内容
由于

## 开发记录
### 2026年9月13日
- 需要记录的问题：关于两个工程文件被git记录之后的换行符问题
  ```
  warning: in the working copy of '260913_software_UART_homework/project.uvopt', LF will be replaced by CRLF the next time Git touches it
  warning: in the working copy of '260913_software_UART_homework/project.uvproj', LF will be replaced by CRLF the next time Git touches it
  ```
- 想法（顿悟）：这个项目TI和RI可以分别用INT0和INT1两个外部中断，而不是像原项目一样只占用一个UART中断。我之前曾经想过如果要完全复刻那么需要添加硬件与门再连接到外部中断引脚上，但这是不必要的。
- 想法（更激进）：可以修改整个架构？（毕竟UART模式框图中的部分如果要实现那么似乎就要用ISR。这样传进来一次信息会触发两次中断。这是否不太好？）
### 9月14日
- 想法：硬件方面的架构不同决定了我绝对不会一板一眼地复刻原硬件 UART 的逻辑流程。
#### 首先只考虑 Receiver 的架构。思考清楚前绝不动笔写代码。
- 想法：经过与 GPT 交流，确定“外部下降沿中断开启接收进程，计时器中断进行具体接收”的框架。此时需要留意：在接收过程中前者应当被禁止触发（我的想法是物理层面上不被触发）
- 想法： Handling 不应该占用时间太长（因为第一个字节发送完成后第二个字节紧接着就会来）——不过通过flag请求中断？这不是又要引入第三个中断了吗？——看来需要其级别更低，或者直接在main中完成
- 假设 Handling 足够快，并且不会丢数据
- 想法：需要复刻每个bit“三中取二”以及进入start bit之后的再次检测
- 不需要对应原RI中断的中断——因为CPU处理传入的字节并不需要硬件动作或者严格的时间规定（一定要在收到字节后若干微秒内完成处理）——原来有中断是因为UART硬件模块与CPU是分开的
- 暂时不引入缓冲机制，在CPU没有消费完旧数据时直接丢弃（假设要加缓冲区也要加在main中或者说调用软件UART服务的程序中）
