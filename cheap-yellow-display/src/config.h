#pragma once

// =============================================================================
// CHEAP YELLOW DISPLAY (ESP32-2432S028R) SCORE TRACKER CONFIGURATION
// =============================================================================

// Wi-Fi Credentials (used for standalone ESPN live-score polling / NTP)
#ifndef WIFI_SSID
#define WIFI_SSID "twentyseven"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "4085299160"
#endif

// Wi-Fi enablement (0 = Laptop USB Serial push mode, 1 = Standalone Wi-Fi)
#ifndef ENABLE_WIFI
#define ENABLE_WIFI 1
#endif

// NTP Servers for Atomic Time Synchronization
#ifndef NTP_SERVER1
#define NTP_SERVER1 "pool.ntp.org"
#endif
#ifndef NTP_SERVER2
#define NTP_SERVER2 "time.nist.gov"
#endif
#ifndef NTP_SERVER3
#define NTP_SERVER3 "time.google.com"
#endif

// Timezone: Pacific Time (PST/PDT)
// POSIX string handles PST (UTC-8) and PDT (UTC-7 daylight saving) automatically:
// Starts 2nd Sunday in March (M3.2.0), ends 1st Sunday in November (M11.1.0)
#ifndef TIMEZONE_PST
#define TIMEZONE_PST "PST8PDT,M3.2.0,M11.1.0"
#endif

// Hardware Pins
#ifndef PIN_BOOT_BTN
#define PIN_BOOT_BTN 0  // CYD BOOT button (active LOW, internal pullup)
#endif

// Power Management / Auto-Dim
#ifndef AUTO_DIM_TIMEOUT_MS
#define AUTO_DIM_TIMEOUT_MS 180000  // 3 minutes before dimming to ambient
#endif
#ifndef BRIGHTNESS_HIGH
#define BRIGHTNESS_HIGH 240
#endif
#ifndef BRIGHTNESS_MED
#define BRIGHTNESS_MED 150
#endif
#ifndef BRIGHTNESS_LOW
#define BRIGHTNESS_LOW 50
#endif
#ifndef BRIGHTNESS_DIM
#define BRIGHTNESS_DIM 25
#endif

// Touch / UI Timing (CYD landscape 320x240)
#ifndef TOUCH_DEBOUNCE_MS
#define TOUCH_DEBOUNCE_MS 250        // Cooldown between accepted taps
#endif
#ifndef TOUCH_ZONE_FRACTION_PCT
#define TOUCH_ZONE_FRACTION_PCT 25   // Left/right thirds for PREV/NEXT as % of width
#endif
#ifndef HOLD_DURATION_MS
#define HOLD_DURATION_MS 6000        // Peaceful hold before auto-advance (6.0s)
#endif
#ifndef TRANSITION_DURATION_MS
#define TRANSITION_DURATION_MS 700   // Smooth page glide duration (700ms)
#endif
