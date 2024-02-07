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
namespace skycoffee {

namespace r4s = esphome::ready4sky;

struct CoffeeState {
  uint8_t     type = 0;
  uint8_t     status = -1;
  uint8_t     strength = -1;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     buzzer = -1;
  uint8_t     lock = -1;
  uint8_t     time_min = 0xFF;
  uint32_t    work_cycles;
  uint32_t    work_time;
  uint32_t    energy;
  uint8_t     wait_command = 0;
};

class SkyCoffee : public r4s::R4SDriver, public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;
    
    void set_signal_strength(sensor::Sensor *signal_strength) { this->signal_strength_ = signal_strength; }
    void set_work_cycles(sensor::Sensor *work_cycles) { this->work_cycles_ = work_cycles; }
    void set_work_time(sensor::Sensor *work_time) { this->work_time_ = work_time; }
    void set_energy(sensor::Sensor *energy) { this->energy_ = energy; }
    void set_power(switch_::Switch *power) { this->power_ = power; }
    void set_buzzer(switch_::Switch *buzzer) { this->buzzer_ = buzzer; }
    void set_lock(switch_::Switch *lock) { this->lock_ = lock; }
    void set_strength(switch_::Switch *strength) { this->strength_ = strength; }
    
    void send_power(bool state);
    void send_buzzer(bool state);
    void send_lock(bool state);
    void send_strength(bool state);

    bool            is_ready = false;
    CoffeeState       coffee_state;

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
    sensor::Sensor *energy_{nullptr};
    
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *buzzer_ = {nullptr};
    switch_::Switch *lock_ = {nullptr};
    switch_::Switch *strength_ = {nullptr};
};

class SkyCoffeePowerSwitch : public switch_::Switch {
  public:
    explicit SkyCoffeePowerSwitch(SkyCoffee *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->coffee_state.status != 0x00)) {
        this->parent_->send_power(state);
      }
    }
  protected:
    SkyCoffee *parent_;
};

class SkyCoffeeBuzzerSwitch : public switch_::Switch {
  public:
    explicit SkyCoffeeBuzzerSwitch(SkyCoffee *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->coffee_state.buzzer != 0x00)) {
        this->parent_->send_buzzer(state);
      }
    }
  protected:
    SkyCoffee *parent_;
};

class SkyCoffeeLockSwitch : public switch_::Switch {
  public:
    explicit SkyCoffeeLockSwitch(SkyCoffee *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->coffee_state.lock != 0x00)) {
        this->parent_->send_lock(state);
      }
    }
  protected:
    SkyCoffee *parent_;
};

class SkyCoffeeStrengthSwitch : public switch_::Switch {
  public:
    explicit SkyCoffeeStrengthSwitch(SkyCoffee *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->coffee_state.strength != 0x00)) {
        this->parent_->send_strength(state);
      }
    }
  protected:
    SkyCoffee *parent_;
};

}  // namespace skycoffee
}  // namespace esphome
#endif