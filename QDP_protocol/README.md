# QDP (Quad Data Protocol) 🚀

QDP is a custom, high-performance embedded communication protocol designed primarily for STM32 microcontrollers[cite: 5]. It features a physical layer utilizing a 4-state logic approach (similar to PAM-4 signaling)[cite: 3, 5] and a robust Data Link (MAC) layer that ensures data integrity and delivery verification[cite: 4].

The project is built with a strong focus on testability, allowing the entire logical stack to be compiled and unit-tested on a PC without requiring physical hardware[cite: 5].

---

## ✨ Key Features

* **Advanced Physical Layer (PHY):** Utilizes hardware EXTI for preamble phase alignment, Timers, and DMA for zero-CPU-overhead transmission and reception[cite: 5].
* **4-State Logic Modulation:** Transmits data using a Look-Up Table (LUT) mapped directly to GPIO BSRR registers, representing 4 distinct voltage states per baud[cite: 3, 5].
* **Quality of Service (QoS):** Supports three distinct message delivery priorities (Fire-and-forget, ACK required, and Responses)[cite: 3].
* **Multi-Channel Support:** Up to 64 virtual channels (including dedicated System and ACK channels)[cite: 3].
* **Data Integrity:** Every frame is protected by a 4-byte (32-bit) CRC sum[cite: 3, 4].
* **TDD Ready:** Includes a `PC_TEST_ENV` macro that safely mocks hardware registers, allowing seamless integration with PC-based unit testing frameworks like Unity[cite: 5].

---

## 📦 Frame Structure

The QDP frame consists of a 5-byte header, a variable-length payload, and a 4-byte CRC[cite: 3]. The absolute maximum frame size is 264 bytes[cite: 3].

| Field | Size | Description |
| :--- | :--- | :--- |
| **Preamble** | 1 Byte | Synchronization byte (`0xCC`)[cite: 3]. Used by PHY (EXTI) to align the timer[cite: 5]. |
| **Magic Word** | 1 Byte | Protocol identifier (`0xA1`) used as a first-line frame validation[cite: 3, 4]. |
| **QoS + Channel**| 1 Byte | 2 bits for QoS level, 6 bits for virtual channel ID[cite: 3]. |
| **Sequence** | 1 Byte | Rolling sequence number (`seq`) for tracking packets and retransmissions[cite: 3, 4]. |
| **Length** | 1 Byte | Size of the payload (`0` to `255` bytes)[cite: 3]. |
| **Payload** | 0-255 Bytes| The actual application data[cite: 3]. |
| **CRC** | 4 Bytes | 32-bit Cyclic Redundancy Check covering Header and Payload[cite: 3, 4]. |

---

## 🏗️ Architecture & Layers

### 1. Physical Layer (`qdp_phy.c` / `qdp_phy.h`)
Responsible for raw electrical signaling. 
* **Tx:** Translates bytes into 2-bit symbols and pushes them to transmission pins via DMA and a predefined LUT[cite: 5].
* **Rx:** Uses an EXTI interrupt on the first preamble edge to align the ADC/DMA timer[cite: 5]. Decodes incoming analog states into a clean byte stream[cite: 5].

### 2. Data Link Layer (`qdp_dl.c` / `qdp_dl.h`)
Acts as the MAC layer.
* **Validation:** Immediately discards frames with invalid Preambles or Magic Words before processing[cite: 4].
* **Integrity:** Calculates and verifies CRC[cite: 4].
* **Routing:** Parses QoS flags. If an ACK is required (`QDP_QoS_1_REQ`), it automatically generates and dispatches an acknowledgment frame via the ACK channel[cite: 4]. Otherwise, it passes the payload up to the application via `APP_Process_Incoming_Data`[cite: 4].

---

## 🚥 Quality of Service (QoS) Levels

| QoS Flag | Name | Behavior |
| :---: | :--- | :--- |
| `0b00` | **QDP_QoS_0_FnF** | Transmits data without expecting any acknowledgment (Fire-and-Forget)[cite: 3]. |
| `0b01` | **QDP_QoS_1_REQ** | Requires an ACK from the receiver. Prepares the MAC layer for potential retransmission[cite: 3, 4]. |
| `0b10` | **QDP_QoS_2_RESP** | Used internally for protocol responses (e.g., sending ACK/NACK frames)[cite: 3]. |

---

## 🧪 PC Unit Testing

This protocol was built with test-driven development in mind. By defining the `PC_TEST_ENV` macro during compilation, the hardware-specific code (STM32 LL drivers, ISRs, EXTI, DMA) is entirely compiled out[cite: 5].

To run unit tests on a host machine (Windows/Linux):
1. Use a C11 compatible compiler (GCC, MSVC).
2. Define the `PC_TEST_ENV` macro flag during the build process[cite: 5].
3. Link the source files with your preferred testing framework (e.g., Unity).
4. Mock the `PHY_Send` and `APP_Process_Incoming_Data` functions to verify logic flow[cite: 4, 5].

> **Note:** The `remaining_bytes_to_read` variable is globally accessible to facilitate synchronization between the Header state and Payload state during PC simulations[cite: 5].