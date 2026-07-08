# ATmega Bare-Metal CLI & Auto-Baud UART 🚀

A lightweight, interrupt-driven Command Line Interface (CLI) for AVR microcontrollers (e.g., ATmega328PB). The project is written in pure C (bare-metal), completely bypassing the Arduino framework. It demonstrates how to optimize processor performance by getting rid of blocking functions like `_delay_ms()`.

### 🚀 Key Features

* 🔄 **Auto-Baud Rate:** The microcontroller automatically detects the terminal's baud rate (57600, 38400, 19200, 9600 bps) based on a single synchronization character (`U`).
* ⚡ **Non-blocking Architecture:** Characters are received in the background via the `USART0_RX_vect` interrupt. The main loop only reacts when the entire command is ready.
* ⏱️ **Asynchronous Timer:** Hardware `Timer1` operates in CTC mode, creating an independent time counter (similar to `millis()`). This allows for background tasks, like LED blinking, while the CLI continuously listens.
* 🧠 **Lightweight Custom Parser:** Instead of the heavy `<string.h>` library, I wrote custom, optimized functions (`strcomp`, `starts_with`) to maximize Flash memory savings.

### 🛠️ Technology Stack & Architecture

**Hardware & Ecosystem**
* AVR family microcontrollers (e.g., Arduino Uno)
* C (avr-gcc), direct hardware register manipulation
* Core libraries: `<avr/interrupt.h>`, `<avr/boot.h>`

**Key Mechanisms**
* Hardware Interrupt Service Routines (ISR) for asynchronous event handling
* Manual configuration of hardware peripherals (UART, Timers)
* Reading the unique MCU ID (Boot Signature)

### 💻 Available CLI Commands

All commands must be terminated with a newline character (Enter key).

| Command | Description |
| :--- | :--- |
| `LED on` / `LED off` | Turns the test LED (connected to PB5) on or off. |
| `PING` | Returns the hardware serial number of the MCU (hex). |
| `BLINK <ms>` | Starts hardware LED blinking in the background every X milliseconds (e.g., `BLINK 500`). The microcontroller does not freeze and continues to receive commands! |
| `BLINK off` | Stops the asynchronous blinking. |

### 📫 How to Run (Quick Start)?

1. Flash the program onto your microcontroller.
2. Open a serial terminal (e.g., RealTerm, minicom, PuTTY).
3. Set one of the supported baud rates (e.g., `57600`).
4. Reset the MCU and **send an uppercase `U` character**.
5. When you see the `--- AUTO-BAUD OK! ---` message, the CLI is synchronized and ready for operation.

---
