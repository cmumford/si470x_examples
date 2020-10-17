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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <si470x.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct rds_oda_data {
  struct {
    char text[65][64];  ///< 65 64-bit text strings.
  } rtplus;             ///< Radiotext Plus (AKA RT+).
  struct {
    struct {
      bool tuning;        ///< Tuning information (or reserved for future use).
      bool single_group;  ///< true=single group, false=multi-group.
      uint8_t dp;         ///< Duration and persistence value.
      bool diversion;     ///< Advised to follow indicated diversion.
      bool pos_dir;       ///< Positive direction (else negative direction).
      uint8_t extent;
      uint16_t event;
      uint16_t location;
    } group;  ///< RDS-TMC single group user messages.
    struct {
      uint8_t variant_code;  ///< Variant: 0 or 1.
      union {
        struct {
          uint8_t X;
          uint8_t ltn;  ///< Location Table Number (ISO 14819-3).
          bool afi;     ///< Alternative Frequency Indicator.
          struct {
            bool i;  ///< International.
            bool n;  ///< National.
            bool r;  ///< Regional.
            bool u;  ///< Urban.
          } mgs;     ///< Message Geographic Scope.
        } v0;        ///< Variant 0 system message.
        struct {
          uint8_t g;    ///< Gap parameter.
          uint8_t sid;  ///< Service identifier.
          uint8_t ta;   ///< Activity Time.
          uint8_t tw;   ///< Window Time.
          uint8_t td;   ///< Delay Time.
        } v1;           ///< Variant 1 system message.
      } variant;        ///< System message variants (0 or 1).
    } system;           ///< RDS-TMS System messages.
  } tmc;                ///< RDS-TMC messages.
  struct {
    uint16_t rtplus_cnt;  ///< # of RT+ groups processed.
    uint16_t tmc_cnt;     ///< # of RDS-TMC groups processed.
    uint16_t itunes_cnt;  ///< # of iTunes groups processed.
  } stats;                ///< ODA statistics.
};

/**
 * Create an instance of struct rds_oda_data for processing of RDS ODA blocks.
 */
struct rds_oda_data* create_oda_data();

/**
 * Delete the oda_data item.
 */
void delete_oda_data(struct rds_oda_data* oda_data);

/**
 * Set all values in ora_data to their default values.
 */
void clear_oda_data(struct rds_oda_data* oda_data);

/**
 * Decode the block data, writing any new ODA data to oda_data.
 */
void decode_oda_blocks(struct rds_oda_data* oda_data,
                       uint16_t app_id,
                       const struct rds_data* rds,
                       const struct rds_blocks* blocks,
                       struct rds_group_type gt);

/**
 * Get a user displayable representation for app_id.
 */
void get_app_name(char* buffer, uint16_t buffer_len, uint16_t app_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */
