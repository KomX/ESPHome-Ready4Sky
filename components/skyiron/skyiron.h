#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "../ready4sky/ready4sky.h"

#ifdef USE_ESP32

#include <string>
#include <array>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_defs.h>

namespace esphome {
namespace skyiron {

namespace r4s = esphome::ready4sky;

struct IronState {
  uint8_t     type = 0;
  uint8_t     status = -1;
  uint8_t     position = -1;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     wait_command = 0;
  uint8_t     safe_mode = 0;
  int8_t      counter = 0;
  bool        power = false;
};

struct IndicIronState {
  std::string text_power    = "Off";     // power or lock status
  std::string text_position = "Stand";   // position status
  bool        update = false;
};

class SkyIron : public r4s::R4SDriver, public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;
    
    void set_signal_strength(sensor::Sensor *signal_strength) { this->signal_strength_ = signal_strength; }
    void set_status_indicator(text_sensor::TextSensor *status_ind) { this->status_ind_ = status_ind; }
    void set_power(switch_::Switch *power) { this->power_ = power; }
    void set_safe_mode(switch_::Switch *safe_mode) { this->safe_mode_ = safe_mode; }
    
    void send_off();
    void send_safe_mode(bool state);

    bool            is_ready = false;
    IronState       iron_state;
    IndicIronState  indication;

  protected:
    void send_(uint8_t command);
    void parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) override;
    void device_online_() override;
    void device_offline_() override;
    void sync_data_() override;
    void verify_contig_() override;

    sensor::Sensor *signal_strength_{nullptr};
    
    text_sensor::TextSensor *status_ind_{nullptr};
    
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *safe_mode_ = {nullptr};
};

class SkyIronPowerSwitch : public switch_::Switch {
  public:
    explicit SkyIronPowerSwitch(SkyIron *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->iron_state.power) {
        if(!state)
          this->parent_->send_off();
      }
    }
  protected:
    SkyIron *parent_;
};

class SkyIronSafeModeSwitch : public switch_::Switch {
  public:
    explicit SkyIronSafeModeSwitch(SkyIron *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->iron_state.safe_mode != 0x00)) {
        this->parent_->send_safe_mode(state);
      }
    }
  protected:
    SkyIron *parent_;
};

}  // namespace skyiron
}  // namespace esphome
#endif