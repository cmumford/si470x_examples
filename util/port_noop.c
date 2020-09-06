// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

/**
 * @file
 *
 * Abstraction implementation for Testing.
 */

#include "port_unix.h"

#include <sys/ioctl.h>
#include <unistd.h>

#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

void port_noop_delay(uint16_t msec) {
  UNUSED(msec);
}

bool port_noop_enable_gpio() {
  return true;
}

void port_noop_set_pin_mode(uint16_t pin, enum pin_mode mode) {
  UNUSED(pin);
  UNUSED(mode);
}

void port_noop_digital_write(uint16_t pin, enum ttl_level level) {
  UNUSED(pin);
  UNUSED(level);
}

bool port_noop_set_i2c_addr(int i2c_fd, uint16_t i2c_addr) {
  UNUSED(i2c_fd);
  UNUSED(i2c_addr);
  return true;
}

bool port_noop_enable_i2c_packet_error_checking(int i2c_fd) {
  UNUSED(i2c_fd);
  return true;
}

bool port_noop_set_interrupt_handler(uint16_t pin,
                                     enum edge_type edge_type,
                                     InterruptHandler handler) {
  UNUSED(pin);
  UNUSED(edge_type);
  UNUSED(handler);
  return true;
}
