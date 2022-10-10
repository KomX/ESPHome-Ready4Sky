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
namespace skycooker {

namespace r4s = esphome::ready4sky;

struct CookerState {
  uint8_t     type = 0;
  uint8_t     status[2] = {0xFF, 0xFF};
  uint8_t     mode[2] = {0xFF, 0xFF};
  uint8_t     submode[2] = {0xFF, 0xFF};
  uint8_t     temperature[2] = {0xFF, 0xFF};
  uint8_t     hours[2] = {0xFF, 0xFF};
  uint8_t     minutes[2] = {0xFF, 0xFF};
  int8_t      wait_hours[2] = {-1, -1};
  int8_t      wait_minutes[2] = {-1, -1};
  uint8_t     heat[2] = {0xFF, 0xFF};
  uint8_t     bowl_num = 0;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     wait_command = 0;
  uint8_t     lock = -1;
  uint8_t     language = 1;
  bool        power = false;
  bool        postheat = false;
  bool        timer_mode = false;
  uint8_t     viz_mode[2] = {0xFF, 0xFF};
  uint8_t     last_mode;
};

struct IndicationState {
  bool        update         = false;
  std::string text_power     = "Выкл.";
  std::string text_condition = "Нет связи";
  std::string text_time      = "??:??";
};

class SkyCooker : public r4s::R4SDriver, public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;
    
    void set_signal_strength(sensor::Sensor *value) { this->signal_strength_ = value; }
    void set_status_indicator(text_sensor::TextSensor *value) { this->status_ind_ = value; }
    void set_power(switch_::Switch *value) { this->power_ = value; }
    void set_postheat(switch_::Switch *value) { this->postheat_ = value; }
    void set_timer_mode(switch_::Switch *value) { this->timer_mode_ = value; }
    void set_mode(select::Select *value) { this->mode_ = value; }
    void set_mode_data(uint8_t index, uint8_t targ_temp, uint8_t hors, uint8_t minutes, uint8_t en_modes);
    void set_submode(select::Select *value) { this->submode_ = value; }
    void set_submode_data(uint8_t index, uint8_t mode, uint8_t time1, uint8_t time2, uint8_t time3, uint8_t time4);
    void set_temperature(number::Number *value) { this->temperature_ = value; }
    void set_timer_hours(number::Number *value) { this->timer_hours_ = value; }
    void set_timer_minutes(number::Number *value) { this->timer_minutes_ = value; }
    void set_language(uint8_t value) { this->cooker_state.language = value; }
    
    void send_power(bool state);
    void send_postheat(bool state);
    void send_timer_mode(bool state);
    void send_mode(uint8_t idx);
    void send_submode(uint8_t idx);
    void send_temperature(uint8_t tt);
    void send_cooking_time(uint8_t th, uint8_t tm);
    void send_delay_time(uint8_t wth, uint8_t wtm, uint8_t th, uint8_t tm);
    

    bool            is_ready = false;
    CookerState     cooker_state;
    IndicationState indication;
    uint8_t         data_temp[24];
    uint8_t         data_hours[24];
    uint8_t         data_mins[24];
    uint8_t         data_flags[24];
    uint8_t         data_product[8][5];

  protected:
    void send_(uint8_t command);
    void parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) override;
    void device_online_() override;
    void device_offline_() override;
    void sync_data_() override;
    void verify_contig_() override;

    sensor::Sensor *signal_strength_{nullptr};
    text_sensor::TextSensor *status_ind_{nullptr};
    select::Select *mode_ = {nullptr};
    select::Select *submode_ = {nullptr};
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *postheat_ = {nullptr};
    switch_::Switch *timer_mode_ = {nullptr};
    number::Number *temperature_{nullptr};
    number::Number *timer_hours_{nullptr};
    number::Number *timer_minutes_{nullptr};
};

class SkyCookerPowerSwitch : public switch_::Switch {
  public:
    explicit SkyCookerPowerSwitch(SkyCooker *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->cooker_state.power) {
        this->parent_->send_power(state);
      }
    }
  protected:
    SkyCooker *parent_;
};

class SkyCookerPostHeatSwitch : public switch_::Switch {
  public:
    explicit SkyCookerPostHeatSwitch(SkyCooker *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->cooker_state.postheat) {
        this->parent_->send_postheat(state);
      }
    }
  protected:
    SkyCooker *parent_;
};

