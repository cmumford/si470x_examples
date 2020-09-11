/**
 * @file
 *
 * @author Chris Mumford
 *
 * @license
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "oda_decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

// clang-format off
//
// http://www.rds.org.uk/2010/pdf/R17_032_1.pdf
#define AID_RT_PLUS 0x4BD7 // Radiotext Plus (RT+).
#define AID_TMC     0xCD46
#define AID_ITUNES  0xC3B0 // iTunes tagging.

// clang-format on

struct rds_oda_data* create_oda_data() {
  return (struct rds_oda_data*)calloc(1, sizeof(struct rds_oda_data));
}

void delete_oda_data(struct rds_oda_data* oda_data) {
  if (!oda_data)
    return;
  free(oda_data);
}

/**
 * Decode Radiotext plus (RT+) data.
 *
 * See https://tech.ebu.ch/docs/techreview/trev_307-radiotext.pdf
 */
static void decode_rt_plus(struct rds_oda_data* oda,
                           const struct rds_data* rds,
                           const struct rds_blocks* blocks) {
  // clang-format off
  //const uint16_t B_ITEM_TOGGLE     = 0b0000000000010000;
  //const uint16_t B_ITEM_RUNNING    = 0b0000000000001000;
  const uint16_t B_CONTENT_TYPE_1  = 0b0000000000000111;
  const uint16_t C_CONTENT_TYPE_1  = 0b1110000000000000;
  const uint16_t C_START_MARKER_1  = 0b0001111110000000;
  const uint16_t C_LENGTH_MARKER_1 = 0b0000000001111110;
  const uint16_t C_CONTENT_TYPE_2  = 0b0000000000000001;
  const uint16_t D_CONTENT_TYPE_2  = 0b1111100000000000;
  const uint16_t D_START_MARKER_2  = 0b0000011111100000;
  const uint16_t D_LENGTH_MARKER_2 = 0b0000000000011111;
  // clang-format on

  if (blocks->b.errors > BLERB_MAX)
    return;
  if (blocks->c.errors > BLERC_MAX)
    return;
  if (blocks->d.errors > BLERD_MAX)
    return;

  // const bool item_toggle = blocks->b.val & B_ITEM_TOGGLE;
  // const bool item_running = blocks->b.val & B_ITEM_RUNNING;
  const uint16_t content_type1 = ((blocks->b.val & B_CONTENT_TYPE_1) << 3) |
                                 ((blocks->c.val & C_CONTENT_TYPE_1) >> 13);
  uint16_t start = (blocks->c.val & C_START_MARKER_1) >> 7;
  uint16_t length = (blocks->c.val & C_LENGTH_MARKER_1) >> 1;

  const struct rds_rt* rt = rds->rt.decode_rt == RT_A ? &rds->rt.a : &rds->rt.b;

  if (content_type1 > 0 && content_type1 <= 63) {  // Valid content type.
    if ((length == 0 && rt->display[start] == ' ')) {
      memset(oda->rtplus.text[content_type1], 0,
             sizeof(oda->rtplus.text[content_type1]));
    } else {
      memset(oda->rtplus.text[content_type1], 0,
             sizeof(oda->rtplus.text[content_type1]));
      memcpy(oda->rtplus.text[content_type1], &rt->display[start], length + 1);
    }
  }

  const uint16_t content_type2 = ((blocks->c.val & C_CONTENT_TYPE_2) << 5) |
                                 ((blocks->d.val & D_CONTENT_TYPE_2) >> 11);
  start = (blocks->d.val & D_START_MARKER_2) >> 5;
  length = blocks->d.val & D_LENGTH_MARKER_2;

  if (content_type2 > 0 && content_type2 <= 63) {  // Valid content type.
    if ((content_type1 != content_type2) && length == 0 &&
        rt->display[start] == ' ') {
      memset(oda->rtplus.text[content_type2], 0,
             sizeof(oda->rtplus.text[content_type2]));
    } else {
      memset(oda->rtplus.text[content_type2], 0,
             sizeof(oda->rtplus.text[content_type2]));
      memcpy(oda->rtplus.text[content_type2], &rt->display[start], length + 1);
    }
  }
}

static void decode_tmc_system_var0(struct rds_oda_data* data,
                                   const struct rds_blocks* blocks) {
  // clang-format off

// Variant 0.
#define C_TMC_VARIANT      0b1100000000000000 // 00=variant 0, 01=variant 1.
#define C_TMC_VARI_0_X     0b0011000000000000
#define C_TMC_VARI_0_LTN   0b0000111111000000 // Location Table Number.
#define C_TMC_VARI_0_AFI   0b0000000000100000 // Alternative Frequency Ind.
#define C_TMC_VARI_0_M     0b0000000000010000 // Mode of transmission.
// MGS = Message Geographic Scope.
#define C_TMC_VARI_0_MGS_I 0b0000000000001000 // MGS International.
#define C_TMC_VARI_0_MGS_N 0b0000000000000100 // MGS National.
#define C_TMC_VARI_0_MGS_R 0b0000000000000010 // MGS Regional.
#define C_TMC_VARI_0_MGS_U 0b0000000000000001 // MGS Urban.

  // clang-format on
  data->tmc.system.variant.v0.ltn = (blocks->c.val & C_TMC_VARI_0_LTN) >> 6;
  data->tmc.system.variant.v0.afi = blocks->c.val & C_TMC_VARI_0_AFI;
  data->tmc.system.variant.v0.mgs.i = blocks->c.val & C_TMC_VARI_0_MGS_I;
  data->tmc.system.variant.v0.mgs.n = blocks->c.val & C_TMC_VARI_0_MGS_N;
  data->tmc.system.variant.v0.mgs.r = blocks->c.val & C_TMC_VARI_0_MGS_R;
  data->tmc.system.variant.v0.mgs.u = blocks->c.val & C_TMC_VARI_0_MGS_U;
}

