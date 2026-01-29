# 🎚️ Audio Preamplifier with Adjustable Gain and 3-Band Tone Control

High-quality analog audio preamplifier designed and simulated in **LTspice**, featuring adjustable gain and an active three-band tone control (Bass, Mid, Treble).  
Project developed as part of the **Analog Electronic Circuits** course at AGH University of Science and Technology.

---

## 📌 Project Overview

This project implements a **high-fidelity audio preamplifier** with:

- Adjustable voltage gain (20–40 dB)  
- Independent 3-band tone control:
  - **Bass** – 100 Hz  
  - **Mid** – 1 kHz  
  - **Treble** – 10 kHz  
- Parallel active filter topology  
- Final summing amplifier stage  
- Dual supply operation (±15 V)  

The design was verified using:
- AC (frequency response) analysis  
- Transient + THD analysis  
- Monte Carlo tolerance analysis  
- PCB layout and 3D visualization  

---

## 🧱 System Architecture

The signal chain consists of three main stages:

1. **Input & Variable Gain Stage**  
   Non-inverting amplifier with adjustable gain (10–100 V/V).

2. **Parallel Active Filter Network**  
   The input signal is split into three independent paths:
   - Low-Pass (Bass)
   - Band-Pass (Mid)
   - High-Pass (Treble)

3. **Summing Amplifier**  
   Filtered signals are recombined using an inverting summing amplifier.

---

## 🔧 Design Requirements

| Parameter | Target |
|----------|--------|
| Gain range | 10 – 100 V/V (20 – 40 dB) |
| Bass center | 100 Hz |
| Mid center | 1 kHz |
| Treble center | 10 kHz |
| Tone adjustment | ≥ ±10 dB per band |
| THD @ 1 kHz, 1 Vpp | < 0.5 % |
| Supply voltage | ±15 V |

---

## 🧮 Filter Design

Each band is implemented using active RC filters:

### Low-Pass (Bass)

Cutoff frequency:

$f_c = \frac{1}{2\pi R_L C_L}$

Designed for **100 Hz**.

---

### Band-Pass (Mid)

Center frequency:

$f_0 = \frac{1}{2\pi R_M C_M}$

Designed for **1 kHz**.

---

### High-Pass (Treble)

Cutoff frequency:

$f_c = \frac{1}{2\pi R_H C_H}$

Designed for **10 kHz**.

Standard capacitor values were selected first, and resistor values were derived and matched to E96 series.

---

## 🔬 LTspice Simulations

The project includes full LTspice testbenches:

### ✔ AC Analysis

- Confirms correct band separation  
- Gain slopes ≈ 40 dB/dec  
- Proper crossover between bands  


---

### ✔ Transient + THD Analysis

Input: 1 kHz, 10mVpp sinusoid  

Results:

| Parameter | Requirement | Simulated |
|----------|-------------|-----------|
| THD | < 0.5 % | **0.05 %** |
| Stability | Stable after ~12 ms | Confirmed |

---

### ✔ Monte Carlo Analysis

Component tolerances included:
- Resistors ±1 %  
- Capacitors ±10 %  

Confirms robustness of frequency response across variations.

---

## 🖥️ Hardware Implementation

- **Operational amplifiers:** OPA1678 (final hardware), OPAx134 (simulation models)  
- **Controls:**  
  - Main gain  
  - Bass  
  - Mid  
  - Treble  

- **Power supply:**  
  - ±15 V analog rails  
  - Additional ±5 V rail via USB  

### PCB

Designed in **Altium Designer**, 2-layer board with solid ground planes.


---

## ⚠️ Known Issues

Although simulations met all requirements, the assembled hardware did not fully operate:

- Summing amplifiers failed due to likely potentiometer damage  
- One OPA1678 was permanently damaged  

---

## 📁 Repository Structure

├── README.md \
├── LICENSE \
│ \
├── img/             # Plots, diagrams, PCB renders \
├── spice/           # LTspice simulations and models \
│     ├── models/ \
│     ├── ac/ \
│     ├── transient/ \
│     └── monte_carlo/ \
|  
├── pcb/              # Schematics, layout, gerbers, 3D views \
└── documentation/          # report sources


---

## 🚀 How to Run Simulations

1. Install **LTspice**
2. Clone the repository
3. Open selected `.cir` file from `spice/`
4. Make sure `OPAx134.lib` is included in the working directory
5. Run:
   - `ac_preamp.cir` for frequency response  
   - `tran_preamp.cir` for transient + THD  
   - `monte_preamp` for Monte Carlo  

---

## 👨‍🎓 Authors

- **Mikołaj Sala**  
- **Kacper Kochanek**  

Electronics and Telecommunications  
AGH University of Science and Technology  

---

## 📜 License

This project is provided for **educational and academic purposes**.  
Feel free to use, modify, and extend with attribution.

---

## ⭐ Acknowledgments

- AGH – Analog Electronic Circuits course  
- LTspice by Analog Devices  
- Texas Instruments OPA1678 / OPAx134 models  

