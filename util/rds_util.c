// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "rds_util.h"

#include <stdio.h>
#include <string.h>

#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

const char* RTPlusCodeNames[65] = {
    "Dummy",
    "Title",
    "Album",
    "Track",
    "Artist",
    "Composition",
    "Movement",
    "Conductor",
    "Composer",
    "Band",
    "Comment",
    "Genre",

    "News",
    "Local News",
    "Stock Mkt",
    "Sports",
    "Lottery",
    "Horoscope",
    "Daily diversion",
    "Health",
    "Event",
    "Szene",
    "Cinema",
    "TV",
    "Date_Time",
    "Weather",
    "Traffic",
    "Alarm",
    "Advertisement",
    "URL",
    "Other",

    "Station.short",
    "Station.long",
    "Programme.now",
    "Programme.next",
    "Programme.part",
    "Programme.host",
    "Programme.editorial",
    "Programme.frequency",
    "Programme.homepage",
    "Programme.subchannel",

    "Phone.hotline",
    "Phone.studio",
    "Phone.other",
    "SMS.studio",
    "SMS.other",
    "Email.hotline",
    "Email.studio",
    "Email.other",
    "MMS.other",
    "Chat",
    "Chat.center",
    "Vote.question",
    "Vote.center",

    "rfu",
    "rfu",

    "Private classes",
    "Private classes",

    "Place",
    "Appointment",
    "Identifier",
    "Purchase",
    "Get.data",
};
/**
 * Decode the PI (Program Identification) code with RDSA for the US region.
 *
 * Only call if in US region and errors are less than 6+.
 *
 * This function ported from http://www.w9wi.com/articles/rds.htm with
 * permission from Doug Smith.
 */
