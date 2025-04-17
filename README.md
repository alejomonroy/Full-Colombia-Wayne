
# Full Colombia Wayne – Communication Firmware for Wayne Fuel Dispensers

This repository contains the firmware developed for electronic control boards used by **FULL Colombia SAS**. These boards enable communication with **Wayne** fuel dispensers (Vista 3 series and newer) using the proprietary **DART Pump Interface** protocol over an RS485 bus. The implementation targets low-power embedded systems, particularly the **ATmega328P** microcontroller, and communicates with a master system via **I2C**.

---

## 📘 Table of Contents

1. [Project Overview](#project-overview)
2. [Implemented Hardware](#implemented-hardware)
3. [Communication Topology](#communication-topology)
4. [Wayne Protocol Description (DART Pump Interface)](#wayne-protocol-description-dart-pump-interface)
   - 4.1 [Protocol Levels](#41-protocol-levels)
   - 4.2 [Device Addressing](#42-device-addressing)
   - 4.3 [Transaction Types](#43-transaction-types)
5. [Message Frame Structure](#message-frame-structure)
6. [Supported Transactions](#supported-transactions)
7. [I2C Interface with Master Controller](#i2c-interface-with-master-controller)
8. [Code Structure](#code-structure)
9. [Reverse Engineering and Validation](#reverse-engineering-and-validation)
10. [Future Improvements](#future-improvements)
11. [Author](#author)

---

## 📌 Project Overview

This firmware enables remote control and monitoring of Wayne dispensers, allowing for reading statuses, initiating and ending fueling transactions, updating prices, and retrieving critical data such as volume dispensed, sale value, and totals. Initially developed through reverse engineering, the implementation was later validated against the official Wayne protocol manual.

---

## 🧰 Implemented Hardware

- **Microcontroller: ATmega328P (Pro Mini)** – Runs the Wayne protocol stack.
- **RS485 Transceiver: SP485** – Handles physical communication with dispensers.
- **UART Interface** – Used for RS485 protocol.
- **I2C Bus** – For communication with a master controller (e.g., ATmega2560).

---

## 🌐 Communication Topology

```
          ┌──────────────────────┐
          │          Main        │
          │    Microcontroller   │◄────────────┐
          └──────────────────────┘      Communication I2C
                                               │
                                               ▼
                                      ┌────────────────────┐
                                      │   Protocol + SP485 │
                                      │   Firmware Wayne   │
                                      └────────────────────┘
                                           UART ⇅ RS485
                                  ┌──────────────┴─────────────┐
                                  │   Dispensadores Wayne (x6) │
                                  └────────────────────────────┘
```

---

## 📡 Wayne Protocol Description (DART Pump Interface)

The **DART Pump Interface** defines the communication protocol between a site controller and Wayne dispensers. It is composed of three layers:

### 4.1 Protocol Levels

1. **Level 1 – Physical**: RS485 Half-Duplex  
2. **Level 2 – Line Protocol**: CRC, sequence counters, delimiters  
3. **Level 3 – Application Layer**: High-level transactions (e.g. `AUTHORIZE`, `RESET`, `RETURN STATUS`)

### 4.2 Device Addressing

Each dispenser side has a unique address in the range `0x50` to `0x6F`. Devices only respond to frames addressed to them.

### 4.3 Transaction Types

Messages between controller and dispenser are organized into *transactions*. A frame may include multiple transactions. Key types include:

- Commands (`CD1`, `CD2`, ..., `CD101`)
- Replies (`DC1`, `DC2`, ..., `DC101`)

---

## 🔠 Message Frame Structure

Wayne message frames have the following structure:

```plaintext
[ADR] [CTRL] [DATA ...] [CRC-1] [CRC-2] [ETX] [SF]
```

Where:

- **ADR**: Device address (0x50–0x6F)
- **CTRL**: Control or sequence byte
- **DATA**: Transaction content (e.g. `01 01` for RETURN STATUS)
- **CRC-1/2**: CRC-16 (Modbus-like)
- **ETX**: End-of-text (0x03)
- **SF**: Stop Frame (0xFA)

The **CRC** is calculated from `ADR` through the end of `DATA`.

---

## ✅ Supported Transactions

### From Controller to Dispenser

| Code   | Description                         |
|--------|-------------------------------------|
| `CD1`  | RETURN STATUS, AUTHORIZE, RESET... |
| `CD2`  | Allowed nozzle numbers              |
| `CD3`  | Preset volume                       |
| `CD4`  | Preset amount                       |
| `CD5`  | Price update                        |
| `CD101`| Request total counters              |

### From Dispenser to Controller

| Code   | Description                             |
|--------|-----------------------------------------|
| `DC1`  | Dispenser status                        |
| `DC2`  | Filled volume and amount                |
| `DC3`  | Nozzle status and price                 |
| `DC9`  | Dispenser identity                      |
| `DC101`| Total counters per nozzle               |

Values such as price and volume are encoded in **packed BCD format**, with **MSB sent first**.

---

## 🔁 I2C Interface with Master Controller

This firmware acts as an I2C slave device. The main controller can:

- Request numerical totals  
- Send price updates  
- Set authorization data  
- Retrieve finalized transactions  

Key data structures:

- `i2cFuncion`: Indicates the requested operation (e.g. `NUMERACION`, `PRECIOS`)
- `funAuth[3][2]`: Pending authorization data
- `PPUArray[3][2][4]`: Price matrix by hose

All I2C logic is handled within the function `LoopI2C_Comunicacion()`.

---

## 🧠 Code Structure

- `ProMini_Protocolo.ino`: Main loop and logic coordination
- `ProMini_Protocolo.h`: Utilities (EEPROM, conversions, prints)
- `Protocolo_Wayne.h`: Core protocol logic (frame construction, parsing, CRC)
- `Variables.h`: Data structures for sales, states, I2C buffers
- `Wayne protocol.pdf`: Official Dart Protocol Specification

---

## 🧪 Reverse Engineering and Validation

Initially, the protocol was reverse-engineered using RS485 sniffing tools and trace analysis, identifying key command patterns like `51 35 01 01 00`. Later, the official Wayne Dart Protocol Specification was obtained and used to verify and correct the implementation, especially in CRC validation, message flow, and frame timing.

---

## 🔮 Future Improvements

- Migrate to modern 32-bit architectures (STM32, ESP32)
- Introduce multitasking with FreeRTOS
- Support additional protocols (Gilbarco, Tokheim)
- Add embedded web server or MQTT bridge via ESP32
- Event logging on SD card or remote SQL database

---

## 👨‍💻 Author

**Luis Alejandro Monroy**  
Electronic engineer specializing in embedded systems and industrial instrumentation.  
[LinkedIn](https://www.linkedin.com/in/alejandro-monroy-dev) | [GitHub](https://github.com/alejomonroy)

---
