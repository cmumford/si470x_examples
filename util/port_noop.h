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

#include <port.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Delay the specified number of milliseconds.
 */
void port_noop_delay(uint16_t msec);

/**
 * Enable GPIO for this process.
 */
bool port_noop_enable_gpio();

/**
 * Set a pin to input or output.
 */
void port_noop_set_pin_mode(uint16_t pin, enum pin_mode mode);

/**
 * Set a pin to high or low.
 */
void port_noop_digital_write(uint16_t pin, enum ttl_level level);

/**
 * Set the I2C address to which all read/write operations refer.
 */
bool port_noop_set_i2c_addr(int fd, uint16_t addr);

/**
 * Enable PEC (if supported).
 */
bool port_noop_enable_i2c_packet_error_checking(int fd);

/**
 * Set the interrupt handler for the given pin/edge_type.
 */
bool port_noop_set_interrupt_handler(uint16_t pin,
                                     enum edge_type,
                                     InterruptHandler);

#ifdef __cplusplus
}
#endif
