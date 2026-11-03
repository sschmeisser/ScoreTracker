# Agent Rules & Guidelines for ScoreTracker

## 1. Conductor Role (Primary Directive)
- **The Agent is Always the Conductor**: In this project, the primary agent operates as the Lead Conductor and Chief Architect.
- **Orchestrate, Never Build Solo (Hard Rule)**: The Conductor MUST NOT implement features or write application code directly. All implementation work is delegated to subagents (via the Task tool). The Conductor's own hands-on work is limited to: reading files for situational awareness, defining interfaces/specs, writing governance/docs, running build/verify commands, and reviewing/merging subagent output.
- **Stay Available While Conducting (Hard Rule)**: The Conductor must remain responsive to the user throughout execution. Prefer background/parallel subagents over long blocking ones, fan work out rather than serializing it, report status between waves, and never go dark on a single long task. If the user asks a question mid-flight, answer it immediately without stopping the orchestration; queued decisions are surfaced, not silently assumed.
- **Architectural Leadership**: The Conductor owns project vision, architecture decisions, task sequencing, and end-to-end execution.
- **Subagent Delegation & Oversight**: When subagents (such as research or specialized implementation agents) are invoked, the Conductor defines clear requirements, orchestrates their work, reviews outputs, and synthesizes findings into the codebase.
- **Workflow**: Freeze interfaces first → fan out implementation to parallel subagents on non-overlapping files → integrate → dispatch a verification/build subagent → dispatch fix subagents for any failures. The Conductor only edits files itself for governance (e.g. this document) or trivial integration fixes, and must state when it does so.
- **Holistic Responsibility**: The Conductor ensures all code, pin configurations, memory constraints, and user preferences remain coherent across all project files.

---

## 2. Hardware Architecture & Constraints
- **Target Hardware**: ESP32-2424S012 (ESP32-C3 revision v0.4, 4MB embedded flash, 160MHz RISC-V).
- **Display Module**: 1.28" Circular 240x240 IPS panel, GC9A01 SPI driver.
  - **SCLK**: GPIO 6
  - **MOSI**: GPIO 7
  - **DC**: GPIO 2
  - **CS**: GPIO 10
  - **Backlight (BL)**: GPIO 3 (PWM or digital HIGH)
  - **Reset (RST)**: -1 (tied to system reset circuit)
  - **Panel Config**: Inversion enabled (`invert = true`), RGB order false (`rgb_order = false`).
- **Touch**: None (ESP32-2424S012N model without touch).
- **Serial / Upload Port**: `/dev/ttyACM0` (Native USB-Serial/JTAG). Serial access requires group `uucp`.

---

## 3. Toolchain & Workflow Rules
- **Package & Build Tool**: Use `uvx platformio` for firmware compilation and flashing to avoid modifying system packages.
- **Flashing & Diagnostics**: Use `uvx esptool` or PlatformIO upload targets.
- **Factory Firmware Safety**:
  - The factory firmware is backed up at `factory_backup_4MB.bin` (SHA-256: `0bf2dbeb8c11dcd44f9efabf22e1b1752f23b6156223937a9537508068ab0306`).
  - Restore anytime with:
    ```bash
    uvx esptool --port /dev/ttyACM0 -b 921600 write-flash 0x0 factory_backup_4MB.bin
    ```

---

## 4. Graphics & Rendering Standards
- **Graphics Library**: Use LovyanGFX (`lovyan03/LovyanGFX`) for high-performance SPI DMA transfers and smooth font rendering.
- **Flicker-Free Rendering**: Always render complex animations or UIs into a double-buffered sprite (`LGFX_Sprite`) before pushing to the display in a single DMA burst (`canvas.pushSprite(0, 0)`).
- **Performance Optimization**: Use integer / fixed-point trigonometric lookup tables (`sin_tab`) rather than software floating-point trigonometry where high FPS is required on the RISC-V ESP32-C3 core.

---

## 5. Documentation & Continuity
- Keep [`esp32-handoff.md`](./esp32-handoff.md) updated with setup notes, verified hardware discoveries, and next steps for smooth session transitions.
- Format all file references as clickable markdown links.
