#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
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

struct CState {
  uint8_t     red = 255;
  uint8_t     green = 255;
  uint8_t     blue = 255;
  uint8_t     brightness = 255;
};

struct CTState {
  uint8_t     tcold = 0;
  CState      ccold;
  CState      cwarm;
  CState      chot;
};


struct KettleState {
  uint8_t     type = 0;
  uint8_t     status = -1;
  uint8_t     temperature = 0;
  uint8_t     target = -1;
  uint8_t     numeric_target = -1;
  uint8_t     programm = -1;
  uint8_t     boil_time_correct = 0;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     color_timing;
  uint8_t     wait_command = 0;
  uint8_t     lock = -1;
  uint8_t     beeper = -1;
  uint32_t    energy;
  uint32_t    work_cycles;
  uint32_t    work_time;
  float       cup_quantity;
  uint16_t    water_volume;
  bool        power = false;
  bool        state_led = false;
  
  bool        full_init = false;
  bool        control_water = false;
  bool        raw_water = false;    // флаг сырой или пребывавшей более 6 часов без кипячения воды
  bool        old_water = false;    // флаг затхлой воды
  uint32_t    new_water_time = 0;   // время последнего обновления воды
  uint32_t    next_boil_time = 0;   // время, после которого необходимо кипятить воду
  uint8_t     last_temp = 0;        // последнее показание температуры с датчика
  uint32_t    last_time = 0;        // время последнего показания
  uint8_t     last_programm = 0;
  uint8_t     last_target = 0;
  uint8_t     off_line_temp = 0;    // температура при снятии с базы
  uint32_t    off_line_time = 0;    // время снятия с база
  CTState     work_light;
  CTState     night_light;
};

struct CupEngineState {
  uint8_t   algorithm = 0;
  uint8_t   phase = 0;
  uint8_t   temp_start_1 = 0;
  uint8_t   temp_stop_1 = 0;
  uint8_t   temp_stop_2 = 0;
  uint32_t  time_start_1 = 0;
  uint16_t  cup_volume = 0;
  float     cup_correct = 0.0;
  float     value_temp = 0.0;
  float     value_finale = 0.0;
};

struct IndicationState {
  uint8_t     time_period = 30;
  uint32_t    change_time = 0;
  std::string text_power    = "Off";    // power or lock status
  std::string text_prog     = "Boil";   // programm status
  std::string text_water    = "??:??";  // fresh water timing
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
    void set_state_led(switch_::Switch *state_led) { this->state_led_ = state_led; }
    void set_beeper(switch_::Switch *beeper) { this->beeper_ = beeper; }
    void set_back_light(light::LightOutput *back_light) { this->back_light_ = back_light; }
    
    void send_on();
    void send_off();
    void send_state_led(bool state);
    void send_beeper(bool state);
    void send_target_temp(uint8_t tt);
    void send_boil_time_adj(uint8_t bta);
    void send_light(uint8_t type);
    void on_off_light(bool state);
    void send_color_timing();
    
    bool            is_ready = false;
    KettleState     kettle_state;
    CupEngineState  cup_state;  // параметры механизма расчета чашек воды
    IndicationState indication;
    
    light::LightState *light_state{nullptr};
    
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
    
    light::LightOutput *back_light_{nullptr};
    
    text_sensor::TextSensor *status_ind_{nullptr};
    
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *state_led_ = {nullptr};
    switch_::Switch *beeper_ = {nullptr};

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

class SkyKettleBackgroundSwitch : public switch_::Switch {
  public:
    explicit SkyKettleBackgroundSwitch(SkyKettle *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->kettle_state.state_led) {
        this->parent_->send_state_led(state);
      }
    }
  protected:
    SkyKettle *parent_;
};

class SkyKettleBeeperSwitch : public switch_::Switch {
  public:
    explicit SkyKettleBeeperSwitch(SkyKettle *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->kettle_state.beeper != 0x00)) {
        this->parent_->send_beeper(state);
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
    }
  protected:
    SkyKettle *parent_;
};

class SkyKettleBoilTimeAdjNumber : public number::Number {
  public:
    void set_parent(SkyKettle *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->parent_->send_boil_time_adj((uint8_t)value);
    }
  protected:
    SkyKettle *parent_;
};