static bool decode_pi_US(char* buffer, size_t buffer_len, uint16_t picode) {
  if (buffer_len < 5)
    return false;

  if ((picode & 0xAF00) == 0xAF00)
    picode = (picode & 0x00FF) << 8;
  if ((picode & 0xA000) == 0xA000)
    picode = ((picode & 0x0F00) << 4) | (picode & 0xFF);

  if (picode > 4095 && picode < 39247) {
    if (picode > 21671) {
      buffer[0] = 'W';
      picode -= 21672;
    } else {
      buffer[0] = 'K';
      picode -= 4096;
    }
    const uint16_t call2 = picode / 676;
    picode -= 676 * call2;
    const uint16_t call3 = picode / 26;
    const uint16_t call4 = picode - (26 * call3);

    buffer[1] = 'A' + call2;
    buffer[2] = 'A' + call3;
    buffer[3] = 'A' + call4;
    buffer[4] = '\0';
    return true;
  }

  switch (picode) {
    case 49829:
      strncpy(buffer, "CIMF", buffer_len);
      break;
    case 17185:
      strncpy(buffer, "CJPT", buffer_len);
      break;
    case 39248:
      strncpy(buffer, "KEX", buffer_len);
      break;
    case 39249:
      strncpy(buffer, "KFH", buffer_len);
      break;
    case 39253:
      strncpy(buffer, "KGU", buffer_len);
      break;
    case 39254:
      strncpy(buffer, "KGW", buffer_len);
      break;
    case 39255:
      strncpy(buffer, "KGY", buffer_len);
      break;
    case 39256:
      strncpy(buffer, "KID", buffer_len);
      break;
    case 39257:
      strncpy(buffer, "KIT", buffer_len);
      break;
    case 39258:
      strncpy(buffer, "KJR", buffer_len);
      break;
    case 39259:
      strncpy(buffer, "KLO", buffer_len);
      break;
    case 39260:
      strncpy(buffer, "KLZ", buffer_len);
      break;
    case 39261:
      strncpy(buffer, "KMA", buffer_len);
      break;
    case 39262:
      strncpy(buffer, "KMJ", buffer_len);
      break;
    case 39263:
      strncpy(buffer, "KNX", buffer_len);
      break;
    case 39264:
      strncpy(buffer, "KOA", buffer_len);
      break;
    case 39268:
      strncpy(buffer, "KQV", buffer_len);
      break;
    case 39269:
      strncpy(buffer, "KSL", buffer_len);
      break;
    case 39270:
      strncpy(buffer, "KUJ", buffer_len);
      break;
    case 39271:
      strncpy(buffer, "KVI", buffer_len);
      break;
    case 39272:
      strncpy(buffer, "KWG", buffer_len);
      break;
    case 39275:
      strncpy(buffer, "KYW", buffer_len);
      break;
    case 39277:
      strncpy(buffer, "WBZ", buffer_len);
      break;
    case 39278:
      strncpy(buffer, "WDZ", buffer_len);
      break;
    case 39279:
      strncpy(buffer, "WEW", buffer_len);
      break;
    case 39281:
      strncpy(buffer, "WGL", buffer_len);
      break;
    case 39282:
      strncpy(buffer, "WGN", buffer_len);
      break;
    case 39283:
      strncpy(buffer, "WGR", buffer_len);
      break;
    case 39285:
      strncpy(buffer, "WHA", buffer_len);
      break;
    case 39286:
      strncpy(buffer, "WHB", buffer_len);
      break;
    case 39287:
      strncpy(buffer, "WHK", buffer_len);
      break;
    case 39288:
      strncpy(buffer, "WHO", buffer_len);
      break;
    case 39290:
      strncpy(buffer, "WIP", buffer_len);
      break;
    case 39291:
      strncpy(buffer, "WJR", buffer_len);
      break;
    case 39292:
      strncpy(buffer, "WKY", buffer_len);
      break;
    case 39293:
      strncpy(buffer, "WLS", buffer_len);
      break;
    case 39294:
      strncpy(buffer, "WLW", buffer_len);
      break;
    case 39297:
      strncpy(buffer, "WOC", buffer_len);
      break;
    case 39299:
      strncpy(buffer, "WOL", buffer_len);
      break;
    case 39300:
      strncpy(buffer, "WOR", buffer_len);
      break;
    case 39304:
      strncpy(buffer, "WWJ", buffer_len);
      break;
    case 39305:
      strncpy(buffer, "WWL", buffer_len);
      break;
    case 39312:
      strncpy(buffer, "KDB", buffer_len);
      break;
    case 39313:
      strncpy(buffer, "KGB", buffer_len);
      break;
    case 39314:
      strncpy(buffer, "KOY", buffer_len);
      break;
    case 39315:
      strncpy(buffer, "KPQ", buffer_len);
      break;
    case 39316:
      strncpy(buffer, "KSD", buffer_len);
      break;
    case 39317:
      strncpy(buffer, "KUT", buffer_len);
      break;
    case 39318:
      strncpy(buffer, "KXL", buffer_len);
      break;
    case 39319:
      strncpy(buffer, "KXO", buffer_len);
      break;
    case 39321:
      strncpy(buffer, "WBT", buffer_len);
      break;
    case 39322:
      strncpy(buffer, "WGH", buffer_len);
      break;
    case 39323:
      strncpy(buffer, "WGY", buffer_len);
      break;
    case 39324:
      strncpy(buffer, "WHP", buffer_len);
      break;
    case 39325:
      strncpy(buffer, "WIL", buffer_len);
      break;
    case 39326:
      strncpy(buffer, "WMC", buffer_len);
      break;
    case 39327:
      strncpy(buffer, "WMT", buffer_len);
      break;
    case 39328:
      strncpy(buffer, "WOI", buffer_len);
      break;
    case 39329:
      strncpy(buffer, "WOW", buffer_len);
      break;
    case 39330:
      strncpy(buffer, "WRR", buffer_len);
      break;
    case 39331:
      strncpy(buffer, "WSB", buffer_len);
      break;
    case 39332:
      strncpy(buffer, "WSM", buffer_len);
      break;
    case 39333:  // Also XHSR?
      strncpy(buffer, "KBW", buffer_len);
      break;
    case 39334:
      strncpy(buffer, "KCY", buffer_len);
      break;
    case 39335:
      strncpy(buffer, "KDF", buffer_len);
      break;
    case 39338:
      strncpy(buffer, "KHQ", buffer_len);
      break;
    case 39339:
      strncpy(buffer, "KOB", buffer_len);
      break;
    case 39347:
      strncpy(buffer, "WIS", buffer_len);
      break;
    case 39348:
      strncpy(buffer, "WJW", buffer_len);
      break;
    case 39349:
      strncpy(buffer, "WJZ", buffer_len);
      break;
    case 39353:
      strncpy(buffer, "WRC", buffer_len);
      break;
    case 26542:
      strncpy(buffer, "WHFI/CHFI", buffer_len);
      break;
    case 39250:
      strncpy(buffer, "KFI/CJBC", buffer_len);
      break;
    case 49160:
      strncpy(buffer, "CJBC-1", buffer_len);
      break;
    case 49158:
      strncpy(buffer, "CBCK", buffer_len);
      break;
    case 52010:
      strncpy(buffer, "CBLG", buffer_len);
      break;
    case 52007:
      strncpy(buffer, "CBLJ", buffer_len);
      break;
    case 52012:
      strncpy(buffer, "CBQT", buffer_len);
      break;
    case 52009:
      strncpy(buffer, "CBEB", buffer_len);
      break;
    case 28378:
      strncpy(buffer, "WJXY/CJXY", buffer_len);
      break;
    case 39251:
      strncpy(buffer, "KGA/CBCx", buffer_len);
      break;
    case 39252:
      strncpy(buffer, "KGO/CBCP", buffer_len);
      break;
    case 941:
      strncpy(buffer, "CKGE", buffer_len);
      break;
    case 16416:
      strncpy(buffer, "KSFW/CBLA", buffer_len);
      break;
    case 25414:
      strncpy(buffer, "WFNY/CFNY", buffer_len);
      break;
    case 27382:
      strncpy(buffer, "WILQ/CILQ", buffer_len);
      break;
    case 27424:
      strncpy(buffer, "WING/CING", buffer_len);
      break;
    case 26428:
      strncpy(buffer, "WHAY/CHAY", buffer_len);
      break;
    case 52033:
      strncpy(buffer, "CBA-FM", buffer_len);
      break;
    case 52034:
      strncpy(buffer, "CBCT", buffer_len);
      break;
    case 52045:
      strncpy(buffer, "CBHM", buffer_len);
      break;
    case 45084:
      strncpy(buffer, "CIQM", buffer_len);
      break;
    case 51806:
      strncpy(buffer, "CHNI, CJNI, or CKNI", buffer_len);
      break;
    case 12289:
      strncpy(buffer, "KLAS (Jamaica)", buffer_len);
      break;
    case 7877:
      strncpy(buffer, "CFPL", buffer_len);
      break;
    case 7760:
      strncpy(buffer, "ZFKY (Cayman Is.)", buffer_len);
      break;
    case 8151:
      strncpy(buffer, "ZFCC (Cayman Is.)", buffer_len);
      break;
    case 12656:
      strncpy(buffer, "WAVW", buffer_len);
      break;
    case 7908:
      strncpy(buffer, "KTCZ", buffer_len);
      break;
    case 42149:
      strncpy(buffer, "KSKZ or KWKR", buffer_len);
      break;
    case 45313:
      strncpy(buffer, "XHCTO", buffer_len);
      break;
    case 34784:
      strncpy(buffer, "XHTRR", buffer_len);
      break;
    default:
      return false;
  }
  buffer[buffer_len - 1] = '\0';
  return true;
}

