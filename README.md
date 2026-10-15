# STM32 DHT11 Weather Station

Mini project sử dụng **STM32 + DHT11 + CP2102 USB-to-TTL** để đo nhiệt độ, độ ẩm và giao tiếp hai chiều với máy tính thông qua UART.

Project được xây dựng theo hướng **Embedded C thực tế**, ưu tiên workflow tách khỏi IDE:

```text
VS Code
   │
   ├── C/C++
   ├── CMake
   └── Ninja / Make
          │
          ▼
   ARM GCC Toolchain
          │
          ▼
   firmware (.elf/.bin/.hex)
          │
          ▼
       STM32
          │
       UART
          │
       CP2102
          │ USB
          ▼
          PC
```

---

## 1. Mục tiêu dự án

Dự án được thực hiện theo từng cấp độ để luyện các kỹ năng Embedded C:

- GPIO
- UART
- Timer / delay và xử lý timing
- Giao tiếp với cảm biến DHT11
- Kiểm tra checksum
- Xử lý dữ liệu
- Giao tiếp UART hai chiều
- Command parser đơn giản
- Cấu trúc project C/C++ nhiều module
- Cross compilation bằng ARM GCC
- CMake
- Ninja hoặc Make
- Debug và flash firmware
- Git / GitHub workflow

---

## 2. Phần cứng

### Thành phần chính

| Thiết bị | Chức năng |
|---|---|
| STM32 | Vi điều khiển chính |
| DHT11 | Đo nhiệt độ và độ ẩm |
| CP2102 | Chuyển đổi USB ↔ UART/TTL |
| PC | Terminal / giám sát dữ liệu |
| Breadboard + jumper | Kết nối phần cứng |

> **Lưu ý:** Mã MCU/board STM32 cụ thể cần được xác định chính xác trước khi cấu hình chân GPIO, clock và linker.

---

## 3. Sơ đồ hệ thống

```text
                    ┌─────────────┐
                    │    DHT11    │
                    │             │
                    │ DATA        │
                    └──────┬──────┘
                           │
                           │ GPIO
                           ▼
                    ┌─────────────┐
                    │    STM32    │
                    │             │
                    │ UART TX ────┼─────────┐
                    │ UART RX ────┼──────┐  │
                    │ GND ────────┼────┐ │  │
                    └─────────────┘    │ │  │
                                       │ │  │
                              ┌────────▼─▼──▼────┐
                              │      CP2102       │
                              │                   │
                              │ RX ← STM32 TX     │
                              │ TX → STM32 RX     │
                              │ GND ──────────────│
                              └────────┬──────────┘
                                       │ USB
                                       ▼
                                  ┌──────────┐
                                  │    PC    │
                                  │ Terminal │
                                  └──────────┘
```

### Kết nối UART

UART phải đấu chéo:

```text
STM32 TX  →  CP2102 RX
STM32 RX  ←  CP2102 TX
STM32 GND ─  CP2102 GND
```

Không nối:

```text
TX → TX
RX → RX
```

---

## 4. DHT11

DHT11 sử dụng giao tiếp digital một dây và truyền tổng cộng **40 bit** dữ liệu:

```text
8 bit  Humidity integer
8 bit  Humidity decimal
8 bit  Temperature integer
8 bit  Temperature decimal
8 bit  Checksum
```

Checksum:

```text
checksum =
humidity_integer
+ humidity_decimal
+ temperature_integer
+ temperature_decimal
```

Firmware phải kiểm tra checksum trước khi chấp nhận dữ liệu.

DHT11 có tốc độ đo tương đối thấp, vì vậy project dự kiến đọc cảm biến khoảng **mỗi 2 giây** thay vì polling liên tục.

---

## 5. UART

Cấu hình UART dự kiến:

```text
Baud rate : 115200
Data bits : 8
Parity    : None
Stop bits : 1
Flow      : None
```

Tương đương:

```text
115200 8N1
```

> Baud rate thực tế phải được cấu hình giống nhau giữa STM32 và terminal trên PC.

---

## 6. Output dự kiến

Khi chạy firmware, terminal có thể hiển thị:

```text
================================
      STM32 WEATHER STATION
================================

Temperature : 28 C
Humidity    : 67 %
Status      : NORMAL
```

Khi vượt ngưỡng:

```text
Temperature : 36 C
Humidity    : 82 %

WARNING: HIGH TEMPERATURE!
WARNING: HIGH HUMIDITY!
```

---

## 7. Command từ PC

Sau khi hoàn thành UART cơ bản, firmware sẽ hỗ trợ command từ terminal.

### TEMP

PC:

```text
TEMP
```

STM32:

```text
Temperature: 28 C
```

### HUM

PC:

```text
HUM
```

STM32:

```text
Humidity: 67 %
```

### STATUS

PC:

```text
STATUS
```

STM32:

```text
Temperature: 28 C
Humidity: 67 %
Status: NORMAL
```

### SET TEMP

Thiết lập ngưỡng nhiệt độ:

```text
SET TEMP 35
```

Firmware lưu:

```text
Temperature limit = 35 C
```

---

## 8. Cảnh báo

Firmware có thể sử dụng các trạng thái:

```text
NORMAL
WARNING
DANGER
```

Ví dụ:

```text
Temperature < 35 C
→ NORMAL
```

