#include <iostream>
#include <ctime>
#include <cstring>

void parse_iso_date(const char* iso, char* day_out, size_t day_sz, char* date_out, size_t date_sz) {
  if (!iso || strlen(iso) < 16) {
    strncpy(day_out, "SUN", day_sz);
    strncpy(date_out, "SEP 20", date_sz);
    return;
  }
  int y = 0, m = 0, d = 0, h = 0, min = 0;
  sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &m, &d, &h, &min);

  struct tm tm_utc;
  memset(&tm_utc, 0, sizeof(tm_utc));
  tm_utc.tm_year = y - 1900;
  tm_utc.tm_mon = m - 1;
  tm_utc.tm_mday = d;
  tm_utc.tm_hour = h;
  tm_utc.tm_min = min;

  time_t t = timegm(&tm_utc);
  // Convert to US Eastern (UTC - 4h in September)
  t -= 4 * 3600;
  struct tm tm_local;
  gmtime_r(&t, &tm_local);

  static const char* DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char* MONTHS[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  snprintf(day_out, day_sz, "%s", DAYS[tm_local.tm_wday]);
  snprintf(date_out, date_sz, "%s %02d", MONTHS[tm_local.tm_mon], tm_local.tm_mday);
}

int main() {
  char d[8], dt[16];
  parse_iso_date("2026-09-18T00:15Z", d, sizeof(d), dt, sizeof(dt));
  std::cout << "2026-09-18T00:15Z -> " << d << " " << dt << std::endl;

  parse_iso_date("2026-09-20T17:00Z", d, sizeof(d), dt, sizeof(dt));
  std::cout << "2026-09-20T17:00Z -> " << d << " " << dt << std::endl;

  parse_iso_date("2026-09-22T00:15Z", d, sizeof(d), dt, sizeof(dt));
  std::cout << "2026-09-22T00:15Z -> " << d << " " << dt << std::endl;
  return 0;
}
