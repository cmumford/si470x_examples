#include <curses.h>
#include <string.h>
#include <unistd.h>

#include <atomic>
#include <memory>
#include <vector>

#include <oda_decode.h>
#include <port_unix.h>
#include <rds_spy_log_reader.h>
#include <rds_util.h>
#include <si470x.h>

namespace {

#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

struct WindowEnder {
  ~WindowEnder() { endwin(); }
};

struct si470x* g_tuner;
struct rds_oda_data* g_oda_data;
std::atomic<bool> g_dirty;

void OnRDSChanged(void*) {
  g_dirty = true;
}

void DecodeODA(uint16_t app_id,
               const struct rds_data* rds,
               const struct rds_blocks* blocks,
               struct rds_group_type gt,
               void* user_data) {
  struct rds_oda_data* oda_data = (struct rds_oda_data*)user_data;
  decode_oda_blocks(oda_data, app_id, rds, blocks, gt);
}

void ClearODA(void* user_data) {
  struct rds_oda_data* oda_data = (struct rds_oda_data*)user_data;
  clear_oda_data(oda_data);
}

void DrawCurrentStats() {
  erase();
  int y = 0;
  mvprintw(y++, 0, "Group     A       B");
  mvprintw(y++, 0, "-----  ------- -------");
#if defined(RDS_DEV)
  struct rds_data rds_data;
  g_dirty = false;
  si470x_get_rds_data(g_tuner, &rds_data);
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

  y = 0;
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

void PrintStats(FILE* f) {
#if defined(RDS_DEV)
  struct rds_data rds_data;
  si470x_get_rds_data(g_tuner, &rds_data);

  fprintf(f, "RDS: %u\n", rds_data.stats.data_cnt);
  fprintf(f, "BERR: %u\n", rds_data.stats.blckb_errors);
  for (int i = 0; i < 16; i++) {
    fprintf(f, "%dA: %d\n", i, rds_data.stats.groups[i].a);
    fprintf(f, "%dB: %d\n", i, rds_data.stats.groups[i].b);
  }

  printf("AF: %d\n", rds_data.stats.counts[PKTCNT_AF]);
  printf("CLOCK: %d\n", rds_data.stats.counts[PKTCNT_CLOCK]);
  printf("EON: %d\n", rds_data.stats.counts[PKTCNT_EON]);
  printf("EWS: %d\n", rds_data.stats.counts[PKTCNT_EWS]);
  printf("FBT: %d\n", rds_data.stats.counts[PKTCNT_FBT]);
  printf("IH: %d\n", rds_data.stats.counts[PKTCNT_IH]);
  printf("MS: %d\n", rds_data.stats.counts[PKTCNT_MS]);
  printf("PAGING: %d\n", rds_data.stats.counts[PKTCNT_PAGING]);
  printf("PI_CODE: %d\n", rds_data.stats.counts[PKTCNT_PI_CODE]);
  printf("PS: %d\n", rds_data.stats.counts[PKTCNT_PS]);
  printf("PTY: %d\n", rds_data.stats.counts[PKTCNT_PTY]);
  printf("PTYN: %d\n", rds_data.stats.counts[PKTCNT_PTYN]);
  printf("RT: %d\n", rds_data.stats.counts[PKTCNT_RT]);
  printf("SLC: %d\n", rds_data.stats.counts[PKTCNT_SLC]);
  printf("TA_CODE: %d\n", rds_data.stats.counts[PKTCNT_TA_CODE]);
  printf("TDC: %d\n", rds_data.stats.counts[PKTCNT_TDC]);
  printf("TMC: %d\n", rds_data.stats.counts[PKTCNT_TMC]);
  printf("TP_CODE: %d\n", rds_data.stats.counts[PKTCNT_TP_CODE]);

  fprintf(f, "RT+: %d\n", g_oda_data->stats.rtplus_cnt);
  fprintf(f, "RDS-TMC: %d\n", g_oda_data->stats.tmc_cnt);
  fprintf(f, "iTunes: %d\n", g_oda_data->stats.itunes_cnt);
#else
  UNUSED(f);
#endif
}

}  // namespace

int main(int argc, const char** argv) {
  bool is_terminal = isatty(fileno(stdout));

  std::vector<struct rds_blocks> blocks;
  if (argc == 2) {
    if (!LoadRdsSpyFile(argv[1], &blocks)) {
      fprintf(stderr, "Can't read \"%s\"\n", argv[1]);
      return 2;
    }
    if (blocks.empty()) {
      fprintf(stderr, "\"%s\" is empty\n", argv[1]);
      return 3;
    }
  }

  g_oda_data = create_oda_data();

  // These are all wiringPi pin numbers. See http://wiringpi.com/pins/
  const struct si470x_config config = {
      .region = REGION_US,
      .advanced_ps_decoding = true,
      .gpio2_int_pin = 5,  // GPIO5
      .reset_pin = 6,      // GPIO6
      .sdio_pin = 8,       // GPIO2
      .sclk_pin = 9,       // GPIO3
  };
  const struct port port_config {
    .delay = port_delay, .enable_gpio = port_enable_gpio,
    .set_pin_mode = port_set_pin_mode, .digital_write = port_digital_write,
    .set_interrupt_handler = port_set_interrupt_handler,
    .set_i2c_addr = port_set_i2c_addr,
    .enable_i2c_packet_error_checking = port_enable_i2c_packet_error_checking
  };
  g_tuner = si470x_create(&config, &port_config);
  if (!g_tuner) {
    fprintf(stderr, "Unable to create the tuner.\n");
    return 1;
  }

  si470x_set_rds_callback(g_tuner, &OnRDSChanged, NULL);
  si470x_set_oda_callbacks(g_tuner, &DecodeODA, &ClearODA, g_oda_data);

  if (is_terminal)
    initscr();
  std::unique_ptr<WindowEnder> ender(is_terminal ? new WindowEnder() : nullptr);

  if (blocks.empty()) {
    if (!si470x_power_on(g_tuner)) {
      fprintf(stderr, "Unable to power on tuner.\n");
      return 1;
    }
  } else {
#if defined(RDS_DEV)
    const uint16_t block_delay_ms = is_terminal ? 50 : 0;
    if (!si470x_power_on_test(g_tuner, blocks.data(), blocks.size(),
                              block_delay_ms)) {
      fprintf(stderr, "Unable to power on tuner with test data.\n");
      return 1;
    }
#endif  // defined(RDS_DEV)
  }

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

  if (is_terminal) {
    mvprintw(30, 0, "Press any key to quit.");
    refresh();
  }

  int ch;
  if (is_terminal)
    nodelay(stdscr, TRUE);
  bool done = false;
  uint32_t loop_count = 0;            // Number of times through key/radio loop.
  const uint32_t update_seconds = 1;  // Update minumum once / second.
  const uint32_t update_msecs = update_seconds * 1000;  // Update every X ms.
  const uint32_t sleep_msecs = 5;  // Sleep for X msec every update.
  const uint32_t update_interval = update_msecs / sleep_msecs;
  while (!done) {
#if defined(RDS_DEV)
    if (!blocks.empty() && !si470x_rds_test_running(g_tuner)) {
      done = true;
    }
#endif  // defined(RDS_DEV)
    if (!is_terminal)
      continue;
    if ((ch = getch()) == ERR) {
      // No key.
      if (g_dirty) {
        g_dirty = false;
        loop_count = 0;
        DrawCurrentStats();
      } else {
        usleep(1000 * sleep_msecs);
        if (loop_count++ >= update_interval) {
          // Gone too long w/o RDS trigger, poll and update.
          loop_count = 0;
          DrawCurrentStats();
        }
      }
    } else {
      switch (ch) {
        default:
          done = true;
      }
    }
  }

  if (!si470x_power_off(g_tuner)) {
    fprintf(stderr, "Unable to power off tuner.\n");
    return 1;
  }
  delete_oda_data(g_oda_data);

  ender.reset();

  PrintStats(stdout);

  return 0;
}
