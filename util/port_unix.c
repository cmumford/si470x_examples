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
#if defined(HAVE_NANOSLEEP)
#include <time.h>
#elif defined(HAVE_USLEEP)
#include <unistd.h>
#endif

#if defined(HAVE_I2C_DEV)
#include <linux/i2c-dev.h>
#endif
#if defined(HAVE_WIRING_PI)
#include <wiringPi.h>
#endif

#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

bool port_supports_gpio() {
#if defined(HAVE_WIRING_PI) || defined(HAVE_I2C_DEV)
  return true;
#else
  return false;
#endif
}

bool port_supports_i2c() {
#if defined(HAVE_WIRING_PI) || defined(HAVE_I2C_DEV)
  return true;
#else
  return false;
#endif
}

#if defined(HAVE_WIRING_PI)

static int xlate_pin_mode(enum pin_mode mode) {
  switch (mode) {
    case PIN_MODE_INPUT:
      return INPUT;
    case PIN_MODE_OUTPUT:
      return OUTPUT;
  }
  return INPUT;
}

static int xlate_ttl_level(enum ttl_level level) {
  switch (level) {
    case TTL_HIGH:
      return HIGH;
    case TTL_LOW:
      return LOW;
  }
  return HIGH;
}

static int xlate_edge_type(enum edge_type type) {
  switch (type) {
    case EDGE_TYPE_FALLING:
      return INT_EDGE_FALLING;
    case EDGE_TYPE_RISING:
      return INT_EDGE_RISING;
    case EDGE_TYPE_BOTH:
      return INT_EDGE_BOTH;
    case EDGE_TYPE_SETUP:
      return INT_EDGE_SETUP;
  }
  return INT_EDGE_FALLING;
}

#endif  // defined(HAVE_WIRING_PI)

void port_delay(uint16_t msec) {
  // usleep isn't always available on RPi, but delay is. Checking for
  // __arm__ is a poor way of detecting RPi.
#if defined(HAVE_WIRING_PI)
  delay(msec);
#elif defined(HAVE_NANOSLEEP)
  UNUSED(msec);
#if 0
  const struct timespec spec = {.tv_sec = msec / 1000,
                                .tv_nsec = 1000000 * (msec % 1000)};
  nanosleep(&spec, NULL);
#endif
#elif defined(HAVE_USLEEP)
  usleep(msec * 1000);
#else
#error "Don't have a delay function."
#endif
}

bool port_enable_gpio() {
#if defined(HAVE_WIRING_PI)
  wiringPiSetupGpio();
#endif
  return true;
}

void port_set_pin_mode(uint16_t pin, enum pin_mode mode) {
#if defined(HAVE_WIRING_PI)
  pinMode(pin, xlate_pin_mode(mode));
#else
  UNUSED(pin);
  UNUSED(mode);
#endif
}

void port_digital_write(uint16_t pin, enum ttl_level level) {
#if defined(HAVE_WIRING_PI)
  digitalWrite(pin, xlate_ttl_level(level));
#else
  UNUSED(pin);
  UNUSED(level);
#endif
}

bool port_set_i2c_addr(int i2c_fd, uint16_t i2c_addr) {
#if defined(HAVE_I2C_DEV)
  return ioctl(i2c_fd, I2C_SLAVE, i2c_addr) >= 0;
#else
  UNUSED(i2c_fd);
  UNUSED(i2c_addr);
  return true;
#endif
}

bool port_enable_i2c_packet_error_checking(int i2c_fd) {
#if defined(HAVE_I2C_DEV)
  return ioctl(i2c_fd, I2C_PEC, 1) >= 0;
#else
  UNUSED(i2c_fd);
  return true;
#endif
}

bool port_set_interrupt_handler(uint16_t pin,
                                enum edge_type edge_type,
                                InterruptHandler handler) {
#if defined(HAVE_WIRING_PI)
  return wiringPiISR(pin, xlate_edge_type(edge_type), handler);
#else
  UNUSED(pin);
  UNUSED(edge_type);
  UNUSED(handler);
  return true;
#endif
}
