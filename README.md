# 在线串口调试助手

[![项目状态](https://img.shields.io/badge/状态-开发中-brightgreen)](https://github.com/your-repo)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

## 📖 项目简介

本项目为嵌入式开发者提供一个**简单、易用、功能强大**的串口调试助手。与传统串口调试助手不同，它完全运行在 **ESP-01S** 模块上：

- **无需 USB 转 TTL 线缆**：ESP-01S 通过 WiFi 与电脑/手机通信，实现无线调试。
- **便捷交互**：可随时通过浏览器进行参数调节、命令下发、数据反馈查看。
- **特别适合调参场景**：摆脱物理连线束缚，让调参过程更加流畅。

> 💡 **注意**：本项目是 **100% Vibe Coding** 的产物，若遇到 Bug 欢迎提 Issue 反馈。

## 🔧 项目原理

1. ESP-01S 接收单片机发送的串口数据（默认波特率 **115200**）。
2. ESP-01S 将串口数据转发到 WebSocket 服务。
3. WebSocket 将数据推送至前端页面，同时前端下发的指令也经此路径反向传输。

> 示意图（可选）：  
> `单片机 <--串口--> ESP-01S <--WiFi/WebSocket--> 浏览器(电脑/手机)`

## 📦 使用说明

> **当前使用说明文档正在完善中，敬请期待。**  
> 基本流程（示例）：
> 1. 烧录程序至 ESP-01S。
> 2. 将 ESP-01S 的 RX/TX 与目标单片机串口连接。
> 3. 上电后 ESP-01S 创建 WiFi 热点或连接至现有路由器。
> 4. 电脑/手机连接同一 WiFi，访问 ESP-01S 提供的 Web 页面。
> 5. 在页面中进行串口数据监控与指令发送。

详细步骤将在后续更新。

## 🙏 致谢

- **串口助手的组件化设计思想** 源自 [VOFA+](https://www.vofa.plus/)，特此感谢。

## 📄 许可证

本项目采用 MIT 许可证进行开源。详情请见 [LICENSE](./LICENSE) 文件。

## 🤝 贡献与反馈

欢迎提交 Issue 或 Pull Request。  
由于项目采用 Vibe Coding 方式，代码风格可能略显随意，但每一条反馈都会被认真对待。