```text
Temperature >= 35 C
→ WARNING
```

Có thể mở rộng thêm điều kiện:

```text
Humidity >= 80 %
→ HIGH HUMIDITY
```

---

## 9. Cấu trúc project

Cấu trúc dự kiến:

```text
STM32_DHT11/
│
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
│
├── Core/
│   ├── Inc/
│   └── Src/
│
├── Drivers/
│
├── App/
│   ├── dht11.c
│   ├── dht11.h
│   ├── uart_log.c
│   └── uart_log.h
│
├── main.c
│
└── build/
```

Thư mục `build/` không nên commit vào Git nếu project sử dụng build ngoài source tree.

Có thể thêm vào `.gitignore`:

```gitignore
build/
.vscode/
*.elf
*.bin
*.hex
```

---

## 10. Build system

Project hướng tới workflow:

```text
Source code
    ↓
CMake
    ↓
Ninja / Make
    ↓
arm-none-eabi-gcc
    ↓
firmware
```

Build cơ bản:

```bash
cmake -B build
cmake --build build
```

Nếu sử dụng Ninja:

```bash
cmake -B build -G Ninja
cmake --build build
```

---

## 11. Toolchain

Các công cụ dự kiến:

- VS Code
- C/C++ extension
- CMake
- CMake Tools
- Ninja hoặc GNU Make
- ARM GNU Toolchain (`arm-none-eabi-gcc`)
- STM32CubeMX cho cấu hình/generate phần ban đầu nếu cần
- STM32CubeProgrammer để flash/debug firmware
- Tera Term / PuTTY / SerialTool để kiểm tra UART
- Git

---

## 12. Development workflow

Workflow chính:

```text
1. Viết code
      ↓
2. CMake configure
      ↓
3. Build
      ↓
4. Kiểm tra warning/error
      ↓
5. Flash STM32
      ↓
6. Mở Serial Terminal
      ↓
7. Test UART
      ↓
8. Test DHT11
      ↓
9. Debug
      ↓
10. Commit Git
```

Mục tiêu là hạn chế việc phụ thuộc vào nút **Build** của một IDE cụ thể.

---

## 13. Roadmap phát triển

### Level 1 — GPIO

- [ ] Cấu hình GPIO
- [ ] Blink LED
- [ ] Kiểm tra project build

### Level 2 — UART

- [ ] Cấu hình UART
- [ ] STM32 gửi `"Hello World"`
- [ ] Kiểm tra bằng CP2102
- [ ] PC nhận dữ liệu
- [ ] STM32 nhận dữ liệu từ PC

### Level 3 — DHT11

- [ ] Implement protocol DHT11
- [ ] Đọc 40 bit
- [ ] Kiểm tra timing
- [ ] Kiểm tra checksum
- [ ] Trả về temperature/humidity

### Level 4 — Integration

- [ ] DHT11 + UART
- [ ] In nhiệt độ
- [ ] In độ ẩm
- [ ] Đọc cảm biến định kỳ
- [ ] Xử lý lỗi sensor

### Level 5 — Command interface

- [ ] `TEMP`
- [ ] `HUM`
- [ ] `STATUS`
- [ ] `SET TEMP xx`
- [ ] Command parser

### Level 6 — Warning system

- [ ] Temperature threshold
- [ ] Humidity threshold
- [ ] NORMAL/WARNING/DANGER
- [ ] Thông báo cảnh báo qua UART

### Level 7 — Data logging

- [ ] Gửi dữ liệu theo format dễ parse
- [ ] Ghi dữ liệu trên PC
- [ ] Viết Python tool đọc COM
- [ ] Vẽ biểu đồ nhiệt độ/độ ẩm

---

## 14. Mục tiêu cuối cùng

Phiên bản hoàn chỉnh hướng tới:

```text
                 DHT11
                   │
                   ▼
              ┌─────────┐
              │  STM32  │
              └────┬────┘
                   │
              UART │
                   ▼
              ┌─────────┐
              │ CP2102  │
              └────┬────┘
                   │ USB
                   ▼
              ┌─────────┐
              │   PC    │
              └────┬────┘
                   │
            Serial application
                   │
                   ▼
        ┌─────────────────────┐
        │ Temperature : 28 C  │
        │ Humidity    : 67 %  │
        │ Status      : NORMAL │
        └─────────────────────┘
```

Sau đó có thể mở rộng thành:

```text
STM32
  │
  ├── DHT11
  │
  ├── UART
  │
  ├── Timer
  │
  └── Command parser
          │
          ▼
         PC
          │
          ▼
      Python tool
          │
          ▼
      Data logging
          │
          ▼
       Dashboard
```

---

## 15. Kiến thức đạt được sau project

Sau khi hoàn thành project, mục tiêu là có thể giải thích và thực hành được:

- C programming trong embedded
- GPIO
- UART
- Digital sensor communication
- Timing
- Checksum
- Interrupt/Timer ở mức cơ bản
- Modular C
- Header/source organization
- Cross compilation
- ARM GCC
- CMake
- Ninja/Make
- Serial debugging
- Git workflow
- Firmware build và flash

Đây là project thực hành cá nhân, mục tiêu chính là xây dựng một workflow gần với môi trường phát triển Embedded C thực tế thay vì chỉ phụ thuộc vào một IDE.
