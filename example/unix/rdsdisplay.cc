#include <curses.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <oda_decode.h>
#include <rds_spy_log_reader.h>
#include <rds_util.h>
#include <si470x.h>
#include <si470x_port.h>

namespace {

enum class DrawMode { Basic, Stats, AltFreq, EON };

struct WindowEnder {
  ~WindowEnder() { endwin(); }
};

struct RDSTestData {
  std::string fname;                      // File name.
  std::vector<struct rds_blocks> blocks;  // RDS block data in file.
};

// Sleep for N msecs in main loop.
constexpr auto kSleepDuration = std::chrono::milliseconds(5);

// Update display every N secs.
constexpr auto kUpdateInterval = std::chrono::seconds(1);

// Seek tuner up to next station every N secs.
constexpr auto kTuneInterval = std::chrono::seconds(5);

struct si470x_t* g_tuner;
struct rds_oda_data* g_oda_data;
std::atomic<bool> g_dirty;
int g_update_num;
DrawMode g_draw_mode = DrawMode::Basic;
std::vector<RDSTestData> g_rds_test_data;
size_t g_current_block_idx = 0;
WINDOW* g_window;

struct TunerDeleter {
  ~TunerDeleter() {
    if (g_tuner)
      si470x_delete(g_tuner);
    if (g_oda_data)
      delete_oda_data(g_oda_data);
  }
};

bool IsPrintableChar(char ch) {
  return ch >= 32 && ch < 127;
}

void MakeSpaces(char* str, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (!IsPrintableChar(str[i]))
      str[i] = ' ';
  }
}

bool AllSpaces(const char* str) {
  const size_t len = strlen(str);
  for (size_t i = 0; i < len; i++) {
    if (str[i] != ' ')
      return false;
  }
  return true;
}

