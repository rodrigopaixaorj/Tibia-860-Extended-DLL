# Tibia 8.60 Extended Client DLL (`ddraw.dll`)

[![Platform](https://img.shields.io/badge/Platform-Windows%20x86%20(32--bit)-blue.svg)](#requirements)
[![Client](https://img.shields.io/badge/Target-Tibia%208.60-orange.svg)](#overview)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Build](https://img.shields.io/badge/Build-CMake%20%7C%20MSVC-informational.svg)](#building-from-source)

A high-performance, modular native extension DLL for the classic **Tibia 8.60 client (`Tibia.exe`)**. Designed for OpenTibia (OTServ) developers and players, it upgrades the legacy client with modern graphics capabilities, uncapped gameplay limits, custom protocol features, and modern operating system optimizations—**without requiring executable disassembly or permanent binary modification**.

---

## Table of Contents

- [Overview & Purpose](#overview--purpose)
- [Key Features](#key-features)
- [How It Works (Technical Architecture)](#how-it-works-technical-architecture)
  - [Zero-Injector DLL Proxying](#1-zero-injector-dll-proxying-ddrawdll)
  - [In-Memory Dynamic Hooking](#2-in-memory-dynamic-hooking)
  - [Hardware Timer Synchronization](#3-hardware-timer-synchronization)
- [Technical Specifications & Client Offsets](#technical-specifications--client-offsets)
- [Extended Network Protocol (Opcode 0x32)](#extended-network-protocol-opcode-0x32)
- [Configuration (`config.ini`)](#configuration-configini)
- [Building from Source](#building-from-source)
- [Installation & Quick Start](#installation--quick-start)
- [Security & Stability](#security--stability)
- [Credits & Acknowledgements](#credits--acknowledgements)
- [License](#license)

---

## Overview & Purpose

The original Tibia 8.60 client was released in 2010. While still the most popular protocol version in the OpenTibia ecosystem, the legacy client suffers from structural limitations:
- Hardcoded sprite limit (capped at `65,535` 16-bit IDs).
- Lack of native 32-bit ARGB alpha transparency (causing magenta color-key fringes and preventing smooth lighting/shadows).
- 16-bit player HP/Mana limits (`65,535`) and 8-bit limits on magic effects (`255`) and skills (`255`).
- Incompatibility with modern Tibia `.dat` file structures (attributes, frame groups, animations).
- Severe frame stuttering on modern Windows 10 and Windows 11 due to low-resolution `timeGetTime` scheduling.

**Tibia 8.60 Extended DLL** solves these limitations by acting as a native middleware wrapper (`ddraw.dll`). It patches client logic directly in memory at runtime, enabling server administrators and client developers to introduce modern features while preserving the authentic feel and performance of the C++ native client.

---

## Key Features

### 🎨 Graphics & Rendering Engine
- **Full 32-bit ARGB Alpha Transparency**: Intercepts texture rendering in both **OpenGL** and **DirectX 9** pipelines, replacing binary magenta transparency with full 8-bit alpha channel blending (`GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`).
- **Extended Sprites (> 65,535)**: Re-engineers sprite indexing from `uint16_t` to `uint32_t`, raising the sprite ceiling to over 4 billion sprites.
- **In-Memory RAM Sprite Cache**: Caches `Tibia.spr` (up to 250 MB) into RAM to eliminate disk I/O bottlenecks and stutter during fast movement and dense map rendering.
- **DirectDraw 7 Upgrade**: Automatically updates legacy DirectDraw calls to DirectDraw 7 for smooth composition on modern desktop window managers.

### ⚙️ Engine Limits Unlocked
- **Extended Player Stats**: Extends player health and mana display from `uint16_t` (max 65,535) to full `int32_t` (over 2 billion).
- **Extended Magic Effects**: Expands effect IDs from `uint8_t` (max 255) to `uint16_t` (max 65,535).
- **Extended Skills**: Increases skill levels from `uint8_t` (max 255) to `uint16_t`.
- **OpenTibia RSA Key Injection**: Replaces CipSoft's default 1024-bit RSA public key with the standard OpenTibia key, allowing connection to custom servers out-of-the-box.

### 📦 Seamless Modern `.dat` Reader
- **Integrated Modern `.dat` Parser**: Completely supersedes the client's internal `.dat` loader.
- **Backwards & Forwards Compatible**: Automatically detects signature flags and version layouts (standard 8.60 or modern 10.x/12.x formats with signature `0x4A10`).
- **Extended Attributes Support**: Parses ground speed, text lengths, light color/intensity, displacement (`dispX`/`dispY`), elevation, minimap colors, lens help, Market Data (attr 33), and modern flags.
- **Frame Groups & Animation Sequences**: Supports complex animation definitions and directional creature frame groups.

### 🖥️ UI & Quality of Life
- **On-Screen Player Mana Bar**: Renders a dedicated mana bar directly underneath the player's health bar on the game screen.
- **HUD Percentage Overlays**: Displays numerical percentages (`%d%%`) on the player health and mana status indicators.
- **Mount System Support**: Automatically handles mount layers and applies character elevation offset (`+3px` vertical axis) when riding.
- **In-Game Market System Framework**: Native binary hooks and message parser for item trading interfaces.

### ⏱️ Performance & OS Compatibility
- **High-Resolution QPC Timer**: Replaces low-precision `timeGetTime` (which runs at ~15.6ms on Windows 10/11) with Windows `QueryPerformanceCounter` hardware timers. Completely cures frame drops, micro-stutters, and input lag.

---

## How It Works (Technical Architecture)

```
+-------------------------------------------------------------+
|                          Tibia.exe                          |
|             (Native 32-bit x86 PE Executable)               |
+------------------------------+------------------------------+
                               |
                               | (Implicit or dynamic load)
                               v
+-------------------------------------------------------------+
|                 Extended DLL ('ddraw.dll')                  |
|                                                             |
|  [DirectDraw Proxy Wrapper]                                 |
|  * FakeDirectDrawCreate       ---> C:\Windows\System32\     |
|  * FakeDirectDrawEnumerate    ---> ddraw.dll (Genuine)      |
|                                                             |
|  [DllMain / InitThread]                                     |
|  * Loads 'config.ini'                                       |
|  * VirtualProtect() on text & data sections                 |
|  * Installs JMP / CALL detours & memory patches             |
|                                                             |
|  [Core Subsystems]                                          |
|  * ExtendedEngine (OGL & DX9 Sprite Hook)                   |
|  * Extended DAT Reader (Loads modern items/creatures)       |
|  * HighResolutionTimer (Hardware QPC)                       |
|  * Network Protocol (Opcode 0x32 Parser)                    |
|  * CreatureManager & ManaBar Renderer                       |
+-------------------------------------------------------------+
```

### 1. Zero-Injector DLL Proxying (`ddraw.dll`)
Standard Tibia 8.60 links to Microsoft's DirectDraw library (`ddraw.dll`). Due to the standard Windows DLL search order, executables search their local directory before checking system folders.
- The project is named `ddraw.dll` and exports the exact symbol table defined in `ddraw.def`.
- Upon being loaded by `Tibia.exe`, it dynamically queries `C:\Windows\System32\ddraw.dll` and forwards all DirectDraw API calls to the real system library (`FakeDirectDrawCreate`, `FakeDirectDrawCreateEx`, etc.).
- **Result:** No third-party injector, launcher, or debugger privilege is required. The modification activates automatically when the client launches.

### 2. In-Memory Dynamic Hooking
On `DLL_PROCESS_ATTACH`, a background thread (`InitThread`) is initialized:
- Acquires the module base address (`GetModuleHandle(NULL)`).
- Calls `VirtualProtect` to grant `PAGE_EXECUTE_READWRITE` permissions over the client's code and read-only data segments.
- Uses three primary hooking techniques:
  - **`HookCall` / `HookCallN`**: Rewrites existing `E8` relative `CALL` targets to redirect execution to extended C++ functions while preserving the stack frame.
  - **`HookJMP`**: Installs unconditional `E9` relative jumps to completely divert subroutines.
  - **Direct Memory Overwrite**: Rewrites specific opcodes, registers, pointers, and RSA keys.
- Calls `FlushInstructionCache` to ensure modified CPU instructions are invalidated and flushed across all processor cores.

### 3. Hardware Timer Synchronization
Classic Tibia relies on `winmm.dll`'s `timeGetTime()`. Under Windows 10 and 11, the default system timer resolution often drops to ~64 Hz (~15.6ms), resulting in severe visual jitter.
The DLL patches the client's internal `timeGetTime` import pointer (`0x1B85A0`), substituting it with a custom implementation based on `QueryPerformanceCounter` and `QueryPerformanceFrequency`. This provides sub-microsecond timer precision, ensuring silky-smooth 60+ FPS rendering and animation cycling.

---

## Technical Specifications & Client Offsets

The following memory addresses and offsets apply to the **clean Tibia 8.60 (v8.60) executable**:

| Target Subsystem | Address / Offset | Mechanism | Purpose |
|---|---|---|---|
| **Base VirtualProtect** | `Base + 0x1000` (`0x238000` bytes) | `VirtualProtect` | Sets `PAGE_EXECUTE_READWRITE` for dynamic hooking |
| **RSA Public Key** | `Base + 0x1B8980` | `HookMemory` | Injects 1024-bit OpenTibia RSA public key |
| **Timer (`timeGetTime`)** | `Base + 0x1B85A0` | Pointer Overwrite | Redirects to hardware `QueryPerformanceCounter` |
| **Sprite Signature** | `Base + 0xAB9C9` | `HookCall` | Custom `.spr` signature validation |
| **Sprite Count** | `Base + 0xAB9D9` | `HookCall` | Reads 32-bit count (`uint32_t`) instead of 16-bit |
| **Sprite Pointers** | `Base + 0xABA36` | `HookCall` | Extended 4-byte sprite offset calculations |
| **Load Sprite Routine** | `Base + 0xADCC9` | `HookCall` | Custom sprite decoder handling transparency |
| **Sprite Indexing** | `Base + 0x10080C` | Opcode Overwrite | Patches `ThingType::getSpriteIndex` for 32-bit sprites |
| **DAT Loader Entry** | `Base + 0x107558` / `0x107000` | `HookCall` / `HookJMP` | Reroutes `.dat` loading to modern custom parser |
| **DirectDraw 7 Upgrade**| `Base + 0x1D8840` | `OverWrite` | Replaces DirectDraw interface GUID with DirectDraw 7 |
| **Health Bar Percent** | `Base + 0x340C4` | `HookCall` | Renders percentage text over HP bar |
| **Mana Bar Percent** | `Base + 0x34276` | `HookCall` | Renders percentage text over MP bar |
| **On-Screen HP/Mana Bar**| `Base + 0xF5675`, `0xF573E`... | `HookCall` / `OverWriteWord` | Draws custom mana bar under player sprite |
| **Magic Effects Limit** | `Base + 0x104B4` | `HookCall` / `OverWriteByte` | Upgrades magic effect parsing from `uint8_t` to `uint16_t` |
| **Player HP/MP Stats** | `Base + 0x11D2B`, `0x11D69` | `HookCall` / `OverWrite` | Extends player HP and Mana parsing to `int32_t` |
| **Player Skills Limit** | `Base + 0x11FA4` | `HookCall` / `OverWriteByte` | Extends skill parsing from `uint8_t` to `uint16_t` |
| **Network Decrypt Hook**| `Base + 0x5C3A0` | `HookCall` | Intercepts decrypted incoming packets for Opcode 0x32 |
| **OpenGL Alpha Blend** | `Base + 0x13E948`, `0x13EA44`| `HookCallN` | Injects `glBlendFunc` and `glEnable(GL_BLEND)` |
| **DirectX 9 Context** | `Base + 0x1D2214`, `0x1D2210`| `OverWrite` | Hooks DX9 device initialization and destruction |

---

## Extended Network Protocol (Opcode 0x32)

The DLL intercepts decrypted incoming network packets via `Hooked_DecryptCall` (`Base + 0x5C3A0`). Packets prefixed with byte `0x32` (`50` decimal) are consumed as **Extended Opcodes**:

```
Packet Structure:
[uint8_t: 0x32] [uint8_t: SubOpcode] [Payload...]
```

### Supported Sub-Opcodes:

| Sub-Opcode | Hex | Name | Payload Structure | Description |
|---|---|---|---|---|
| **1** | `0x01` | `EXT_OPCODE_SET_MOUNT` | `[uint32_t creatureId]`<br>`[uint16_t mountId]`<br>`[uint8_t head, body, legs, feet]` | Sets creature mount and applies outfit color masks. Elevates creature by `3px` on the Y-axis. |
| **2** | `0x02` | `EXT_OPCODE_REMOVE_MOUNT` | `[uint32_t creatureId]` | Disables mount on creature and resets rendering height offset. |
| **3** | `0x03` | `EXT_OPCODE_MARKET_DATA` | `[Binary Market Payload]` | Opens native Market interface and populates offer buffers. |

---

## Configuration (`config.ini`)

All features can be toggled without recompiling the DLL. Place `config.ini` alongside `ddraw.dll` and `Tibia.exe`:

```ini
; ===================================================================
; Tibia 860 - Extended Client DLL Configuration
; Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)
; ===================================================================

[General]
; Enables high resolution hardware timer for Windows 10/11 (fixes FPS stuttering)
HighResolutionTimer = 1

; Enables extended sprite limit (> 65,535 sprites via uint32_t)
ExtendedSprites = 1

; Enables support for 32-bit Alpha transparency (ARGB)
AlphaTransparency = 1

; Enables in-memory sprite texture caching for higher performance (< 250MB)
CacheSprites = 1

[Features]
; Enables on-screen mana bar drawn below player character
DrawManaBar = 1

; Enables mounts system and layer elevation (+3px)
EnableMounts = 1

; Enables Market System graphical interface
EnableMarket = 1

; Enables extended server communication channel (Opcode 0x32)
ExtendedOpcode = 1

[Limits]
; Breaks magic effects limit (uint8_t 255 -> uint16_t 65535)
ExtendedMagicEffects = 1

; Breaks player health and mana display limit (uint16_t -> int32_t)
ExtendedPlayerStats = 1

; Breaks player skills display limit (uint8_t -> uint16_t)
ExtendedPlayerSkills = 1
```

---

## Building from Source

### Requirements
- **Operating System**: Windows 7 / 8 / 10 / 11
- **Toolchain**: Microsoft Visual C++ (MSVC) from Visual Studio 2019, 2022, or 2026
- **Architecture Target**: **`x86 / Win32 (32-bit)`** *(Tibia 8.60 is a 32-bit executable; 64-bit builds cannot be injected)*
- **Build System**: CMake 3.15 or newer
- **Windows SDK**: DirectX 9 SDK headers (`d3d9.h`), OpenGL headers (`gl.h`), Windows Multimedia (`winmm.lib`)

### Option 1: Automatic Build (`build.bat`)
Run the provided automated build script:
```cmd
build.bat
```
The script will locate your installed Visual Studio CMake binary, configure the 32-bit solution, compile in `Release` mode, and copy `ddraw.dll` directly to the project root.

### Option 2: Manual CMake Build
Open the **Developer Command Prompt for VS** (choose **x86 Native Tools Command Prompt**):

```cmd
mkdir build
cd build
cmake .. -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The output file `ddraw.dll` will be generated in `build/Release/ddraw.dll`.

---

## Installation & Quick Start

1. Compile the project or obtain a pre-compiled `ddraw.dll`.
2. Copy `ddraw.dll` and `config.ini` directly into your **Tibia 8.60 client directory** (where `Tibia.exe`, `Tibia.dat`, and `Tibia.spr` are located).
3. Start `Tibia.exe`.
4. The DLL will initialize automatically, hooking the client and applying your configuration settings.

---

## Security & Stability

- **Memory Protection**: Protects target memory areas before writing and restores memory protection attributes via `VirtualProtect`.
- **Instruction Cache Consistency**: Executes `FlushInstructionCache` after every patch to prevent CPU instruction cache incoherency.
- **Fail-Safe Exception Handling**: All critical hooks (network packet inspection, creature rendering, texture binding, and DAT parsing) are wrapped in structured exception blocks (`try / catch`) to safeguard the client from crashes when receiving malformed packets or corrupt asset files.
- **Thread Safety**: Global states (`CreatureManager`, `MarketSystem`, `Sprites`) are synchronized with `std::mutex` and `std::lock_guard` to ensure stability across rendering, network, and main UI threads.

---

## Credits & Acknowledgements

- **Creator & Lead Developer**: [Nottinghster](https://github.com/rodrigopaixaorj) (`Tibia 860 - Extended Client DLL`)
- **Special Thanks & Acknowledgements**:
  - **[Jo3bingham](https://github.com/Jo3bingham)** (Creator of **[TibiaAPI](https://github.com/Jo3bingham/TibiaAPI)**): Sincere gratitude for the groundbreaking reverse-engineering research, memory address definitions, and client constants documented in the TibiaAPI project. These insights served as the foundational baseline and inspiration for mapping memory offsets and architecting this extension DLL.
  - **[SaiyansKing](https://github.com/SaiyansKing)** (Creator of **[Tibia-Extended-Client-Library](https://github.com/SaiyansKing/Tibia-Extended-Client-Library)**): Immense credit and appreciation for the architectural concepts, graphic engine hooking techniques, and extended sprite handling that served as a core foundation and reference for this project.

---

## License

This project is licensed under the [MIT License](LICENSE). You are free to use, modify, and distribute this software with proper attribution.