class SkyKettleBackgroundLight : public light::LightOutput {
  public:
    void set_parent(SkyKettle *parent) { this->parent_ = parent; }
    light::LightTraits get_traits() override {
      light::LightTraits traits{};
      traits.set_supported_color_modes({light::ColorMode::RGB});
      return traits;
    }
    void setup_state(light::LightState *state) override { this->parent_->light_state = state; }
    void write_state(light::LightState *state) override {
      if(this->parent_->send_data[22]) {
        this->parent_->send_data[22] = 0x00;
        return;
      }
      if(this->parent_->send_data[20] == 95)
        this->parent_->send_data[20] = 0;
      if(state->remote_values.is_on()) {
        CTState *wld;
        if(this->parent_->send_data[20] == 0) {
          if(this->parent_->kettle_state.programm != 0x03) {
            if(!this->parent_->kettle_state.power)
              this->parent_->on_off_light(true);
          }
          else if(this->parent_->kettle_state.full_init) {
            wld = &this->parent_->kettle_state.night_light;
            wld->ccold.red =
            wld->cwarm.red =
            wld->chot.red = (uint8_t)(state->current_values.get_red()*255);
            wld->ccold.green =
            wld->cwarm.green =
            wld->chot.green = (uint8_t)(state->current_values.get_green()*255);
            wld->ccold.blue =
            wld->cwarm.blue =
            wld->chot.blue = (uint8_t)(state->current_values.get_blue()*255);
            wld->ccold.brightness =
            wld->cwarm.brightness =
            wld->chot.brightness = (uint8_t)(state->current_values.get_brightness()*255);
            this->parent_->send_light(1);
          }
        }
        else if((this->parent_->send_data[20] > 0) && (this->parent_->send_data[20] < 9)) {
          if((this->parent_->send_data[20] > 0) && (this->parent_->send_data[20] < 5))
            wld = &this->parent_->kettle_state.work_light;
          else
            wld = &this->parent_->kettle_state.night_light;
          switch(this->parent_->send_data[20]) {
            case 0x01:
            case 0x05: {
              wld->ccold.red = (uint8_t)(state->current_values.get_red()*255);
              wld->ccold.green = (uint8_t)(state->current_values.get_green()*255);
              wld->ccold.blue = (uint8_t)(state->current_values.get_blue()*255);
              wld->ccold.brightness = (uint8_t)(state->current_values.get_brightness()*255);
              break;
            }
            case 0x02:
            case 0x06: {
              wld->cwarm.red = (uint8_t)(state->current_values.get_red()*255);
              wld->cwarm.green = (uint8_t)(state->current_values.get_green()*255);
              wld->cwarm.blue = (uint8_t)(state->current_values.get_blue()*255);
              wld->cwarm.brightness = (uint8_t)(state->current_values.get_brightness()*255);
              break;
            }
            case 0x03:
            case 0x07: {
              wld->chot.red = (uint8_t)(state->current_values.get_red()*255);
              wld->chot.green = (uint8_t)(state->current_values.get_green()*255);
              wld->chot.blue = (uint8_t)(state->current_values.get_blue()*255);
              wld->chot.brightness = (uint8_t)(state->current_values.get_brightness()*255);
              break;
            }
            case 0x04:
            case 0x08: {
              wld->tcold = (uint8_t)(state->current_values.get_brightness()*100);
              break;
            }
          }
        }
        else if(this->parent_->send_data[20] == 10) {
        
        }
        else if(this->parent_->send_data[20] == 11) {
        
        }
      }
      else {
        if(this->parent_->send_data[20] == 0) {
          if(this->parent_->kettle_state.full_init)
            this->parent_->on_off_light(false);
        }
        else {
          if((this->parent_->send_data[20] > 0) && (this->parent_->send_data[20] < 5)) {
            this->parent_->send_data[20] = 0;
            this->parent_->send_light(0);
          }
          else if(this->parent_->send_data[20] == 5) {
            this->parent_->send_data[20] = 0;
            this->parent_->send_light(1);
          }
          else if((this->parent_->send_data[20] > 5) && (this->parent_->send_data[20] < 9)) {
            this->parent_->send_data[20] = 0;
            this->parent_->send_light(2);
          }
          else if(this->parent_->send_data[20] == 9) {
            this->parent_->send_data[20] = 0;
            uint8_t ctg = ((uint8_t)(state->current_values.get_brightness() * 6)) + 1;
            this->parent_->send_data[3] = 30 * ((ctg < 7) ? ctg : (ctg-1));
            this->parent_->send_color_timing();
          }
          else if(this->parent_->send_data[20] == 10) {
          
          }
          else if(this->parent_->send_data[20] == 11) {
          
          }
        }    
      }
    }

  protected:
    SkyKettle         *parent_{nullptr};
};

}  // namespace skykettle
}  // namespace esphome
#endif