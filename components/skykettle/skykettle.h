#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "../ready4sky/ready4sky.h"

#ifdef USE_ESP32

#include <string>
#include <array>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_defs.h>

namespace esphome {
namespace skykettle {

namespace r4s = esphome::ready4sky;

struct KettleState {
  uint8_t     type = 0;
  uint8_t     status;
  uint8_t     temperature;
  uint8_t     target = -1;
  uint8_t     numeric_target = -1;
  uint8_t     programm = -1;
  uint8_t     boil_time = 0;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     wait_command = 0;
  uint32_t    energy;
  uint32_t    work_cycles;
  uint32_t    work_time;
  float       cup_quantity;
  uint16_t    water_volume;
  bool        power;
  bool        lock;
  bool        raw_water = false;
  uint32_t    new_water_time = 0;
  uint8_t     last_temp = 0;
  uint32_t    last_time = 0;
  uint8_t     off_line_temp = 0;
  uint32_t    off_line_time = 0;
};

struct CupEngineState {
  uint8_t     algorithm = 0;
  uint8_t     phase = 0;
  uint8_t     temp_start_1 = 0;
  uint8_t     temp_stop_1 = 0;
  uint8_t     temp_stop_2 = 0;
  uint32_t    time_start_1 = 0;
  uint16_t    cup_volume = 0;
  float       cup_correct = 0.0;
  float       value_temp = 0.0;
  float       value_finale = 0.0;
};

class SkyKettle : public r4s::R4SDriver, public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;

    void set_cup_volume(uint16_t cup_volume) { this->cup_state.cup_volume = cup_volume; }
    void set_cup_correct(float cup_correct) { this->cup_state.cup_correct = cup_correct; }
    void set_temperature(sensor::Sensor *temperature) { this->temperature_ = temperature; }
    void set_energy(sensor::Sensor *energy) { this->energy_ = energy; }
    void set_signal_strength(sensor::Sensor *signal_strength) { this->signal_strength_ = signal_strength; }
    void set_work_cycles(sensor::Sensor *work_cycles) { this->work_cycles_ = work_cycles; }
    void set_work_time(sensor::Sensor *work_time) { this->work_time_ = work_time; }
    void set_cup_quantity(sensor::Sensor *cup_quantity) { this->cup_quantity_ = cup_quantity; }
    void set_water_volume(sensor::Sensor *water_volume) { this->water_volume_ = water_volume; }
    void set_target(number::Number *target) { this->target_ = target; }
    void set_status_indicator(text_sensor::TextSensor *status_ind) { this->status_ind_ = status_ind; }
    void set_boil_time_adj(number::Number *boil_time_adj) { this->boil_time_adj_ = boil_time_adj; }
    void set_power(switch_::Switch *power) { this->power_ = power; }
    
    void send_on();
    void send_off();
    void send_target_temp(uint8_t tt);
    void send_boil_time_adj(uint8_t bta);
    
    bool is_ready = false;
    KettleState kettle_state;
    CupEngineState cup_state;         // параметры механизма расчета чашек воды

  protected:
    void send_(uint8_t command);
    void parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) override;
    void device_online_() override;
    void device_offline_() override;
    void sync_data_() override;
    void verify_contig_() override;
    void cup_engine_(uint8_t temp, uint32_t stamp);
    
    sensor::Sensor *temperature_{nullptr};
    sensor::Sensor *energy_{nullptr};
    sensor::Sensor *signal_strength_{nullptr};
    sensor::Sensor *work_cycles_{nullptr};
    sensor::Sensor *work_time_{nullptr};
    sensor::Sensor *cup_quantity_{nullptr};
    sensor::Sensor *water_volume_{nullptr};
    
    number::Number *target_{nullptr};
    number::Number *boil_time_adj_{nullptr};
    
    text_sensor::TextSensor *status_ind_{nullptr};
    
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *lock_ = {nullptr};

};

class SkyKettlePowerSwitch : public switch_::Switch {
  public:
    explicit SkyKettlePowerSwitch(SkyKettle *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->kettle_state.power) {
        if(state)
          this->parent_->send_on();
        else
          this->parent_->send_off();
      }
    }

  protected:
    SkyKettle *parent_;
};

class SkyKettleTargetNumber : public number::Number {
  public:
    void set_parent(SkyKettle *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->parent_->send_target_temp((uint8_t)value);
//      this->publish_state(value);
    }
  protected:
    SkyKettle *parent_;
};

class SkyKettleBoilTimeAdjNumber : public number::Number {
  public:
    void set_parent(SkyKettle *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->parent_->send_boil_time_adj((uint8_t)value);
//      this->publish_state(value);
    }
  protected:
    SkyKettle *parent_;
};


}  // namespace skykettle
}  // namespace esphome
#endif