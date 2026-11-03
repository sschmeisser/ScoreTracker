#pragma once

// =============================================================================
// BIG PILOT WATCH CONFIGURATION
// =============================================================================

// Wi-Fi Credentials for Atomic NTP Time Synchronization
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

// NTP Servers
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define NTP_SERVER3 "time.google.com"

// Timezone: Pacific Time (PST/PDT)
// POSIX string handles PST (UTC-8) and PDT (UTC-7 daylight saving) automatically:
// Starts 2nd Sunday in March (M3.2.0), ends 1st Sunday in November (M11.1.0)
#define TIMEZONE_PST "PST8PDT,M3.2.0,M11.1.0"

// Hardware Pins
#ifndef PIN_BOOT_BTN
#define PIN_BOOT_BTN 9
#endif

// Power Management / Auto-Dim
#define AUTO_DIM_TIMEOUT_MS 180000  // 3 minutes before dimming to ambient
#define BRIGHTNESS_HIGH     240
#define BRIGHTNESS_MED      150
#define BRIGHTNESS_LOW       50
#define BRIGHTNESS_DIM       25