class SkyCookerTimerModeSwitch : public switch_::Switch {
  public:
    explicit SkyCookerTimerModeSwitch(SkyCooker *parent): parent_(parent) {}
    void write_state(bool state) override {
      this->parent_->send_timer_mode(state);
    }
  protected:
    SkyCooker *parent_;
};

class SkyCookerModeSelect : public select::Select {
 public:
  explicit SkyCookerModeSelect(SkyCooker *parent) : parent_(parent) {}
  void control(const std::string &state) override {
    this->publish_state(state);
    auto index = this->index_of(state);
    if(index.value() != this->parent_->cooker_state.viz_mode[this->parent_->cooker_state.bowl_num])
      this->parent_->send_mode(index.value());
    ESP_LOGD("", "Selected Mode: %s (%d) t[%d]h[%d]m[%d]", state.c_str(), index.value(), 
          this->parent_->data_temp[index.value()],
          this->parent_->data_hours[index.value()],
          this->parent_->data_mins[index.value()]
          );
  }

 protected:
  SkyCooker *parent_;
};

class SkyCookerSubModeSelect : public select::Select {
 public:
  explicit SkyCookerSubModeSelect(SkyCooker *parent) : parent_(parent) {}
  void control(const std::string &state) override {
    this->publish_state(state);
    auto index = this->index_of(state);
    if(index.value() != this->parent_->cooker_state.submode[this->parent_->cooker_state.bowl_num]) 
      this->parent_->send_submode(index.value());
    ESP_LOGD("", "Selected Submode: %s (%d)", state.c_str(), index.value());
  }

 protected:
  SkyCooker *parent_;
};

class SkyCookerTemperatureNumber : public number::Number {
  public:
    void set_parent(SkyCooker *parent) { this->parent_ = parent; }
    void control(float value) override {
      if((uint8_t)value != this->parent_->cooker_state.temperature[this->parent_->cooker_state.bowl_num]) {
        this->publish_state(value);
        this->parent_->send_temperature((uint8_t)value);
      }
    }
  protected:
    SkyCooker *parent_;
};

class SkyCookerTimerHoursNumber : public number::Number {
  public:
    void set_parent(SkyCooker *parent) { this->parent_ = parent; }
    void control(float value) override {
      if(((uint8_t)value != this->parent_->cooker_state.hours[this->parent_->cooker_state.bowl_num])
            && !this->parent_->cooker_state.timer_mode){
        this->publish_state(value);
        this->parent_->send_cooking_time( (uint8_t)value, 
                                          this->parent_->cooker_state.minutes[this->parent_->cooker_state.bowl_num]
                                        );
      }
      if(((uint8_t)value != this->parent_->cooker_state.wait_hours[this->parent_->cooker_state.bowl_num])
            && this->parent_->cooker_state.timer_mode){
        this->publish_state(value);
        this->parent_->send_delay_time( (uint8_t)value, 
                                        this->parent_->cooker_state.wait_minutes[this->parent_->cooker_state.bowl_num],
                                        this->parent_->cooker_state.hours[this->parent_->cooker_state.bowl_num],
                                        this->parent_->cooker_state.minutes[this->parent_->cooker_state.bowl_num]
                                      );
      }
    }
  protected:
    SkyCooker *parent_;
};

class SkyCookerTimerMinutesNumber : public number::Number {
  public:
    void set_parent(SkyCooker *parent) { this->parent_ = parent; }
    void control(float value) override {
      if(((uint8_t)value != this->parent_->cooker_state.minutes[this->parent_->cooker_state.bowl_num])
            && !this->parent_->cooker_state.timer_mode){
        this->publish_state(value);
        this->parent_->send_cooking_time( this->parent_->cooker_state.hours[this->parent_->cooker_state.bowl_num],
                                          (uint8_t)value
                                        );
      }
      if(((uint8_t)value != this->parent_->cooker_state.wait_minutes[this->parent_->cooker_state.bowl_num])
            && this->parent_->cooker_state.timer_mode){
        this->publish_state(value);
        this->parent_->send_delay_time( this->parent_->cooker_state.wait_hours[this->parent_->cooker_state.bowl_num],
                                        (uint8_t)value,
                                        this->parent_->cooker_state.hours[this->parent_->cooker_state.bowl_num],
                                        this->parent_->cooker_state.minutes[this->parent_->cooker_state.bowl_num]
                                      );
      }
    }
  protected:
    SkyCooker *parent_;
};


}  // namespace skycooker
}  // namespace esphome
#endif