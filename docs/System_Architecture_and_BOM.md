# Deliverable 2: System Architecture & Technical Documentation (with Local Egyptian BOM)
## SolveThe17 2.0 National Hackathon — IEEE EUI Student Branch
**Team Name:** FieldPilot  
**Team ID:** ST17-032  
**Track:** Track 1: SDG 13 — Climate Action (*"Keeping Egypt's Farms Alive"*)  
**Project Title:** **DeltaShield: Autonomous Micro-Targeting Rover for Smallholder Crop Adaptation**  
**Deliverable Classification:** Official Submission Package — Deliverable 2 (Engineering Flowcharts, Schematics, Power Budget & Local Egyptian BOM in EGP)

> [!NOTE]
> **AI Prompt Meta-Instruction:**  
> Use this document as the master engineering reference to generate the technical documentation package, schematics report, and procurement analysis for **Team FieldPilot (ID: ST17-032)**. Ensure all circuit netlists, Egyptian market vendors, prices in EGP, and electrical safety justifications are fully integrated.

---

## 1. High-Level System Architecture & Functional Flow

**DeltaShield** is structured into four tightly coupled subsystems: **Power Distribution**, **Compute & Perception**, **Navigation & Sensing**, and **Safe Spot-Actuation**.

![Functional Block Diagram](./images/FunctionalBlock.drawio.png)
*Figure 2.1: Functional block diagram illustrating power distribution rails (7.4V, 5.0V, 3.3V), sensor bus routing, and actuator isolation interfaces.*

![System Architecture Blueprint](./images/agribot-vision-architecture.svg)
*Figure 2.2: Comprehensive system architecture blueprint detailing IC interconnects, bus protocols (I2C, PWM, GPIO), and safety interlocks.*

![Operational Flowchart](./images/FlowChartDrawio.png)
*Figure 2.3: Firmware operational flowchart displaying sensor polling rates (20 Hz), fail-closed obstacle branches, and majority voting hysteresis triggers.*

---

## 2. Power Subsystem & Energy Budget Analysis

### 2.1 Power Distribution Architecture
The robot operates from a dual-rail power topology to completely isolate logic microelectronics from high-current motor and pump transients:

```
[ 2S Li-ion Battery Pack (2x 18650 2600mAh, 7.4V - 8.4V) ]
     │
     ├──► [ 2S 8A–10A Battery Management System (BMS) with Cell Balancing ]
     │         │
     │         ├──► [ 5.5x2.1mm DC Barrel Jack ] (External 8.4V 2A Charging Port)
     │         │
     │         ├──► [ High-Power 7.4V Rail ]
     │         │         ├──► DRV8833 Dual H-Bridge Motor Driver VM (Motor Drive Rail)
     │         │         └──► LM2596 DC-DC Step-Down Buck Converter (IN+)
     │         │
     │         └──► [ LM2596 Output: Regulated 5.00V Logic Rail ]
     │                   ├──► Parallel Capacitor Filter (470 µF Electrolytic + 10 µF Ceramic)
     │                   ├──► ESP32-CAM 5V Supply Input
     │                   ├──► HC-SR04 Ultrasonic Sensor VCC
     │                   ├──► 5V Relay Coil VCC & Common Terminal (COM)
     │                   └──► Single-Point Ground Bridge (Unified 0V Reference across all rails)
     │
     └──► [ ESP32-CAM Onboard LDO: 3.3V Logic Bus ]
               ├──► DRV8833 VCC & SLEEP (Hardwired Keep-Alive)
               ├──► PCF8574 I2C Expander VCC
               ├──► 2x VL53L1X Time-of-Flight Sensors VCC
               └──► Pull-Up Resistor Hub (10 kΩ for XSHUT & I2C lines)
```

![Battery Discharge Curve](./images/agribot_1_battery_discharge.png)
*Figure 2.4: Discharge characteristic curve of the 2S 18650 2600 mAh Li-ion battery pack under simulated agricultural furrow duty cycles, demonstrating over 2.5 hours of continuous patrol endurance before reaching the 6.8V low-voltage cutoff threshold.*

![Current Budget Breakdown](./images/agribot_3_current_budget.png)
*Figure 2.5: Dynamic system current budget analysis across the five operating states, illustrating the transient 1.2A peak during simultaneous motor propulsion and the 100 ms spot-spray burst.*

