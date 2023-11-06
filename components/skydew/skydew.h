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
namespace skydew {

namespace r4s = esphome::ready4sky;

struct DewState {
  uint8_t     type = 0;
  uint8_t     status = -1;
  uint8_t     humidity = -1;
  uint8_t     temperature = -1;
  uint8_t     target_humidity = -1;
  uint8_t     mode = -1;
  uint8_t     night_mode = -1;
  uint8_t     steam_level = -1;
  uint8_t     alarm = -1;
  uint8_t     beeper = -1;
  uint8_t     warm_steam = -1;
  uint8_t     version;
  uint8_t     relise;
  uint8_t     hours = -1;
  uint8_t     minutes = -1;
  uint8_t     time_min = -1;
  uint8_t     wait_command = 0;
  uint8_t     language = 1;
};

struct IndicationState {
  bool        update         = false;
  std::string text_power     = "Выкл.";
  std::string text_condition = "";
  std::string text_time      = "??:??";
};

class SkyDew : public r4s::R4SDriver, public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;
    
    void set_temperature(sensor::Sensor *value) { this->temperature_ = value; }
    void set_humidity(sensor::Sensor *value) { this->humidity_ = value; }
    void set_signal_strength(sensor::Sensor *value) { this->signal_strength_ = value; }
    void set_work_cycles(sensor::Sensor *value) { this->work_cycles_ = value; }
    void set_work_time(sensor::Sensor *value) { this->work_time_ = value; }
    void set_energy(sensor::Sensor *value) { this->energy_ = value; }
    void set_language(uint8_t value) { this->dew_state.language = value; }
    void set_power(switch_::Switch *value) { this->power_ = value; }
    void set_auto_mode(switch_::Switch *value) { this->auto_mode_ = value; }
    void set_night_mode(switch_::Switch *value) { this->night_mode_ = value; }
    void set_beeper(switch_::Switch *value) { this->beeper_ = value; }
    void set_warm_steam(switch_::Switch *value) { this->warm_steam_ = value; }
    void set_mode(select::Select *value) { this->mode_ = value; }
    
    void set_target_humidity(number::Number *value) { this->target_humidity_ = value; }
    void set_steam_level(number::Number *value) { this->steam_level_ = value; }
    
    void set_status_indicator(text_sensor::TextSensor *value) { this->status_ind_ = value; }
    
    void send_power(bool state);
    void send_auto_mode(bool state);
    void send_night_mode(bool state);
    void send_beeper(bool state);
    void send_warm_steam(bool state);
    void send_target_humidity(uint8_t value);
    void send_steam_level(uint8_t value);
    void send_mode(uint8_t value);
    
    bool            is_ready = false;
    DewState        dew_state;
    IndicationState indication;
    
  protected:
    void send_(uint8_t command);
    void parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) override;
    void device_online_() override;
    void device_offline_() override;
    void sync_data_() override;
    void verify_contig_() override;
    
    sensor::Sensor *signal_strength_{nullptr};
    sensor::Sensor *temperature_{nullptr};
    sensor::Sensor *humidity_{nullptr};
    sensor::Sensor *work_time_{nullptr};
    sensor::Sensor *energy_{nullptr};
    sensor::Sensor *work_cycles_{nullptr};
    
    text_sensor::TextSensor *status_ind_{nullptr};
    
    switch_::Switch *power_ = {nullptr};
    switch_::Switch *auto_mode_ = {nullptr};
    switch_::Switch *night_mode_ = {nullptr};
    switch_::Switch *beeper_ = {nullptr};
    switch_::Switch *warm_steam_ = {nullptr};
    
    number::Number *steam_level_{nullptr};
    number::Number *target_humidity_{nullptr};
    
    select::Select *mode_ = {nullptr};
};

class SkyDewPowerSwitch : public switch_::Switch {
  public:
    explicit SkyDewPowerSwitch(SkyDew *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->dew_state.status) {
        this->parent_->send_power(state);
      }
    }
  protected:
    SkyDew *parent_;
};

class SkyDewAutoModeSwitch : public switch_::Switch {
  public:
    explicit SkyDewAutoModeSwitch(SkyDew *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->dew_state.mode != 0x00)) {
        this->parent_->send_auto_mode(state);
      }
    }
  protected:
    SkyDew *parent_;
};

class SkyDewNightModeSwitch : public switch_::Switch {
  public:
    explicit SkyDewNightModeSwitch(SkyDew *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != (this->parent_->dew_state.night_mode != 0x00)) {
        this->parent_->send_night_mode(state);
      }
    }
  protected:
    SkyDew *parent_;
};

class SkyDewBeeperSwitch : public switch_::Switch {
  public:
    explicit SkyDewBeeperSwitch(SkyDew *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->dew_state.beeper) {
        this->parent_->send_beeper(state);
      }
    }
  protected:
    SkyDew *parent_;
};

class SkyDewWarmSteamSwitch : public switch_::Switch {
  public:
    explicit SkyDewWarmSteamSwitch(SkyDew *parent): parent_(parent) {}
    void write_state(bool state) override {
      if(state != this->parent_->dew_state.warm_steam) {
        this->parent_->send_warm_steam(state);
      }
    }
  protected:
    SkyDew *parent_;
};

class SkyDewTargetHumidityNumber : public number::Number {
  public:
    void set_parent(SkyDew *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->parent_->send_target_humidity((uint8_t)value);
    }
  protected:
    SkyDew *parent_;
};

class SkyDewSteamLevelNumber : public number::Number {
  public:
    void set_parent(SkyDew *parent) { this->parent_ = parent; }
    void control(float value) override {
      this->parent_->send_steam_level((uint8_t)value);
    }
  protected:
    SkyDew *parent_;
};

class SkyDewModeSelect : public select::Select {
 public:
  explicit SkyDewModeSelect(SkyDew *parent) : parent_(parent) {}
  void control(const std::string &state) override {
    this->publish_state(state);
    auto index = this->index_of(state);
    if(index.value() != this->parent_->dew_state.mode)
      this->parent_->send_mode(index.value());
    ESP_LOGD("", "Select Mode: %s (%d)", state.c_str(), index.value());
  }

 protected:
  SkyDew *parent_;
};

}  // namespace skydew
}  // namespace esphome
#endif