bool IsSpace(char ch) {
  // Note: 0xa (newline) is a legal character for PTYN.
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

void TrimTrailingWhitespace(char* str) {
  char* ch = str + strlen(str) - 1;
  while (ch >= str) {
    if (IsSpace(*ch))
      *ch-- = '\0';
    else
      break;
  }
}

bool ContainsTime(const struct rds_data* rds) {
  return rds->clock.day_high || rds->clock.day_low || rds->clock.hour ||
         rds->clock.minute;
}

void OnRDSChanged(void*) {
  g_dirty = true;
}

void ClearODA(void* user_data) {
  struct rds_oda_data* oda_data = (struct rds_oda_data*)user_data;
  clear_oda_data(oda_data);
}

void DecodeODA(uint16_t app_id,
               const struct rds_data* rds,
               const struct rds_blocks* blocks,
               struct rds_group_type gt,
               void* user_data) {
  struct rds_oda_data* oda_data = (struct rds_oda_data*)user_data;
  decode_oda_blocks(oda_data, app_id, rds, blocks, gt);
}

int DrawHeader(const si470x_state_t& state, const rds_data& rds_data) {
  if (g_rds_test_data.empty()) {
    char picode[40];
    if (!decode_pi_code(picode, ARRAY_SIZE(picode), rds_data.pi_code,
                        REGION_US)) {
      picode[0] = '\0';
    }
    mvprintw(0, 0, "Frequency: %.1f MHz (%s), RSSI: %d dB",
             state.frequency / 1e6, picode, state.rssi);
  } else {
    const auto& test_data = g_rds_test_data[g_current_block_idx];
    mvprintw(0, 0, "File %u/%d: \"%s\"", g_current_block_idx + 1,
             g_rds_test_data.size(), test_data.fname.c_str());
  }
  move(1, 0);
  hline('=', 200);
  return 2;
}

void DrawCurrentState() {
  erase();

  si470x_state_t state;
  if (!si470x_get_state(g_tuner, &state))
    return;
  rds_data rds_data;
  if (!si470x_get_rds_data(g_tuner, &rds_data))
    return;

  char ps[ARRAY_SIZE(rds_data.ps.display) + 1];
  memcpy(ps, (char*)rds_data.ps.display, sizeof(ps));
  MakeSpaces(ps, ARRAY_SIZE(ps) - 1);
  ps[ARRAY_SIZE(ps) - 1] = '\0';

  char rta[ARRAY_SIZE(rds_data.rt.a.display) + 1];
  memcpy(rta, (char*)rds_data.rt.a.display, sizeof(rta));
  MakeSpaces(rta, ARRAY_SIZE(rta) - 1);
  rta[ARRAY_SIZE(rta) - 1] = '\0';
  TrimTrailingWhitespace(rta);

  char rtb[ARRAY_SIZE(rds_data.rt.b.display) + 1];
  memcpy(rtb, (char*)rds_data.rt.b.display, sizeof(rtb));
  MakeSpaces(rtb, ARRAY_SIZE(rtb) - 1);
  rtb[ARRAY_SIZE(rtb) - 1] = '\0';
  TrimTrailingWhitespace(rtb);

  char ptyn[ARRAY_SIZE(rds_data.ptyn.display) + 1];
  memcpy(ptyn, (char*)rds_data.ptyn.display, sizeof(ptyn));
  MakeSpaces(ptyn, ARRAY_SIZE(ptyn) - 1);
  ptyn[ARRAY_SIZE(ptyn) - 1] = '\0';

  char ct[40];
  if (ContainsTime(&rds_data))
    format_local_time(ct, ARRAY_SIZE(ct), &rds_data);
  else
    ct[0] = '\0';

  char picode[40];
  if (!decode_pi_code(picode, ARRAY_SIZE(picode), rds_data.pi_code,
                      REGION_US)) {
    picode[0] = '\0';
  }
  char manufacturer[20];
  get_manufacturer_name(state.manufacturer, manufacturer,
                        ARRAY_SIZE(manufacturer));

  int y = DrawHeader(state, rds_data);
  mvprintw(y++, 0, "%s, Enabled:%c mfr:%s firmware:%d revision:%c",
           get_device_name(state.device), state.enabled ? 'Y' : 'N',
           manufacturer, state.firmware, state.revision);

  mvprintw(y++, 0, "Volume: %d", state.volume);
  mvprintw(y++, 0, "Stereo: %c", state.stereo ? 'Y' : 'N');
  if (rds_data.valid_values & RDS_TP_CODE ||
      rds_data.valid_values & RDS_TA_CODE) {
    mvprintw(y++, 0, "Traffic (TP): %c, TA: %c", rds_data.tp_code ? 'Y' : 'N',
             rds_data.ta_code ? 'Y' : 'N');
  }
  if (rds_data.valid_values & RDS_MS)
    mvprintw(y++, 0, "M/S:  %s", rds_data.music ? "music" : "speech");
  if (rds_data.valid_values & RDS_PTY)
    mvprintw(y++, 0, "PTY:  %s", get_pty_code_name(rds_data.pty, REGION_US));
  if (rds_data.valid_values & RDS_PTYN)
    mvprintw(y++, 0, "PTYN: [%s]", ptyn);
  if (rds_data.valid_values & RDS_FBT) {
    // mvprintw(y++, 0, "FBT: [%s]", fbt);
  }
  if (rds_data.valid_values & RDS_SLC) {
    if (rds_data.slc.variant_code == SLC_VARIANT_PAGING) {
      mvprintw(y++, 0, "SLC:  la:%c vc:%d paging:%u cc:%u",
               rds_data.slc.la ? 'Y' : 'N', rds_data.slc.variant_code,
               rds_data.slc.data.paging.paging,
               rds_data.slc.data.paging.country_code);
    } else {
      // shortcut because all other values are just unsigned ints.
      mvprintw(y++, 0, "SLC:  la:%c vc:%d data:0x%08x",
               rds_data.slc.la ? 'Y' : 'N', rds_data.slc.variant_code,
               rds_data.slc.data.tmc_id);
    }
  }
  if (rds_data.valid_values & RDS_PIC) {
    mvprintw(y++, 0, "PIN:  Day: %02d Time: %02d:%02d", rds_data.pic.day,
             rds_data.pic.hour, rds_data.pic.minute);
  }
  if (rds_data.valid_values & RDS_PS)
    mvprintw(y++, 0, "PS:   [%s]", ps);
  if (rds_data.valid_values & RDS_RT) {
    mvprintw(y++, 0, "RTA%c: \"%s\"", rds_data.rt.decode_rt == RT_A ? '*' : ' ',
             rta);
    mvprintw(y++, 0, "RTB%c: \"%s\"", rds_data.rt.decode_rt == RT_B ? '*' : ' ',
             rtb);
    for (int code_id = 1; code_id <= 63; code_id++) {
      char text[ARRAY_SIZE(g_oda_data->rtplus.text[code_id]) + 1];
      memcpy(text, (char*)g_oda_data->rtplus.text[code_id], sizeof(text));
      MakeSpaces(text, ARRAY_SIZE(text) - 1);
      text[ARRAY_SIZE(text) - 1] = '\0';
      TrimTrailingWhitespace(text);
      if (!AllSpaces(text)) {
        const char* name = get_rdsplus_code_name(code_id);
        move(y, 0);
        clrtoeol();
        move(y, 0);
        clrtoeol();
        mvprintw(y++, 0, "RT+ %s: \"%s\"", name, text);
      }
    }
  }
  for (int idx = 0; idx < NUM_TDC; idx++) {
    char text[TDC_LEN + 1];
    memcpy(text, (char*)rds_data.tdc.data[idx], TDC_LEN);
    MakeSpaces(text, ARRAY_SIZE(text) - 1);
    text[ARRAY_SIZE(text) - 1] = '\0';
    TrimTrailingWhitespace(text);
    if (!AllSpaces(text)) {
      mvprintw(y++, 0, "TDC[%d]: \"%s\"", idx, text);
    }
  }
  if (rds_data.valid_values & RDS_CLOCK)
    mvprintw(y++, 0, "CT:   %s", ct);
  if (rds_data.valid_values & RDS_AF)
    mvprintw(y++, 0, "AF:   cnt=%u", rds_data.af.count);

  // Divider - below here is derived metrics and debug stuff.
  move(y++, 0);
  hline('-', 200);

  for (uint8_t idx = 0; idx < rds_data.oda_cnt; idx++) {
    char oda_name[20];
    get_app_name(oda_name, ARRAY_SIZE(oda_name), rds_data.oda[idx].id);
    mvprintw(y++, 0, "ODA[%u] %u%c:%s, cnt:%u", idx, rds_data.oda[idx].gt.code,
             rds_data.oda[idx].gt.version, oda_name,
             rds_data.oda[idx].pkt_count);
  }

  mvprintw(y++, 0, "Update: %d", g_update_num);

  refresh();
}

void DrawCurrentStats() {
  erase();

  si470x_state_t state;
  if (!si470x_get_state(g_tuner, &state))
    return;
  rds_data rds_data;
  if (!si470x_get_rds_data(g_tuner, &rds_data))
    return;

  int top = DrawHeader(state, rds_data);
  int y = top;
  mvprintw(y++, 0, "Group     A       B");
  mvprintw(y++, 0, "-----  ------- -------");
#if defined(RDS_DEV)
  for (int i = 0; i < 16; i++) {
    char A[20];
    char B[20];

    if (rds_data.stats.groups[i].a) {
      snprintf(A, ARRAY_SIZE(A), "%d", rds_data.stats.groups[i].a);
      A[ARRAY_SIZE(A) - 1] = '\0';
    } else {
      A[0] = '\0';
    }

    if (rds_data.stats.groups[i].b) {
      snprintf(B, ARRAY_SIZE(B), "%d", rds_data.stats.groups[i].b);
      B[ARRAY_SIZE(B) - 1] = '\0';
    } else {
      B[0] = '\0';
    }

    int a_padding = 5 - strlen(A);
    int b_padding = 5 - strlen(B);

    mvprintw(y++, 0, "  %02d    %*s%s   %*s%s", i, a_padding, " ", A, b_padding,
             " ", B);
  }

  y = top;
  const int x = 30;
  mvprintw(y++, x, "     Group Data");
  mvprintw(y++, x, "----------------------");
  mvprintw(y++, x, "RDS count:      %d", rds_data.stats.data_cnt);
  mvprintw(y++, x, "Block B errors: %d", rds_data.stats.blckb_errors);

  mvprintw(y++, x, "AF:      %d", rds_data.stats.counts[PKTCNT_AF]);
  mvprintw(y++, x, "CLOCK:   %d", rds_data.stats.counts[PKTCNT_CLOCK]);
  mvprintw(y++, x, "EON:     %d", rds_data.stats.counts[PKTCNT_EON]);
  mvprintw(y++, x, "EWS:     %d", rds_data.stats.counts[PKTCNT_EWS]);
  mvprintw(y++, x, "FBT:     %d", rds_data.stats.counts[PKTCNT_FBT]);
  mvprintw(y++, x, "IH:      %d", rds_data.stats.counts[PKTCNT_IH]);
  mvprintw(y++, x, "MS:      %d", rds_data.stats.counts[PKTCNT_MS]);
  mvprintw(y++, x, "PAGING:  %d", rds_data.stats.counts[PKTCNT_PAGING]);
  mvprintw(y++, x, "PI_CODE: %d", rds_data.stats.counts[PKTCNT_PI_CODE]);
  mvprintw(y++, x, "PS:      %d", rds_data.stats.counts[PKTCNT_PS]);
  mvprintw(y++, x, "PTY:     %d", rds_data.stats.counts[PKTCNT_PTY]);
  mvprintw(y++, x, "PTYN:    %d", rds_data.stats.counts[PKTCNT_PTYN]);
  mvprintw(y++, x, "RT:      %d", rds_data.stats.counts[PKTCNT_RT]);
  mvprintw(y++, x, "SLC:     %d", rds_data.stats.counts[PKTCNT_SLC]);
  mvprintw(y++, x, "TA_CODE: %d", rds_data.stats.counts[PKTCNT_TA_CODE]);
  mvprintw(y++, x, "TDC:     %d", rds_data.stats.counts[PKTCNT_TDC]);
  mvprintw(y++, x, "TMC:     %d", rds_data.stats.counts[PKTCNT_TMC]);
  mvprintw(y++, x, "TP_CODE: %d", rds_data.stats.counts[PKTCNT_TP_CODE]);

  mvprintw(y++, x, "RT+:     %d", g_oda_data->stats.rtplus_cnt);
  mvprintw(y++, x, "RDS-TMC: %d", g_oda_data->stats.tmc_cnt);
  mvprintw(y++, x, "iTunes:  %d", g_oda_data->stats.itunes_cnt);
#endif
}

int DrawAFTable(int y, int x, int table_num, const struct rds_af_table* table) {
  if (table->tuned_freq.freq) {
    if (table->tuned_freq.band == AF_BAND_UHF) {
      mvprintw(y++, x, "%d) Tuned freq: %.1f MHz", table_num,
               table->tuned_freq.freq / 10.0f);
    } else {
      mvprintw(y++, x, "%d) Tuned freq: %u KHz", table_num,
               table->tuned_freq.freq);
    }
  }

  for (int i = 0; i < table->count; i++) {
    char method[30];
    if (table->entry[i].attrib == AF_ATTRIB_REG_VARIANT)
      strcpy(method, "Same program");
    else
      strcpy(method, "Rgn. variant");
    if (table->entry[i].band == AF_BAND_UHF) {
      mvprintw(y++, x, "%02d  %.1f MHz  %s", i, table->entry[i].freq / 10.0f,
               method);
    } else {
      mvprintw(y++, x, "%02d  %u KHz  %s", i, table->entry[i].freq, method);
    }
  }
  return y;
}

void DrawAlternativeFrequencies() {
  erase();

  si470x_state_t state;
  if (!si470x_get_state(g_tuner, &state))
    return;
  rds_data rds_data;
  if (!si470x_get_rds_data(g_tuner, &rds_data))
    return;

  int y = DrawHeader(state, rds_data);

  if (!rds_data.af.count) {
    mvprintw(y, 0, "No alternative frequencies.");
    return;
  }

  // All tables use the same encoding method, so the first one will do.
  char encoding_method;
  switch (rds_data.af.table[0].enc_method) {
    case AF_EM_UNKNOWN:
      encoding_method = '?';
      break;
    case AF_EM_A:
      encoding_method = 'A';
      break;
    case AF_EM_B:
      encoding_method = 'B';
      break;
  }

  mvprintw(y++, 0, "Encoding method: %c", encoding_method);

  const int col_width = 30;
  const int max_cols = getmaxx(g_window) / col_width;
  int col = 0;  ///< Column (0, or 1).
  int row_top = y + 1;
  int row_bottom_max = row_top;
  for (int t = 0; t < rds_data.af.count; t++) {
    int x = col * col_width;
    int y = DrawAFTable(row_top, x, t + 1, &rds_data.af.table[t].table);
    row_bottom_max = std::max(row_bottom_max, y);
    if (++col == max_cols) {
      col = 0;
      row_top = row_bottom_max + 1;
    }
  }
}

void DrawEON() {
  erase();

  si470x_state_t state;
  if (!si470x_get_state(g_tuner, &state))
    return;
  rds_data rds_data;
  if (!si470x_get_rds_data(g_tuner, &rds_data))
    return;

  int y = DrawHeader(state, rds_data);

  if (!(rds_data.valid_values & RDS_EON)) {
    mvprintw(y, 0, "No EON data");
    return;
  }

  char ps[ARRAY_SIZE(rds_data.eon.on.ps) + 1];
  memcpy(ps, (char*)rds_data.eon.on.ps, sizeof(rds_data.eon.on.ps));
  MakeSpaces(ps, ARRAY_SIZE(ps) - 1);
  ps[ARRAY_SIZE(ps) - 1] = '\0';
  mvprintw(y++, 0, "PS:  [%s]", ps);
  mvprintw(y++, 0, "PTY: %s",
           get_pty_code_name(rds_data.eon.on.pty, REGION_US));
  mvprintw(y++, 0, "Traffic TP: %c, TA: %c",
           rds_data.eon.on.tp_code ? 'Y' : 'N',
           rds_data.eon.on.ta_code ? 'Y' : 'N');

  if (rds_data.eon.on.af.table.count) {
    y++;
    mvprintw(y++, 0, "Alternative Frequencies");
    mvprintw(y++, 0, "=======================");
    char encoding_method;
    switch (rds_data.eon.on.af.enc_method) {
      case AF_EM_UNKNOWN:
        encoding_method = '?';
        break;
      case AF_EM_A:
        encoding_method = 'A';
        break;
      case AF_EM_B:
        encoding_method = 'B';
        break;
    }

    mvprintw(y++, 0, "Encoding method: %c", encoding_method);

    y = DrawAFTable(y, 0, 1, &rds_data.eon.on.af.table);
  }
}

void DrawFooter() {
  const int y = getmaxy(g_window) - 1;

  mvprintw(y, 0,
           "Q/q: Quit, u: Seek up, "
           "d: Seek down, b: Basic, s: Stats, "
           "a: AF table, e: EON");
}

void Draw() {
  g_update_num++;
  g_dirty = false;
  switch (g_draw_mode) {
    case DrawMode::Basic:
      DrawCurrentState();
      break;
    case DrawMode::Stats:
      DrawCurrentStats();
      break;
    case DrawMode::AltFreq:
      DrawAlternativeFrequencies();
      break;
    case DrawMode::EON:
      DrawEON();
      break;
  }
  DrawFooter();
}

}  // namespace