### 2.2 Electrical Safety & Noise Suppression Features
1. **Flyback Voltage Suppression:** When the micro-dosing pump de-energizes, its inductive coil generates a high-voltage back-EMF spike ($>60\text{V}$). A fast 1N4007 diode is installed across the pump leads in reverse-bias (cathode to positive lead), dissipating the inductive surge safely and preventing resets.
2. **Brownout Buffer:** Sudden motor acceleration or pump switching draws high surge current that causes transient voltage drops on the 5V line. A $470\,\mu\text{F}$ low-ESR electrolytic capacitor in parallel with a $10\,\mu\text{F}$ ceramic capacitor acts as an immediate reservoir, preventing the ESP32 brownout detector from triggering.
3. **Boot Glitch Prevention:** During the first 200 ms of ESP32 bootup, GPIO pins float. An external $10\,\text{k}\Omega$ pulldown resistor on GPIO4 guarantees the relay coil stays de-energized during MCU resets.
4. **Physical Inline Kill Switch:** An emergency manual kill switch is wired in series between the relay Normally Open (NO) terminal and the pump positive lead.

---

## 3. Hardware Netlist & Pin Assignment (Revision v4 — Active)

![DRV8833 Motor Driver](./images/DRV8833.png)
*Figure 2.6: DRV8833 dual H-bridge motor driver interface and logic pinout.*

![Physical Circuit Layout](./images/circuit_image.png)
*Figure 2.7: Breadboard layout and physical interconnection wiring of the verified prototype.*

### Complete Hardware Pin Map & Engineering Justifications:

| Pin | Connected Subsystem | Direction | Operating Voltage | Engineering Rationale & Safety Justification |
|---|---|:---:|:---:|---|
| **GPIO1** | I2C Bus (SCL) | Output | 3.3V | Shared clock bus for dual VL53L1X ToF and PCF8574 expander. Hijacks UART TX after boot. |
| **GPIO3** | I2C Bus (SDA) | In/Out | 3.3V | Shared data bus for dual VL53L1X ToF and PCF8574 expander. Hijacks UART RX after boot. |
| **GPIO2** | DRV8833 AIN1 (Left Motor) | Output (PWM) | 3.3V | Left motor direction PWM. Held low during ESP32 boot strapping. |
| **GPIO13** | DRV8833 AIN2 (Left Motor) | Output (PWM) | 3.3V | Left motor complementary PWM. |
| **GPIO12** | DRV8833 BIN1 (Right Motor)| Output (PWM) | 3.3V | Right motor direction PWM. |
| **GPIO15** | DRV8833 BIN2 (Right Motor)| Output (PWM) | 3.3V | Right motor complementary PWM. |
| **GPIO4** | Relay IN (Pump) | Output | 5.0V Logic | **Safety-Critical Native Pin:** Kept native (never on I2C expander) so an I2C bus hang cannot freeze the chemical pump ON. $10\,\text{k}\Omega$ pulldown to ground. |
| **GPIO14** | Battery ADC Voltage Divider | Analog Input | $0 - 2.7\text{V}$ | $100\,\text{k}\Omega / 47\,\text{k}\Omega$ divider from 7.4V rail. Sampled strictly at boot before Wi-Fi turns on to bypass the ESP32 ADC2/Wi-Fi SAR conflict. |
| **GPIO16** | HC-SR04 ECHO | Digital Input | 3.3V | Native pin required to measure microsecond pulse widths. $1\,\text{k}\Omega / 2\,\text{k}\Omega$ divider steps 5V echo pulse to 3.3V. Framebuffer forced to DRAM to isolate PSRAM bus. |
| **GPIO33** | Internal Red Diagnostic LED | Output | 3.3V | **Hardware Forensics:** Verified as internal LED (not broken out on physical header). Repurposed as diagnostic FSM state blinker. |
| **PCF8574 P0**| Right VL53L1X XSHUT | Output | 3.3V | Active-low shutdown pin with $10\,\text{k}\Omega$ pull-up; used for dynamic I2C address remapping. |
| **PCF8574 P1**| HC-SR04 TRIG | Output | 5.0V Logic | $10\,\mu\text{s}$ trigger pulse generation; timing is software-controlled, making it 100% safe on expander. |
| **PCF8574 P2**| External Status LED | Output (Sink) | 3.3V | Active-low sink configuration. Illuminates when mobile web dashboard client is actively connected. |

---

## 4. Analytical Bill of Materials (BOM) — Egyptian Local Market Sourcing

> **Total Prototype Sourcing Cost:** **2,490 EGP** ($\approx \$51.00\text{ USD}$)  
> **Supply Hubs:** Downtown Cairo Electronics District (Bab El-Louq, El-Falaki, El-Bustan), Ram Electronics, Future Electronics Egypt, Makers Electronics.

