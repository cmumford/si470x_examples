#include <string.h>

#include <rds_decoder.h>
#include <si470x.h>
#include "oda_decode.h"
#include "port_espidf.h"
#include "rds_util.h"

namespace {

si470x* g_tuner;
rds_oda_data* g_oda_data;

}  // namespace

extern "C" void app_main() {
  g_oda_data = create_oda_data();

  const port port_config{
      .delay = port_delay,
      .enable_gpio = port_enable_gpio,
      .set_pin_mode = port_set_pin_mode,
      .digital_write = port_digital_write,
      .set_interrupt_handler = port_set_interrupt_handler,
      .set_i2c_addr = port_set_i2c_addr,
      .enable_i2c_packet_error_checking = port_enable_i2c_packet_error_checking,
      .i2c_write = port_i2c_write,
      .i2c_read = port_i2c_read,
  };
  const si470x_config config = {
      .region = REGION_US,
      .advanced_ps_decoding = true,
      .gpio2_int_pin = 5,  // GPIO5
      .reset_pin = 6,      // GPIO6
      .sdio_pin = 2,       // GPIO2
      .sclk_pin = 3,       // GPIO3
  };
  g_tuner = si470x_create(&config, &port_config);
}
