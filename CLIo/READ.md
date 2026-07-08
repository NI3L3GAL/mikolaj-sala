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

### 🐛 Challenges & Hardware Limitations

Building a bare-metal system from scratch exposed several fascinating hardware quirks:

* **115200 bps on a 16 MHz Clock:** Generating exactly 115200 bps with a 16 MHz oscillator is mathematically impossible on AVR. It results in a +2.1% timing error, which causes frame drops when transmitting longer strings. To ensure 100% stability, the maximum supported baud rate was capped at `57600 bps`.
* **Race Conditions (Atomicity):** Reading a 32-bit time variable (`ms_tick`) on an 8-bit architecture takes 4 separate processor operations. If a timer interrupt fired mid-read, the variable became corrupted. This was solved by wrapping the read operation in an atomic block (saving `SREG` and using `cli()`).
* **Blocking inside ISR:** Initially, echoing characters back to the terminal inside the RX interrupt caused the processor to freeze when a full string (like `BLINK 200`) was pasted at once. The solution was keeping the ISR strictly for buffering and raising state flags.

### 🔮 Future Improvements

While the current architecture is robust, there is always room for expansion:

* **Circular Buffer (Ring Buffer):** Upgrading the linear RX buffer to an interrupt-driven ring buffer to completely eliminate the risk of data overrun during heavy UART traffic.
* **Interrupt-Driven TX:** Currently, `UART_transmit` uses active polling (`while`). Implementing the `UDRE` (USART Data Register Empty) interrupt would make sending data 100% non-blocking.
* **Hardware PWM (Fading):** Utilizing another hardware timer (e.g., `Timer2`) to introduce a `FADE` command, smoothly dimming the LED via Pulse Width Modulation instead of simple binary toggling.

### 📫 How to Run (Quick Start)?

1. Flash the program onto your microcontroller.
2. Open a serial terminal (e.g., RealTerm, minicom, PuTTY).
3. Set one of the supported baud rates (e.g., `57600`).
4. Reset the MCU and **send an uppercase `U` character**.
5. When you see the `--- AUTO-BAUD OK! ---` message, the CLI is synchronized and ready for operation.

---