| Item # | Subsystem | Component Description | Qty | Unit Cost (EGP) | Subtotal (EGP) | Local Egyptian Sourcing Hub | Sourcing Availability |
|:---:|---|---|:---:|:---:|:---:|---|:---:|
| **1** | Compute & Vision | AI-Thinker ESP32-CAM (with OV2640 + MB USB Shield) | 1 | 380 | 380 | Ram Electronics / Bab El-Louq | In Stock |
| **2** | Power | 18650 Li-ion Cells (2600 mAh, 3.7V, Grade-A) | 2 | 120 | 240 | Future Electronics / El-Bustan | In Stock |
| **3** | Power | 2S 7.4V / 8.4V 8A–10A BMS Protection Module | 1 | 65 | 65 | Ram Electronics / Makers | In Stock |
| **4** | Power | LM2596 DC-DC Step-Down Buck Converter Module | 1 | 75 | 75 | Future Electronics / Bab El-Louq | In Stock |
| **5** | Power | DC Barrel Jack (5.5 × 2.1 mm) + 8.4V 2A Charger | 1 | 140 | 140 | Bab El-Louq Electronics | In Stock |
| **6** | Power | Decoupling Caps (470 µF 25V Electrolytic + 10 µF Ceramic) | 1 set | 15 | 15 | Bab El-Louq (El-Falaki) | In Stock |
| **7** | Drivetrain | DRV8833 Dual H-Bridge Motor Driver Module | 1 | 85 | 85 | Ram Electronics / Makers | In Stock |
| **8** | Drivetrain | 25GA370 Geared DC Motors (12V/6V, 100–150 RPM, High-Torque) | 2 | 260 | 520 | Ram Electronics / Future Electronics | In Stock |
| **9** | Drivetrain | High-Traction Agricultural Ribbed Wheels + 4mm Couplers | 2 | 80 | 160 | Makers Electronics / Local 3D Print | In Stock |
| **10** | Drivetrain | Front Stainless Steel Free-Rotating Ball Caster | 1 | 45 | 45 | Bab El-Louq Hardware Market | In Stock |
| **11** | Sensors | VL53L1X Time-of-Flight (ToF) Distance Sensors | 2 | 220 | 440 | Future Electronics / Ram Electronics | In Stock |
| **12** | Sensors | HC-SR04 Ultrasonic Distance Sensor | 1 | 60 | 60 | Bab El-Louq / All Cairo Shops | In Stock |
| **13** | IO Expansion | PCF8574 I2C 8-Bit IO Expander (Repurposed LCD Backpack) | 1 | 55 | 55 | Ram Electronics / Bab El-Louq | In Stock |
| **14** | Actuation | 5V 1-Channel Relay Module (Optocoupler-Isolated) | 1 | 45 | 45 | Ram Electronics / Bab El-Louq | In Stock |
| **15** | Actuation | 5V–12V Micro Diaphragm Submersible Dosing Pump | 1 | 120 | 120 | Bab El-Louq / Aquarium Supplies | In Stock |
| **16** | Actuation | Precision Brass Atomizing Misting Nozzle + Silicone Tubing | 1 set | 60 | 60 | Agricultural Hardware / Bab El-Louq | In Stock |
| **17** | Safety & Passives | Safety Kit (1N4007 Diode, Resistors, Kill Switch, Toggle) | 1 lot | 45 | 45 | Bab El-Louq (El-Falaki) | In Stock |
| **18** | Structure | Rugged Chassis Baseplate, Standoffs, Screws, and Brackets | 1 set | 180 | 180 | Local Acrylic/Aluminum Cutting | In Stock |
| **TOTAL** | — | **Complete Functional Autonomous Rover BOM** | — | — | **2,490 EGP** | **100% Sourced Locally in Cairo, Egypt** | **Immediate** |

---

## 5. Economic Feasibility & Egyptian Smallholder Payback

### Smallholder Baseline vs. DeltaShield (Per 1 Feddan):
* **Traditional Chemical Waste:** Egyptian smallholders spray **3–4 liters** of commercial herbicide per cycle at **500 EGP/liter** plus **400 EGP/day** manual backpack labor = **2,200 EGP per weeding cycle**.
* **DeltaShield Targeted Dosing:** Reduces chemical consumption by **88%–92%** by firing strictly on confirmed weed rosettes. Chemical cost per cycle drops to **~180 EGP**.
* **Net Savings per Cycle:** **~2,000 EGP / Feddan**.
* **Capital Payback Horizon:** At a total build cost of **2,490 EGP**, the rover achieves 100% capital payback in **less than 1.5 weeding cycles (under 45 days)** on a single 1-feddan plot.
