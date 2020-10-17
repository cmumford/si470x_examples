#include <mgos.h>
#include <mgos_rpc.h>

#include <mgos_si470x.h>
#include <rds_util.h>
#include <ssd1306.h>

#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

struct app_data {
  struct si470x_t* tuner;
  struct rds_data* rds_data;
  bool continuous_seek;
  bool dirty;
  struct mgos_ssd1306* display;
  double last_draw_time;
  uint32_t update_num;
};

const int kFixedFont = 0;
const int kVariableFont = 1;
const int kStatusHeight = 16;

static bool HasAnyText(const char* str) {
  size_t len = strlen(str);
  for (size_t i = 0; i < len; i++) {
    if (str[i] != ' ')
      return true;
  }
  return false;
}

static bool IsPrintableChar(char ch) {
  return ch >= 32 && ch < 127;
}

static void MakeSpaces(char* str, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (!IsPrintableChar(str[i]))
      str[i] = ' ';
  }
}

static bool ContainsTime(const struct rds_data* rds) {
  return rds->clock.day_high || rds->clock.day_low || rds->clock.hour ||
         rds->clock.minute;
}

static bool IsSpace(char ch) {
  // Note: 0xa (newline) is a legal character for PTYN.
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static void TrimTrailingWhitespace(char* str) {
  char* ch = str + strlen(str) - 1;
  while (ch >= str) {
    if (IsSpace(*ch))
      *ch-- = '\0';
    else
      break;
  }
}

/*
 * Update the display.
 */
static void UpdateDisplayCb(void* arg) {
  struct app_data* app = (struct app_data*)arg;
  app->dirty = false;
  app->update_num++;
  if (!app->display)
    return;
  const uint64_t draw_start = mgos_uptime_micros();

  mgos_ssd1306_clear(app->display);
  mgos_ssd1306_select_font(app->display, kVariableFont);

  const int width = mgos_ssd1306_get_width(app->display);
  const int height = mgos_ssd1306_get_height(app->display);
  const int line_height = mgos_ssd1306_get_font_height(app->display);

  if (!app->tuner) {
    mgos_ssd1306_draw_string(app->display, 0, 0, "No tuner.");
    goto UPDATE_DONE;
  }

  struct si470x_state_t state;
  if (!mgos_si470x_get_state(app->tuner, &state)) {
    LOG(LL_ERROR, ("Unable to get tuner state."));
    mgos_ssd1306_draw_string(app->display, 0, 0, "Error getting state.");
    goto UPDATE_DONE;
  }
  struct rds_data* rds = app->rds_data;
  if (!mgos_si470x_get_rds_data(app->tuner, rds)) {
    LOG(LL_ERROR, ("Unable to get tuner RDS data."));
    mgos_ssd1306_draw_string(app->display, 0, 0, "Error getting RDS data.");
    goto UPDATE_DONE;
  }

  char ps[ARRAY_SIZE(rds->ps.display) + 1];
  memcpy(ps, (char*)rds->ps.display, sizeof(rds->ps.display));
  MakeSpaces(ps, ARRAY_SIZE(ps) - 1);
  ps[ARRAY_SIZE(ps) - 1] = '\0';

  const struct rds_rt* rtext =
      rds->rt.decode_rt == RT_A ? &rds->rt.a : &rds->rt.b;
  char rt[ARRAY_SIZE(rtext->display) + 1];
  memcpy(rt, (char*)rtext->display, sizeof(rtext->display));
  MakeSpaces(rt, ARRAY_SIZE(rt) - 1);
  rt[ARRAY_SIZE(rt) - 1] = '\0';
  TrimTrailingWhitespace(rt);

  char picode[40];
  if (!decode_pi_code(picode, ARRAY_SIZE(picode), rds->pi_code, REGION_US))
    picode[0] = '\0';

  int y = 0;
  char buff[80];
  const int kBuffSize = ARRAY_SIZE(buff);

  {
    // Draw RSSI graph
    const int kMaxRSSI = 60;
    const int kBarHeight = 2;
    int bar_width = width * state.rssi / kMaxRSSI;
    if (bar_width > width)
      bar_width = width;
    const int kBarTop = kStatusHeight - kBarHeight;
    mgos_ssd1306_fill_rectangle(app->display, 0, kBarTop, bar_width, kBarHeight,
                                SSD1306_COLOR_WHITE);
  }

  // Header.
  {
    snprintf(buff, ARRAY_SIZE(buff), "%.1f MHz (%s)", state.frequency / 1e6,
             picode);
    mgos_ssd1306_draw_string(app->display, 0, y, buff);
    y = kStatusHeight;
  }

  {
    mgos_ssd1306_draw_string(app->display, 0, y,
                             get_pty_code_name(rds->pty, REGION_US));
    const int kDbWidth = 30;
    snprintf(buff, kBuffSize, "%d Db", state.rssi);
    mgos_ssd1306_draw_string(app->display, width - kDbWidth, y, buff);
    y += line_height;
  }

  {
    snprintf(buff, kBuffSize, "[%s]/%c", ps,
             mgos_sys_config_get_si470x_advanced_ps() ? 'A' : 'B');
    buff[kBuffSize - 1] = '\0';
    mgos_ssd1306_select_font(app->display, kFixedFont);
    mgos_ssd1306_draw_string(app->display, 0, y, buff);
    mgos_ssd1306_select_font(app->display, kVariableFont);
    const int kMSWidth = 40;
    snprintf(buff, kBuffSize, "%.01f ms", app->last_draw_time);
    buff[kBuffSize - 1] = '\0';
    mgos_ssd1306_draw_string(app->display, width - kMSWidth, y, buff);
    y += line_height;
  }

  {
    snprintf(buff, kBuffSize, "\"%s\"", rt);
    buff[kBuffSize - 1] = '\0';
    mgos_ssd1306_draw_string(app->display, 0, y, buff);
    y += line_height;
  }

UPDATE_DONE:

  // Draw the update counter.
  {
    const int kWidth = 85;
    snprintf(buff, kBuffSize, "draw: %u", app->update_num);
    buff[kBuffSize - 1] = '\0';
    mgos_ssd1306_draw_string(app->display, width - kWidth, height - line_height,
                             buff);
    y += line_height;
  }

  mgos_ssd1306_refresh(app->display, /*force=*/false);
  app->last_draw_time = (mgos_uptime_micros() - draw_start) / 1000;
}

static void LogStateCb(void* arg) {
  struct app_data* app = (struct app_data*)arg;
  app->dirty = false;

  struct si470x_state_t state;
  struct rds_data* rds = app->rds_data;
  if (!mgos_si470x_get_state(app->tuner, &state)) {
    LOG(LL_ERROR, ("Unable to get tuner state."));
    return;
  }
  if (!mgos_si470x_get_rds_data(app->tuner, rds)) {
    LOG(LL_ERROR, ("Unable to get RDS data."));
    return;
  }

  char ps[ARRAY_SIZE(rds->ps.display) + 1];
  memcpy(ps, (char*)rds->ps.display, sizeof(rds->ps.display));
  MakeSpaces(ps, ARRAY_SIZE(ps) - 1);
  ps[ARRAY_SIZE(ps) - 1] = '\0';

  const struct rds_rt* rtext =
      rds->rt.decode_rt == RT_A ? &rds->rt.a : &rds->rt.b;
  char rt[ARRAY_SIZE(rtext->display) + 1];
  memcpy(rt, (char*)rtext->display, sizeof(rtext->display));
  MakeSpaces(rt, ARRAY_SIZE(rt) - 1);
  rt[ARRAY_SIZE(rt) - 1] = '\0';
  TrimTrailingWhitespace(rt);

  char ptyn[ARRAY_SIZE(rds->ptyn.display) + 1];
  memcpy(ptyn, (char*)rds->ptyn.display, sizeof(rds->ptyn.display));
  MakeSpaces(ptyn, ARRAY_SIZE(ptyn) - 1);
  ptyn[ARRAY_SIZE(ptyn) - 1] = '\0';
  TrimTrailingWhitespace(ptyn);

  char picode[40];
  if (!decode_pi_code(picode, ARRAY_SIZE(picode), rds->pi_code, REGION_US)) {
    picode[0] = '\0';
  }

  LOG(LL_INFO, ("%.1f MHz (%s)@%d, PS:\"%s\" PTYN:\"%s\"",
                state.frequency / 1e6, picode, state.rssi, ps, ptyn));
  if (HasAnyText(rt))
    LOG(LL_INFO, ("     RT:\"%s\"", rt));
  if (ContainsTime(rds)) {
    char buffer[40];
    format_local_time(buffer, ARRAY_SIZE(buffer), rds);
    LOG(LL_INFO, ("     Clock: %s", buffer));
  }
}

static void TuneCb(void* arg) {
  struct app_data* app = (struct app_data*)arg;

  if (!app->continuous_seek)
    return;

  bool reached_sfbl;
  int new_freq =
      mgos_si470x_seek_up(app->tuner, /*allow_wrap=*/false, &reached_sfbl);
  struct si470x_state_t state;
  mgos_si470x_get_state(app->tuner, &state);
  if (new_freq == -1) {
    LOG(LL_ERROR, ("ERROR seeking"));
  } else {
    LOG(LL_INFO,
        ("Seeked up to new freq:%.1f MHz, chan:%d, RSSI:%d, SF/BL:%c",
         new_freq / 1e6, state.channel, state.rssi, reached_sfbl ? 'Y' : 'N'));
  }
}

static void GetStateCb(struct mg_rpc_request_info* ri,
                       void* cb_arg,
                       struct mg_rpc_frame_info* fi,
                       struct mg_str args) {
  UNUSED(fi);
  UNUSED(args);

  struct app_data* app = (struct app_data*)cb_arg;

  struct si470x_state_t state;
  if (mgos_si470x_get_state(app->tuner, &state)) {
    struct rds_data* rds = app->rds_data;
    char ps[ARRAY_SIZE(rds->ps.display) + 1];
    if (mgos_si470x_get_rds_data(app->tuner, rds))
      memcpy(ps, rds->ps.display, sizeof(rds->ps.display));
    else
      memset(ps, 0, sizeof(rds->ps.display));
    MakeSpaces(ps, sizeof(ps) - 1);
    ps[ARRAY_SIZE(ps) - 1] = '\0';

    mg_rpc_send_responsef(ri,
                          "{"
                          "enabled:%B,"
                          "manufacturer:%Q,"
                          "firmware:%d,"
                          "device:%d,"
                          "revision:\"%c\","
                          "frequency:%d,"
                          "channel:%d,"
                          "volume:%d,"
                          "stereo:%B,"
                          "rssi:%u,"
                          "PS:%Q"
                          "}",
                          state.enabled, state.manufacturer, state.firmware,
                          state.device, state.revision, state.frequency,
                          state.channel, state.volume, state.stereo, state.rssi,
                          ps);
  } else {
    mg_rpc_send_errorf(ri, -1, "Call failed.");
  }
}

static void DoTuneCb(struct mg_rpc_request_info* ri,
                     void* cb_arg,
                     struct mg_rpc_frame_info* fi,
                     struct mg_str args) {
  UNUSED(fi);

  struct app_data* app = (struct app_data*)cb_arg;
  double freq_mhz = 0.0;
  app->continuous_seek = false;
  if (json_scanf(args.p, args.len, ri->args_fmt, &freq_mhz) == 1) {
    int freq_hz = freq_mhz * 1000000.0;
    if (!mgos_si470x_set_frequency(app->tuner, freq_hz))
      mg_rpc_send_errorf(ri, -1, "Can't tune radio to %lf", freq_mhz);
    else
      mg_rpc_send_responsef(ri, "%.01lf", freq_mhz);
  } else {
    mg_rpc_send_errorf(ri, -1, "Can't parse freq arg.");
  }
}

static void AddRPCHandlers(struct app_data* app) {
  mg_rpc_add_handler(mgos_rpc_get_global(), "SI470X.State", NULL, GetStateCb,
                     app);

  mg_rpc_add_handler(mgos_rpc_get_global(), "SI470X.Tune", "%lf", DoTuneCb,
                     app);
}

/**
 * Called with the RDS data has changed.
 *
 * NOTE: This can be quite frequent (every 50 msec. or so).
 */
static void OnRDSChanged(void* data) {
  struct app_data* app = (struct app_data*)data;
  app->dirty = true;

  if (mgos_sys_config_get_app_rds_activity_gpio() >= 0)
    mgos_gpio_toggle(mgos_sys_config_get_app_rds_activity_gpio());
}

static struct app_data* CreateAppData() {
  struct app_data* app = (struct app_data*)calloc(1, sizeof(struct app_data));
  if (!app)
    return NULL;

  app->rds_data = (struct rds_data*)calloc(1, sizeof(struct rds_data));
  if (!app->rds_data) {
    free(app);
    return NULL;
  }

  if (mgos_sys_config_get_ssd1306_enable()) {
    app->display = mgos_ssd1306_get_global();
    if (!app->display)
      LOG(LL_INFO, ("No display connected."));
  }

  return app;
}

bool CreateTuner(struct app_data* app) {
  app->tuner = mgos_si470x_get_global();
  if (!app->tuner) {
    LOG(LL_ERROR, ("Unable to get the tuner."));
    return false;
  }
  LOG(LL_INFO, ("Created the tuner."));
  mgos_si470x_set_rds_callback(app->tuner, &OnRDSChanged, app);

  if (!mgos_si470x_power_on(app->tuner)) {
    LOG(LL_ERROR, ("Unable to power on tuner."));
    return false;
  }
  LOG(LL_INFO, ("Tuner is powered on."));

  const int frequency = 98500000;
  if (!mgos_si470x_set_frequency(app->tuner, frequency)) {
    LOG(LL_ERROR, ("Unable to tune to frequency %d.", frequency));
    return false;
  }
  LOG(LL_INFO, ("Tuned to %.1f MHz.", frequency / 1e6));

  const int volume = 7;
  if (!mgos_si470x_set_volume(app->tuner, volume)) {
    LOG(LL_ERROR, ("Unable to set volume to %d.", volume));
    return false;
  }
  LOG(LL_INFO, ("Set volume to %d.", volume));

  mgos_si470x_set_mute(app->tuner, false);
  mgos_si470x_set_soft_mute(app->tuner, false);

  return true;
}

enum mgos_app_init_result mgos_app_init(void) {
  struct app_data* app = CreateAppData();

  const int activity_pin = mgos_sys_config_get_app_rds_activity_gpio();
  if (activity_pin >= 0) {
    mgos_gpio_setup_output(activity_pin, false);
  }

  if (!CreateTuner(app))
    LOG(LL_ERROR, ("Error creating tuner."));

  if (app->display) {
    LOG(LL_INFO, ("Not adding state/tune timers - using display."));
    const int display_refresh_ms = 250;
    mgos_set_timer(display_refresh_ms, MGOS_TIMER_REPEAT, UpdateDisplayCb, app);
  } else {
    mgos_set_timer(2000 /* ms */, MGOS_TIMER_REPEAT, LogStateCb, app);
  }
  app->continuous_seek = true;
  if (false)
    mgos_set_timer(10000 /* ms */, MGOS_TIMER_REPEAT, TuneCb, app);

  AddRPCHandlers(app);

  return MGOS_APP_INIT_SUCCESS;
}
