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
namespace skyplug {

namespace r4s = esphome::ready4sky;

struct PlugState {
  uint8_t     type = 0;
  uint8_t     status = 0xFF;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     wait_command = 0;
  uint8_t     lock = 0xFF;
  uint8_t     remember = 0xFF;
  uint32_t    work_cycles;
  uint32_t    work_time;
};

class SkyPlug : public r4s::R4SDriver, public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;
    
    void set_signal_strength(sensor::Sensor *signal_strength) { this->signal_strength_ = signal_strength; }
    void set_work_cycles(sensor::Sensor *work_cycles) { this->work_cycles_ = work_cycles; }
    void set_work_time(sensor::Sensor *work_time) { this->work_time_ = work_time; }
    void set_power(switch_::Switch *power) { this->power_ = power; }
    void set_lock(switch_::Switch *lock) { this->lock_ = lock; }
    void set_remember(switch_::Switch *remember) { this->remember_ = remember; }
    
    void send_power(bool state);
    void send_lock(bool state);
    void send_remember(bool state);

    bool            is_ready = false;
    PlugState       plug_state;

  protected:
    void send_(uint8_t command);
    void parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) override;
    void device_online_() override;
    void device_offline_() override;
    void sync_data_() override;
    void verify_contig_() override;

    sensor::Sensor *signal_strength_{nullptr};
    sensor::Sensor *work_cycles_{nullptr};
    sensor::Sensor *work_time_{nullptr};
    
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *lock_;
    switch_::Switch *remember_;
};

class SkyPlugPowerSwitch : public switch_::Switch {
  public:
    explicit SkyPlugPowerSwitch(SkyPlug *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->plug_state.status) {
        this->parent_->send_power(state);
      }
    }
  protected:
    SkyPlug *parent_;
};

class SkyPlugLockSwitch : public switch_::Switch {
  public:
    explicit SkyPlugLockSwitch(SkyPlug *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->plug_state.lock != 0x00)) {
        this->parent_->send_lock(state);
      }
    }
  protected:
    SkyPlug *parent_;
};

class SkyPlugRememberSwitch : public switch_::Switch {
  public:
    explicit SkyPlugRememberSwitch(SkyPlug *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->plug_state.remember != 0x00)) {
        this->parent_->send_remember(state);
      }
    }
  protected:
    SkyPlug *parent_;
};

}  // namespace skyplug
}  // namespace esphome
#endif