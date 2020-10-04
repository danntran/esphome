#include "mitsubishi.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mitsubishi {

static const char *TAG = "mitsubishi.climate";

const uint32_t MITSUBISHI_OFF = 0x00;

const uint8_t MITSUBISHI_COOL = 0x18;
const uint8_t MITSUBISHI_DRY = 0x10;
const uint8_t MITSUBISHI_AUTO = 0x20;
const uint8_t MITSUBISHI_HEAT = 0x08;
const uint8_t MITSUBISHI_FAN_AUTO = 0x00;

// Pulse parameters in usec
const uint16_t MITSUBISHI_HEADER_MARK = 3400;
const uint16_t MITSUBISHI_HEADER_SPACE = 1750;
const uint16_t MITSUBISHI_BIT_MARK = 450;
const uint16_t MITSUBISHI_ONE_SPACE = 1300;
const uint16_t MITSUBISHI_ZERO_SPACE = 420;
const uint16_t MITSUBISHI_RPT_MARK = 440;
const uint16_t MITSUBISHI_RPT_SPACE = 17000;

void MitsubishiClimate::transmit_state() {
  uint32_t remote_state[18] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x48, 0x00, 0x30,
                               0x58, 0x61, 0x00, 0x00, 0x00, 0x10, 0x40, 0x00, 0x00};

  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      remote_state[6] = MITSUBISHI_COOL;
      break;
    case climate::CLIMATE_MODE_HEAT:
      remote_state[6] = MITSUBISHI_HEAT;
      break;
    case climate::CLIMATE_MODE_AUTO:
      remote_state[6] = MITSUBISHI_AUTO;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      remote_state[5] = MITSUBISHI_OFF;
      break;
  }

  remote_state[7] =
      (uint8_t) roundf(clamp(this->target_temperature, MITSUBISHI_TEMP_MIN, MITSUBISHI_TEMP_MAX) - MITSUBISHI_TEMP_MIN);

  ESP_LOGV(TAG, "Sending Mitsubishi target temp: %.1f state: %02X mode: %02X temp: %02X", this->target_temperature,
           remote_state[5], remote_state[6], remote_state[7]);

  // Checksum
  for (int i = 0; i < 17; i++) {
    remote_state[17] += remote_state[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();

  data->set_carrier_frequency(38000);
  // repeat twice
  for (uint16_t r = 0; r < 2; r++) {
    // Header
    data->item(MITSUBISHI_HEADER_MARK, MITSUBISHI_HEADER_SPACE);
    // Data
    for (uint8_t i : remote_state)
      for (uint8_t j = 0; j < 8; j++) {
        bool bit = i & (1 << j);
        if (bit)
          data->item(MITSUBISHI_BIT_MARK, MITSUBISHI_ONE_SPACE);
        else
          data->item(MITSUBISHI_BIT_MARK, MITSUBISHI_ZERO_SPACE);
      }
    // Footer
    if (r == 0)
      data->item(MITSUBISHI_RPT_MARK, MITSUBISHI_RPT_SPACE);
      // data->mark(MITSUBISHI_RPT_MARK);
      // data->space(MITSUBISHI_RPT_SPACE);
    else
      data->mark(MITSUBISHI_RPT_MARK);
  }
  // data->mark(MITSUBISHI_RPT_MARK);

  transmit.perform();
}

}  // namespace mitsubishi
}  // namespace esphome
