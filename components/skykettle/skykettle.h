#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/switch/switch.h"
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
  uint8_t     type;
  uint8_t     status;
  uint8_t     temperature;
  uint8_t     target;
  uint8_t     numeric_target;
  uint8_t     programm;
  uint32_t    energy;
  uint32_t    work_cycles;
  uint32_t    work_time;
  float       cup_quantity;
  uint16_t    water_volume;
  float       version;
  bool        power;
  bool        lock;
};

struct CupEngineState {
  uint8_t     algorithm = 0;
  uint8_t     phase = 0;
  uint8_t     temp_last = 0;
  uint8_t     temp_start_1 = 0;
  uint8_t     temp_stop_1 = 0;
  uint8_t     temp_stop_2 = 0;
  uint32_t    time_last = 0;
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

    void parse_response(uint8_t *data, int8_t data_len, uint32_t timestamp) override;
    void device_online() override;
    void device_offline() override;
    void sync_data() override;

    void send(uint8_t command);
    void cup_engine(uint8_t temp, uint32_t stamp);
    
    void set_cup_volume(uint16_t cup_volume) { this->cup_state.cup_volume = cup_volume; }
    void set_cup_correct(float cup_correct) { this->cup_state.cup_correct = cup_correct; }
    
    void set_temperature(sensor::Sensor *temperature) { this->temperature_ = temperature; }
    void set_energy(sensor::Sensor *energy) { this->energy_ = energy; }
    void set_signal_strength(sensor::Sensor *signal_strength) { this->signal_strength_ = signal_strength; }
    void set_work_cycles(sensor::Sensor *work_cycles) { this->work_cycles_ = work_cycles; }
    void set_work_time(sensor::Sensor *work_time) { this->work_time_ = work_time; }
    void set_cup_quantity(sensor::Sensor *cup_quantity) { this->cup_quantity_ = cup_quantity; }
    void set_water_volume(sensor::Sensor *water_volume) { this->water_volume_ = water_volume; }
    
    void set_power(switch_::Switch *power) { this->power_ = power; }
    
    bool is_active = false;
    KettleState kettle_state;
    CupEngineState  cup_state;         // параметры механизма расчета чашек воды
    
  protected:
    sensor::Sensor *temperature_{nullptr};
    sensor::Sensor *energy_{nullptr};
    sensor::Sensor *signal_strength_{nullptr};
    sensor::Sensor *work_cycles_{nullptr};
    sensor::Sensor *work_time_{nullptr};
    sensor::Sensor *cup_quantity_{nullptr};
    sensor::Sensor *water_volume_{nullptr};
    
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *lock_ = {nullptr};

};

class SkyKettlePowerSwitch : public switch_::Switch {
  public:
    explicit SkyKettlePowerSwitch(SkyKettle *parent): parent_(parent) {}

    void write_state(bool state) override {
      if(state != this->parent_->kettle_state.power) {
        if(state)
          this->parent_->send(0x03);
        else
          this->parent_->send(0x04);
      }
    }

  protected:
    SkyKettle *parent_;
};

}  // namespace skykettle
}  // namespace esphome
#endif