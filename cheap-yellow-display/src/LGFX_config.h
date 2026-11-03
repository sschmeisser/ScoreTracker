#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// =============================================================================
// ESP32-2432S028R "Cheap Yellow Display" (CYD) LGFX Configuration
// -----------------------------------------------------------------------------
// ILI9341 2.8" 240x320 panel driven over HSPI in landscape (320x240), plus an
// XPT2046 resistive touch controller on a SEPARATE VSPI bus.
//
// All pin numbers and raw touch calibration bounds are exposed as #define
// macros so main.cpp can implement the TOUCHCAL runtime calibration feature.
// =============================================================================

// --- Display (ILI9341) SPI pins ---
#ifndef PIN_LCD_SCLK
#define PIN_LCD_SCLK 14
#endif
#ifndef PIN_LCD_MOSI
#define PIN_LCD_MOSI 13
#endif
#ifndef PIN_LCD_MISO
#define PIN_LCD_MISO 12
#endif
#ifndef PIN_LCD_DC
#define PIN_LCD_DC   2
#endif
#ifndef PIN_LCD_CS
#define PIN_LCD_CS   15
#endif
// Backlight, PWM-capable
#ifndef PIN_LCD_BL
#define PIN_LCD_BL   21
#endif

// --- Touch (XPT2046) SPI pins (separate bus) ---
#ifndef PIN_TOUCH_SCLK
#define PIN_TOUCH_SCLK 25
#endif
#ifndef PIN_TOUCH_MOSI
#define PIN_TOUCH_MOSI 32
#endif
#ifndef PIN_TOUCH_MISO
#define PIN_TOUCH_MISO 39
#endif
#ifndef PIN_TOUCH_CS
#define PIN_TOUCH_CS   33
#endif
// INT tied off; the driver polls instead of using an interrupt.
#ifndef PIN_TOUCH_IRQ
#define PIN_TOUCH_IRQ  -1
#endif

// --- XPT2046 raw calibration bounds (runtime-tunable via TOUCHCAL) ---
#ifndef XPT_XMIN
#define XPT_XMIN 200
#endif
#ifndef XPT_XMAX
#define XPT_XMAX 3700
#endif
#ifndef XPT_YMIN
#define XPT_YMIN 240
#endif
#ifndef XPT_YMAX
#define XPT_YMAX 3800
#endif

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI      _bus;
  lgfx::Light_PWM    _light;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX() {
    // -------------------------------------------------------------------
    // Display SPI bus (HSPI)
    // -------------------------------------------------------------------
    {
      auto c = _bus.config();
      c.spi_host    = HSPI_HOST;
      c.spi_mode    = 0;
      c.freq_write  = 55000000;
      c.freq_read   = 16000000;
      c.pin_sclk    = PIN_LCD_SCLK;
      c.pin_mosi    = PIN_LCD_MOSI;
      c.pin_miso    = PIN_LCD_MISO;
      c.pin_dc      = PIN_LCD_DC;
      c.dma_channel = SPI_DMA_CH_AUTO;
      c.use_lock    = true;
      _bus.config(c);
      _panel.setBus(&_bus);
    }

    // -------------------------------------------------------------------
    // Panel (ILI9341) — landscape 320x240 native geometry
    // -------------------------------------------------------------------
    {
      auto c = _panel.config();
      c.pin_cs           = PIN_LCD_CS;
      c.pin_rst          = -1;
      c.pin_busy         = -1;
      c.panel_width      = 320;
      c.panel_height     = 240;
      c.memory_width     = 320;
      c.memory_height    = 240;
      c.offset_x         = 0;
      c.offset_y         = 0;
      c.invert           = false;
      // rgb_order is CYD-variant dependent and compile-time only.
      // Verified on this board: with rgb_order=false a pure RED bar renders BLUE
      // (R/B swapped) while a BLACK bar stays black, so the panel is BGR and the
      // swap must be enabled. Flip if a future board renders the opposite.
      c.rgb_order        = true;
      c.readable         = true;
      c.bus_shared       = true;
      c.dummy_read_pixel = 8;
      c.dummy_read_bits  = 1;
      _panel.config(c);
      _panel.setTouch(&_touch);
    }

    // -------------------------------------------------------------------
    // Backlight (PWM)
    // -------------------------------------------------------------------
    {
      auto c = _light.config();
      c.pin_bl      = PIN_LCD_BL;
      c.freq        = 44100;
      c.pwm_channel = 7;
      c.invert      = false;
      _light.config(c);
      _panel.setLight(&_light);
    }

    // -------------------------------------------------------------------
    // Touch controller (XPT2046) on its own VSPI bus
    // -------------------------------------------------------------------
    {
      auto c = _touch.config();
      c.spi_host       = VSPI_HOST;
      c.freq           = 1000000;
      c.pin_sclk       = PIN_TOUCH_SCLK;
      c.pin_mosi       = PIN_TOUCH_MOSI;
      c.pin_miso       = PIN_TOUCH_MISO;
      c.pin_cs         = PIN_TOUCH_CS;
      c.pin_int        = PIN_TOUCH_IRQ;
      c.bus_shared     = true;
      c.offset_rotation = 0;
      c.x_min          = XPT_XMIN;
      c.x_max          = XPT_XMAX;
      c.y_min          = XPT_YMIN;
      c.y_max          = XPT_YMAX;
      _touch.config(c);
    }

    setPanel(&_panel);
  }
};