/**
 * Decode the RDS PI data for non US ("rest of world").
 */
static bool decode_pi_ROW(char* buffer, size_t buffer_len, uint16_t pi_code) {
  UNUSED(buffer);
  UNUSED(buffer_len);
  UNUSED(pi_code);
  // TODO: Implement PI Code decoding for ROW.
  return false;
}

bool decode_pi_code(char* buffer,
                    size_t buffer_len,
                    uint16_t pi_code,
                    enum si470x_region_t region) {
  if (region == REGION_US)
    return decode_pi_US(buffer, buffer_len, pi_code);
  else
    return decode_pi_ROW(buffer, buffer_len, pi_code);
}

const char* get_rdsplus_code_name(uint16_t code_id) {
  if (code_id > ARRAY_SIZE(RTPlusCodeNames))
    return "Unknown";
  else
    return RTPlusCodeNames[code_id];
}

static const char* get_pty_code_name_US(uint8_t pty_code) {
  switch (pty_code) {
    case 0:
      return "";
    case 1:
      return "News";
    case 2:
      return "Information";
    case 3:
      return "Sports";
    case 4:
      return "Talk";
    case 5:
      return "Rock";
    case 6:
      return "Classic Rock";
    case 7:
      return "Adult Hits";
    case 8:
      return "Soft Rock";
    case 9:
      return "Top 40";
    case 10:
      return "Country";
    case 11:
      return "Oldies";
    case 12:
      return "Soft";
    case 13:
      return "Nostalgia";
    case 14:
      return "Jazz";
    case 15:
      return "Classical";
    case 16:
      return "Rhythm and Blues";
    case 17:
      return "Soft Rhythm and Blues";
    case 18:
      return "Foreign Language";
    case 19:
      return "Religious Music";
    case 20:
      return "Religious Talk";
    case 21:
      return "Personality";
    case 22:
      return "Public";
    case 23:
      return "College";
    case 29:
      return "Weather";
    case 30:
      return "Emergency Test";
    case 31:
      return "Emergency";
  }
  return "[Reserved]";
}