int main(int argc, const char** argv) {
  int ret;

  if (argc == 2) {
#if !defined(RDS_DEV)
    fprintf(stdout, "Can't run with test blocks without RDS_DEV defined\n");
    return 1;
#endif
    auto readl = [](const std::string& fname) {
      RDSTestData test_data;
      test_data.fname = fname;
      if (!LoadRdsSpyFile(fname.c_str(), &test_data.blocks)) {
        fprintf(stderr, "Can't read \"%s\"\n", fname.c_str());
        return 2;
      }
      if (test_data.blocks.empty()) {
        fprintf(stderr, "\"%s\" is empty\n", fname.c_str());
        return 3;
      }
      g_rds_test_data.push_back(test_data);
      return 0;
    };
    struct stat sb;
    if (-1 == stat(argv[1], &sb)) {
      perror("Can't stat file/dir");
      return 5;
    }
    if (S_ISDIR(sb.st_mode)) {
      DIR* dir = opendir(argv[1]);
      if (!dir) {
        perror("Cant open dir");
        return 6;
      }
      struct dirent* ent;
      while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name))
          continue;
        std::string fname = argv[1];
        fname += '/';
        fname += ent->d_name;
        if ((ret = readl(fname)))
          return ret;
      }
      closedir(dir);
    } else {
      if ((ret = readl(argv[1])))
        return ret;
    }
  }

  g_oda_data = create_oda_data();

  struct si470x_port_t* port = port_create(!g_rds_test_data.empty());

  if (argc == 1 && !port_supports_gpio(port)) {
    fprintf(stderr,
            "This port doesn't support GPIO, "
            "can only run with test data\n");
    return 1;
  }
  if (argc == 1 && !port_supports_i2c(port)) {
    fprintf(stderr,
            "This port doesn't support I2C, "
            "can only run with test data\n");
    return 1;
  }

  // These are all wiringPi pin numbers. See http://wiringpi.com/pins/
  const struct si470x_config_t config = {
      .port = port,
      .region = REGION_US,
      .advanced_ps_decoding = true,
      .gpio2_int_pin = 5,  // GPIO5
      .reset_pin = 6,      // GPIO6
      .i2c =
          {
              .bus = 1,
              .sdio_pin = 8,  // GPIO2
              .sclk_pin = 9,  // GPIO3
              .slave_addr = 0x10,
          },
  };
  g_tuner = si470x_create(&config);
  TunerDeleter tuner_deleter;

  if (!g_tuner) {
    fprintf(stderr, "Unable to create the tuner.\n");
    return 1;
  }

  si470x_set_rds_callback(g_tuner, &OnRDSChanged, NULL);
  si470x_set_oda_callbacks(g_tuner, &DecodeODA, &ClearODA, g_oda_data);

  auto power_on_tuner = [=]() {
    if (g_rds_test_data.empty()) {
      if (!si470x_power_on(g_tuner)) {
        fprintf(stderr, "Unable to power on tuner.\n");
        return 1;
      }
    } else {
#if defined(RDS_DEV)
      const uint16_t rds_block_delay_ms = 50;
      const auto& test_data = g_rds_test_data[g_current_block_idx];
      if (!si470x_power_on_test(g_tuner, test_data.blocks.data(),
                                test_data.blocks.size(), rds_block_delay_ms)) {
        fprintf(stderr, "Unable to power on tuner with test data.\n");
        return 1;
      }
#endif  // defined(RDS_DEV)
    }
    g_update_num = 0;
    return 0;
  };

  if ((ret = power_on_tuner()))
    return ret;

  const int frequency = 98500000;
  if (!si470x_set_frequency(g_tuner, frequency)) {
    fprintf(stderr, "Unable to tune to frequency %d.\n", frequency);
    return 1;
  }

  const int volume = 7;
  if (!si470x_set_volume(g_tuner, volume)) {
    fprintf(stderr, "Unable to set volume to %d.\n", volume);
    return 1;
  }

  si470x_set_mute(g_tuner, false);

  si470x_set_soft_mute(g_tuner, false);

  g_window = initscr();
  WindowEnder ender;

  refresh();

  int ch;
  nodelay(stdscr, TRUE);
  bool done = false;

  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto next_update_time = now + kUpdateInterval;
  auto next_tune_time = now + kTuneInterval;
  bool auto_tune = g_rds_test_data.empty();

  while (!done) {
    now = std::chrono::system_clock::now().time_since_epoch();
    if ((ch = getch()) == ERR) {
      // No key.
      bool do_sleep = true;
      if (g_dirty) {
        Draw();
        next_update_time = now + kUpdateInterval;
        do_sleep = false;
      } else if (now >= next_update_time) {
        // Gone too long w/o RDS trigger, poll and update.
        Draw();
        next_update_time = now + kUpdateInterval;
        do_sleep = false;
      }
      if (auto_tune && now >= next_tune_time) {
        bool reached_sfbl;
        si470x_seek_up(g_tuner, /*allow_wrap=*/true, &reached_sfbl);
        next_tune_time = now + kTuneInterval;
        do_sleep = false;
      }
      if (do_sleep)
        std::this_thread::sleep_for(kSleepDuration);
    } else {
      bool reached_sfbl;
      switch (ch) {
        case 'a':
          g_draw_mode = g_draw_mode != DrawMode::AltFreq ? DrawMode::AltFreq
                                                         : DrawMode::Basic;
          g_dirty = true;
          break;
        case 'b':
          g_draw_mode = DrawMode::Basic;
          g_dirty = true;
          break;
        case 's':
          g_draw_mode = g_draw_mode != DrawMode::Stats ? DrawMode::Stats
                                                       : DrawMode::Basic;
          g_dirty = true;
          break;
        case 'e':
          g_draw_mode =
              g_draw_mode != DrawMode::EON ? DrawMode::EON : DrawMode::Basic;
          g_dirty = true;
          break;
        case 'u':
          auto_tune = false;
          if (g_rds_test_data.empty()) {
            si470x_seek_up(g_tuner, /*allow_wrap=*/true, &reached_sfbl);
          } else {
            if (g_current_block_idx++ >= g_rds_test_data.size() - 1)
              g_current_block_idx = 0;
            if (!si470x_power_off(g_tuner)) {
              return 1;
            }
            if ((ret = power_on_tuner()))
              return ret;
          }
          g_dirty = true;
          break;
        case 'd':
          auto_tune = false;
          if (g_rds_test_data.empty()) {
            si470x_seek_down(g_tuner, /*allow_wrap=*/true, &reached_sfbl);
          } else {
            if (g_current_block_idx-- == 0)
              g_current_block_idx = g_rds_test_data.size() - 1;
            if (!si470x_power_off(g_tuner))
              return 1;
            if ((ret = power_on_tuner()))
              return ret;
          }
          g_dirty = true;
          break;
        case 'q':
        case 'Q':
          done = true;
          break;
      }
    }
  }

  return 0;
}
