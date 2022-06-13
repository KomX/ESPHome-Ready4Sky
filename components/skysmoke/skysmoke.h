#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "../ready4sky/ready4sky.h"

#ifdef USE_ESP32

#include <string>
#include <array>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_defs.h>

namespace esphome {
namespace skysmoke {

namespace r4s = esphome::ready4sky;

struct SmokeState {
  uint8_t     type;
  uint8_t     temperature = 0xFF;
  uint8_t     battery_level = 0xFF;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     smoke = 0xFF;
};

class SkySmoke : public r4s::R4SDriver, public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;

    void set_smoke(binary_sensor::BinarySensor *smoke) { this->smoke_ = smoke; }
    void set_temperature(sensor::Sensor *temperature) { this->temperature_ = temperature; }
    void set_signal_strength(sensor::Sensor *signal_strength) { this->signal_strength_ = signal_strength; }
    void set_battery_level(sensor::Sensor *battery_level) { this->battery_level_ = battery_level; }

    bool is_active = false;
    SmokeState smoke_state;

  protected:
    void send_(uint8_t command);
    void parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) override;
    void device_online_() override;
    void device_offline_() override;
    void sync_data_() override;
    void verify_contig_() override;
    
    binary_sensor::BinarySensor *smoke_{nullptr};
    sensor::Sensor *temperature_{nullptr};
    sensor::Sensor *signal_strength_{nullptr};
    sensor::Sensor *battery_level_{nullptr};
    
};

}  // namespace skysmoke
}  // namespace esphome
#endif