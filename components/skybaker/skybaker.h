#pragma once

#include "esphome/core/component.h"
//#include "esphome/components/sensor/sensor.h"
#include "esphome/core/helpers.h"
#include "../ready4sky/ready4sky.h"

#ifdef USE_ESP32

#include <string>
#include <array>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_defs.h>

namespace esphome {
namespace skybaker {

namespace r4s = esphome::ready4sky;

struct BakerState {
  uint8_t     type = 0;
  uint8_t     status = 0xFF;
  uint8_t     mode = 0xFF;
  uint8_t     hours = 0xFF;
  uint8_t     minutes = 0xFF;
  uint8_t     baker_minutes = 10;
  uint8_t     delay_hours = 2;
  uint8_t     delay_minutes = 0;
  uint8_t     heat_hours = 0;
  uint8_t     heat_minutes = 10;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     wait_command = 0;
  uint8_t     language = 1;
  uint32_t    last_time = 0;
  bool        power = false;
  bool        preheat = false;
  bool        postheat = false;
  bool        delay = false;
  bool        update_timer = true;
};

struct IndicationState {
  bool        update         = false;
  std::string text_power     = "Выкл.";
  std::string text_condition = "Нет связи";
  std::string text_time      = "??:??";
};

class SkyBaker : public Component, public ready4sky::R4SDriver {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;
    
    void set_signal_strength(sensor::Sensor *value) { this->signal_strength_ = value; }
    void set_status_indicator(text_sensor::TextSensor *value) { this->status_ind_ = value; }
    void set_power(switch_::Switch *value) { this->power_ = value; }
    void set_postheat(switch_::Switch *value) { this->postheat_ = value; }
    void set_delayed_start(switch_::Switch *value) { this->delay_ = value; }
    void set_mode(select::Select *value) { this->mode_ = value; }
    void set_timer_hours(number::Number *value) { this->timer_hours_ = value; }
    void set_timer_minutes(number::Number *value) { this->timer_minutes_ = value; }
    void set_language(uint8_t value) { this->baker_state.language = value; }
    
    void send_power(bool state);
    void send_setting(uint8_t mode, bool heat);
    void timer_corrector(uint8_t hour, uint8_t minute);

    bool            is_ready = false;
    BakerState      baker_state;
    IndicationState indication;

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
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *postheat_ = {nullptr};
    switch_::Switch *delay_ = {nullptr};
    number::Number *timer_hours_{nullptr};
    number::Number *timer_minutes_{nullptr};

};

class SkyBakerPowerSwitch : public switch_::Switch {
  public:
    explicit SkyBakerPowerSwitch(SkyBaker *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->baker_state.power) {
        this->parent_->send_power(state);
      }
    }
  protected:
    SkyBaker *parent_;
};

class SkyBakerPostHeatSwitch : public switch_::Switch {
  public:
    explicit SkyBakerPostHeatSwitch(SkyBaker *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->baker_state.postheat) {
        this->parent_->send_setting(this->parent_->baker_state.mode, state);
      }
    }
  protected:
    SkyBaker *parent_;
};

class SkyBakerDelaySwitch : public switch_::Switch {
  public:
    explicit SkyBakerDelaySwitch(SkyBaker *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->baker_state.delay) {
        this->parent_->baker_state.delay = state;
        this->publish_state(state);
        this->parent_->baker_state.update_timer = true;
        this->parent_->send_setting(this->parent_->baker_state.mode, this->parent_->baker_state.postheat);
      }
    }
  protected:
    SkyBaker *parent_;
};

class SkyBakerModeSelect : public select::Select {
 public:
  explicit SkyBakerModeSelect(SkyBaker *parent) : parent_(parent) {}
  void control(const std::string &state) override {
    auto index = this->index_of(state);
    if(index.value() != this->parent_->baker_state.mode) {
      this->parent_->baker_state.update_timer = true;
      this->parent_->send_setting(index.value(), this->parent_->baker_state.postheat);
    }
    else {
      this->publish_state(state);
      ESP_LOGD("", "Selected Mode: %s (%d) ", state.c_str(), index.value());
    }
  }

 protected:
  SkyBaker *parent_;
};

class SkyBakerTimerHoursNumber : public number::Number {
  public:
    void set_parent(SkyBaker *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->publish_state(value);
      if((uint8_t)value != this->parent_->baker_state.hours)
        this->parent_->timer_corrector((uint8_t)value, 0xFF);
    }
  protected:
    SkyBaker *parent_;
};

class SkyBakerTimerMinutesNumber : public number::Number {
  public:
    void set_parent(SkyBaker *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->publish_state(value);
      if((uint8_t)value != this->parent_->baker_state.minutes)
        this->parent_->timer_corrector(0xFF, (uint8_t)value);
    }
  protected:
    SkyBaker *parent_;
};

}  // namespace skybaker
}  // namespace esphome
#endif