# smartTrashCan
Smart Trash Can using Raspberry Pi / Pico W and C

## 專案功能
- 自動開蓋（HY-SRF05 + SG90）→ Pico W
- 滿溢偵測（TCRT5000）→ Rpi5
- CO 偵測（MQ-7 + MCP3008）→ Rpi5
- Server(Node.js) + HTML 顯示狀態 → Rpi5
- Raspberry Pi 5 + Pico W

## 程式結構
- `/lid`：Pico W 的感測器控制程式（C）
- `/server`：前端介面
- `/smartTrashCan/mq7`：一氧化碳感測器
- `/smartTrashCan/status`：一氧化碳 + 垃圾量滿狀態(傳到前端)
- `/smartTrashCan/tcrt5000`：紅外線反射（Linux kernel module（IRQ 與 sysfs））