static void decode_tmc_system_var1(struct rds_oda_data* data,
                                   const struct rds_blocks* blocks) {
  // clang-format off

#define C_TMC_VARI_1_G    0b0011000000000000 // Gap parameter.
#define C_TMC_VARI_1_SID  0b0000111111000000 // Service identifier.
#define C_TMC_VARI_1_TA   0b0000000000110000 // Activity time. (if M=1).
#define C_TMC_VARI_1_TW   0b0000000000001100 // Window time. (if M=1).
#define C_TMC_VARI_1_TD   0b0000000000000011 // Delay time, (if M=1).

  // clang-format on

  data->tmc.system.variant.v1.g = (blocks->c.val & C_TMC_VARI_1_G) >> 12;
  data->tmc.system.variant.v1.sid = (blocks->c.val & C_TMC_VARI_1_SID) >> 6;
  data->tmc.system.variant.v1.ta = (blocks->c.val & C_TMC_VARI_1_TA) >> 4;
  data->tmc.system.variant.v1.tw = (blocks->c.val & C_TMC_VARI_1_TW) >> 2;
  data->tmc.system.variant.v1.td = blocks->c.val & C_TMC_VARI_1_TD;
}

/**
 * Decode the RTL-TMC data stored in group 3A.
 */
static void decode_tmc_3A(struct rds_oda_data* data,
                          const struct rds_blocks* blocks) {
  data->tmc.system.variant_code = (blocks->c.val & C_TMC_VARIANT) >> 14;
  if (data->tmc.system.variant_code == 0)
    decode_tmc_system_var0(data, blocks);
  else
    decode_tmc_system_var1(data, blocks);
}

/**
 * Decode the RTL-TMC data stored in group 8A.
 */
static void decode_tmc_8A(struct rds_oda_data* data,
                          const struct rds_blocks* blocks) {
  // clang-format off

#define B_TMC_TUNING      0b0000000000010000
#define B_TMC_F           0b0000000000001000
#define B_TMC_DP          0b0000000000000111
#define C_TMC_DIVERSION   0b1000000000000000
#define C_TMC_DIRECTION   0b0100000000000000
#define C_TMC_EXTENT      0b0011100000000000
#define C_TMC_EVENT       0b0000011111111111

  // clang-format on

  data->tmc.group.tuning = blocks->b.val & B_TMC_TUNING;
  data->tmc.group.single_group = blocks->b.val & B_TMC_F;
  data->tmc.group.dp = blocks->b.val & B_TMC_DP;
  data->tmc.group.diversion = blocks->c.val & C_TMC_DIVERSION;
  data->tmc.group.pos_dir = blocks->c.val & C_TMC_DIRECTION;
  data->tmc.group.extent = (blocks->c.val & C_TMC_EXTENT) >> 11;
  data->tmc.group.event = blocks->c.val & C_TMC_EVENT;
  data->tmc.group.location = blocks->d.val;
}

/**
 * Decode the RTL-TMC data as per ISO 14819-1.
 */
static void decode_tmc(struct rds_oda_data* data,
                       const struct rds_blocks* blocks,
                       struct rds_group_type gt) {
  if (gt.code == 8 && gt.version == 'A') {
    decode_tmc_8A(data, blocks);
  } else if (gt.code == 3 && gt.version == 'A') {
    decode_tmc_3A(data, blocks);
  }
}

static void decode_itunes(struct rds_oda_data* data) {
  UNUSED(data);
}

void decode_oda_blocks(struct rds_oda_data* oda_data,
                       uint16_t app_id,
                       const struct rds_data* rds,
                       const struct rds_blocks* blocks,
                       struct rds_group_type gt) {
  switch (app_id) {
    case AID_RT_PLUS:
      oda_data->stats.rtplus_cnt++;
      decode_rt_plus(oda_data, rds, blocks);
      break;
    case AID_TMC:
      oda_data->stats.tmc_cnt++;
      decode_tmc(oda_data, blocks, gt);
      break;
    case AID_ITUNES:
      oda_data->stats.itunes_cnt++;
      decode_itunes(oda_data);
      break;
    case 0x0:
      break;
  }
}

void clear_oda_data(struct rds_oda_data* oda_data) {
  memset(oda_data, 0, sizeof(*oda_data));
}

void get_app_name(char* buffer, uint16_t buffer_len, uint16_t app_id) {
  if (buffer_len == 0)
    return;
  switch (app_id) {
    case AID_RT_PLUS:
      strncpy(buffer, "RT+", buffer_len);
      break;
    case AID_TMC:
      strncpy(buffer, "RDS-TMC", buffer_len);
      break;
    case AID_ITUNES:
      strncpy(buffer, "iTunes", buffer_len);
      break;
    default:
      snprintf(buffer, buffer_len, "0x%X", app_id);
  }
  buffer[buffer_len - 1] = '\0';
}
