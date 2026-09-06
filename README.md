# STM32F103 NTSC Video Engine

A lightweight **320×200 monochrome NTSC composite video engine** developed for the **STM32F103 Medium Density** series.

The project generates a real-time composite video signal using the STM32's hardware peripherals and a compact framebuffer, demonstrating how an STM32F103 can be used to produce video and embedded graphics without a dedicated video controller.

## Features

* **STM32F103 Medium Density** support
* **320×200 monochrome video**
* NTSC composite video generation
* Hardware timer-based NTSC synchronization
* SPI-based pixel transmission
* DMA support
* Interrupt-driven video timing
* Compact 1-bit framebuffer
* Bitmap rendering
* Direct framebuffer graphics
* Designed for low-memory STM32 devices
* Real-time embedded graphics

## Hardware

The project is designed around the **STM32F103 Medium Density** family.

The video output uses the STM32 hardware peripherals to generate the timing and pixel stream required for monochrome composite video.

Because the STM32F103 has significantly less memory and resources than larger STM32 devices, the project focuses on efficient framebuffer storage, timing, and rendering.

## Video Resolution

```text
Resolution: 320 × 200
Color:     1-bit monochrome

Bytes per scanline:
320 / 8 = 40 bytes

Framebuffer size:
40 × 200 = 8000 bytes
```

Each byte represents eight horizontal pixels.

This compact representation makes it possible to store a complete 320×200 monochrome frame while keeping memory usage as low as possible.

## Video Architecture

```text
                 STM32F103
                     │
          ┌──────────┴──────────┐
          │                     │
       Timer                  SPI
          │                     │
     NTSC Sync             Pixel Data
          │                     │
          └──────────┬──────────┘
                     │
                    DMA
                     │
                     ▼
              Composite Output
                     │
                     ▼
                 TV / Monitor
```

The timer is responsible for maintaining the NTSC timing while SPI is used to transmit the monochrome pixel data.

DMA can be used to move framebuffer data to the SPI peripheral with minimal CPU intervention.

## Memory Efficiency

One of the main goals of this project is making useful graphics possible on the relatively limited resources of the STM32F103.

A 320×200 monochrome framebuffer requires only:

```text
320 × 200 / 8 = 8,000 bytes
```

This leaves the remaining MCU resources available for application logic, graphics calculations, and other peripherals.

## Graphics

The framebuffer can be accessed directly by the graphics code, allowing applications to draw:

* Pixels
* Lines
* Rectangles
* Bitmaps
* Text
* Simple animations
* Retro-style graphics

The project can also be used as a foundation for lightweight embedded graphics engines.

## Why STM32F103?

The STM32F103 is a popular and inexpensive ARM Cortex-M3 microcontroller.

This project explores what can be achieved with the F103 despite its limited memory and processing resources.

The goal is not to use the most powerful MCU available, but to demonstrate that **real-time composite video and graphics are possible even on a small STM32** with careful use of hardware peripherals and memory.

## Learning Topics

This project is useful for experimenting with:

* STM32 timers
* SPI
* DMA
* Interrupts
* GPIO
* NTSC timing
* Composite video
* Framebuffers
* Bit-packed graphics
* Embedded graphics
* Memory optimization
* Real-time rendering
* ARM Cortex-M3 programming

## Status

🚧 **Work in Progress**

The project is experimental and may continue to evolve as video timing, rendering performance, and graphics features are improved.

## License

This project is **free and open source** and is released under the **MIT License**.

You are free to use, modify, copy, distribute, and build upon the software, subject to the terms of the MIT License.

See the [`LICENSE`](LICENSE) file for the complete license text.

## Contributions

Suggestions, optimizations, bug fixes, hardware adaptations, and graphics improvements are welcome.

Feel free to open an issue or submit a pull request if you would like to contribute.

## Disclaimer

This project is provided **as-is**, without warranty of any kind.

It is intended primarily for educational, experimental, and embedded-development purposes.

---

**STM32F103 • Cortex-M3 • NTSC • SPI • DMA • 320×200 • Monochrome • Embedded Graphics**
