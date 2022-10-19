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
namespace skyheat {

namespace r4s = esphome::ready4sky;

struct HeatState {
  uint8_t     type = 0;
  uint8_t     status = -1;
  uint8_t     temperature = -1;
  uint8_t     target_power = -1;
  uint8_t     target_temperature = -1;
  uint8_t     programm = -1;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     wait_command = 0;
  uint8_t     remember = -1;
  uint8_t     lock = -1;
  uint32_t    work_cycles;
  uint32_t    work_time;
};

class SkyHeat : public r4s::R4SDriver, public Component {
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
    void set_target_power(number::Number *target) { this->target_power_ = target; }
    void set_target_temperature(number::Number *target) { this->target_temperature_ = target; }
    
    void send_power(bool state);
    void send_lock(bool state);
    void send_remember(bool state);
    void send_target_power(uint8_t tt);
    void send_target_temperature(uint8_t tt);
    
    bool            is_ready = false;
    HeatState       heat_state;
    
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
    switch_::Switch *lock_ = {nullptr};
    switch_::Switch *remember_ = {nullptr};
    
    number::Number *target_power_{nullptr};
    number::Number *target_temperature_{nullptr};
};

class SkyHeatPowerSwitch : public switch_::Switch {
  public:
    explicit SkyHeatPowerSwitch(SkyHeat *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->heat_state.status) {
        this->parent_->send_power(state);
      }
    }
  protected:
    SkyHeat *parent_;
};

class SkyHeatLockSwitch : public switch_::Switch {
  public:
    explicit SkyHeatLockSwitch(SkyHeat *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->heat_state.lock != 0x00)) {
        this->parent_->send_lock(state);
      }
    }
  protected:
    SkyHeat *parent_;
};

class SkyHeatRememberSwitch : public switch_::Switch {
  public:
    explicit SkyHeatRememberSwitch(SkyHeat *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->heat_state.remember != 0x00)) {
        this->parent_->send_remember(state);
      }
    }
  protected:
    SkyHeat *parent_;
};

class SkyHeatTargetPowerNumber : public number::Number {
  public:
    void set_parent(SkyHeat *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->publish_state(value);
      if((uint8_t)value != this->parent_->heat_state.target_power)
        this->parent_->send_target_power((uint8_t)value);
    }
  protected:
    SkyHeat *parent_;
};

}  // namespace skyheat
}  // namespace esphome
#endif