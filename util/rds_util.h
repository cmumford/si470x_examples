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

#if !defined(UNUSED)
#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)
#endif

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Decode the program identification code (pi_code) value into a displayable
 * string.
 */
bool decode_pi_code(char* buffer,
                    size_t buffer_len,
                    uint16_t pi_code,
                    enum si470x_region_t region);

const char* get_rdsplus_code_name(uint16_t code_id);

const char* get_pty_code_name(uint8_t pty_code, enum si470x_region_t region);

const char* get_device_name(enum si470x_device_t device);

void get_manufacturer_name(uint16_t id, char* name, size_t name_len);

void format_local_time(char* buff, uint8_t bufflen, const struct rds_data* rds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
