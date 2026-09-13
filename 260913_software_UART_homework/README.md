# 作业项目：不用固定引脚和硬件，使用软件实现 UART 串口功能

## 开发记录
### 2026年9月13日
- 需要记录的问题：关于两个工程文件被git记录之后的换行符问题
  ```
  warning: in the working copy of '260913_software_UART_homework/project.uvopt', LF will be replaced by CRLF the next time Git touches it
  warning: in the working copy of '260913_software_UART_homework/project.uvproj', LF will be replaced by CRLF the next time Git touches it
  ```
- 想法（顿悟）：这个项目TI和RI可以分别用INT0和INT1两个外部中断，而不是像原项目一样只占用一个UART中断。我之前曾经想过如果要完全复刻那么需要添加硬件与门再连接到外部中断引脚上，但这是不必要的。
- 想法（更激进）：可以修改整个架构？（毕竟UART模式框图中的部分如果要实现那么似乎就要用ISR。这样传进来一次信息会触发两次中断。这是否不太好？）