const char* get_pty_code_name(uint8_t pty_code, enum si470x_region_t region) {
  if (region == REGION_US)
    return get_pty_code_name_US(pty_code);
  else
    return "? PTY NAME";
}

const char* get_device_name(enum si470x_device_t device) {
  switch (device) {
    case DEVICE_4700:
      return "Si4700";
    case DEVICE_4701:
      return "Si4701";
    case DEVICE_4702:
      return "Si4702";
    case DEVICE_4703:
      return "Si4703";
    case DEVICE_UNKNOWN:
    default:
      return "Unknown";
  }
}

void get_manufacturer_name(uint16_t id, char* name, size_t name_len) {
  if (id == 0x242)
    strncpy(name, "Silicon Labs", name_len);
  else
    snprintf(name, name_len, "Unknown: 0x%x", id);
  name[name_len - 1] = '\0';
}

/**
 * Combine the 17-bit Modified Julian date into a single MJD value.
 */
static uint32_t get_mjd(const struct rds_data* rds) {
  return ((uint32_t)rds->clock.day_high) << 16 | rds->clock.day_low;
}

/**
 * Convert Modified Julian Date (MJD) to Date.
 *
 * Adapted from description in Annex G (p. 104) of RDMS specification.
 */
static void MJD2Date(int mjd, int* year, int* month, int* day) {
  *year = (int)((mjd - 15078.2) / 365.25);
  *month = (int)((mjd - 14956.1 - (int)(*year * 365.25)) / 30.6001);
  *day = mjd - 14956 - (int)(*year * 365.25) - (int)(*month * 30.6001);
  const int k = ((*month == 14) || (*month == 15)) ? 1 : 0;
  *year += k;
  *year += 1900;  // Spec says from 1900.
  *month -= 1 + k * 12;
};

void format_local_time(char* buff,
                       uint8_t bufflen,
                       const struct rds_data* rds) {
  if (!bufflen)
    return;

  uint32_t mjd = get_mjd(rds);

  // Use the UTF offset (# of 30 min. chunks) to convert UTC to local time.
  int hour = rds->clock.hour;
  int minute = rds->clock.minute;
  // TODO: Properly handle odd offsets (e.g. 30 min., 90 min.).
  int offset_hours = rds->clock.utc_offset / 2;
  hour += offset_hours;
  if (hour > 23) {
    hour -= 24;
    mjd++;
  } else if (hour < 0) {
    hour += 24;
    mjd--;
  }
  int year, month, day;
  MJD2Date(mjd, &year, &month, &day);

  snprintf(buff, bufflen, "%d/%d/%04d %02d:%02d", month, day, year, hour,
           minute);
  buff[bufflen - 1] = '\0';
}
