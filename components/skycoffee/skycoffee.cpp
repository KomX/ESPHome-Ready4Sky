#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "skycoffee.h"
#include "../ready4sky/ready4sky.h"

#ifdef USE_ESP32

namespace esphome {
namespace skycoffee {

static const char *const TAG = "SkyCoffee";

float SkyCoffee::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void SkyCoffee::setup() {
  auto err = esp_ble_gattc_app_register(this->app_id);
  if (err) {
    ESP_LOGE(TAG, "gattc app register failed. app_id=%d code=0x%X", this->app_id, err);
    this->mark_failed();
  }
  this->set_state(r4s::DrvState::IDLE);
  this->coffee_state.type = this->type;
}

void SkyCoffee::dump_config() {
  ESP_LOGCONFIG(TAG, "SkyCoffee:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->usr_model.c_str());
  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->parent()->address_str(this->address).c_str());
}

void SkyCoffee::device_online_() {
  this->coffee_state.wait_command = 0x00;
  this->send_(0xFF);
}

void SkyCoffee::device_offline_() {
  this->is_authorize = false;
  this->is_ready = false;
}

void SkyCoffee::sync_data_() {
  if((this->signal_strength_ != nullptr) && (this->sync_data_time > this->update_rssi_time)) {
    this->signal_strength_->publish_state(this->rssi);
    this->update_rssi_time = this->sync_data_time + this->update_rssi_period;  
  }
  if(this->is_authorize && this->is_ready) {
    if(this->coffee_state.wait_command != 0) {
      uint8_t cmd = this->coffee_state.wait_command;
      this->coffee_state.wait_command = 0x00;
      send_(cmd);
    }
    else
      this->send_(0x06);
  }
}

void SkyCoffee::verify_contig_() {
  this->conf_error = true;
  size_t pos = this->mnf_model.find("RCM-");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RFS-KCM");
  if (pos != std::string::npos)
    this->conf_error = false;
  if(this->conf_error)
    ESP_LOGE(TAG, "!!! %s product and %s component are not compatible !!!", this->mnf_model.c_str(), TAG);
}

void SkyCoffee::parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) {
  this->is_ready = false;
  if(data[2] != 0x06)
    ESP_LOGD(TAG, "%s NOTIFY: %s", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
  bool err = false;
  switch(data[2]) {
    case 0x01: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->coffee_state.version = data[3];
        this->coffee_state.relise = data[4];
        ESP_LOGI(TAG, "%s INFO:   Version: %2.2f", this->mnf_model.c_str(), (data[3] + data[4]*0.01));
        if(this->coffee_state.type & 0x06)
          this->send_(0x6e);
        else
          this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x03: { // power ON
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x04: { // power OFF
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x06: {
      // обновление состояния выключателя
      if(this->coffee_state.status != data[11]) {
        this->coffee_state.status = data[11];
        if(this->coffee_state.status != 0x00)
          this->power_->publish_state(true);
        else
          this->power_->publish_state(false);
        ESP_LOGI(TAG, "%s NOTIFY: %s (state)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
      }
      if(this->coffee_state.type & 0x0e) {
        // обновление состояния защищённого режима
        if(this->coffee_state.lock != data[16]) {
          this->coffee_state.lock = data[16];
          ESP_LOGI(TAG, "%s NOTIFY: %s (lock)", this->mnf_model.c_str(),
                    format_hex_pretty(data, data_len).c_str());
          if(this->lock_ != nullptr) {
            if(this->coffee_state.lock != 0x00)
              this->lock_->publish_state(true);
            else
              this->lock_->publish_state(false);
          }
        }
        // обновление состояния крепости напитка
        if(this->coffee_state.strength != data[14]) {
          this->coffee_state.strength = data[14];
          ESP_LOGI(TAG, "%s NOTIFY: %s (strength)", this->mnf_model.c_str(),
                    format_hex_pretty(data, data_len).c_str());
          if(this->strength_ != nullptr) {
            if(this->coffee_state.strength != 0x00)
              this->strength_->publish_state(true);
            else
              this->strength_->publish_state(false);
          }
        }
      }
      if(this->coffee_state.type & 0x08) { //1509 
        // обновление состояния звукового режима
        if(this->coffee_state.buzzer != data[15]) {
          this->coffee_state.buzzer = data[15];
          ESP_LOGI(TAG, "%s NOTIFY: %s (buzzer)", this->mnf_model.c_str(),
                    format_hex_pretty(data, data_len).c_str());
          if(this->buzzer_ != nullptr) {
            if(this->coffee_state.buzzer != 0x00)
              this->buzzer_->publish_state(true);
            else
              this->buzzer_->publish_state(false);
          }
        }
        // обновление времени на индикаторе
        if(this->coffee_state.time_min != data[9]) {
          if((this->notify_data_time + this->tz_offset) != 0) {
            this->coffee_state.time_min = (uint8_t)((this->notify_data_time + this->tz_offset)%3600/60);
            this->coffee_state.wait_command = 0x64;
          }
        }
      }
      this->is_ready = true;
      break;
    }
    case 0x3c:
    case 0x3e:
    case 0x1b: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x47: {
      if(data[1] == this->cmd_count) {
        // получение времени наработки
        this->coffee_state.work_time = (data[5] + (data[6]<<8) + (data[7]<<16) + (data[8]<<24));
        ESP_LOGI(TAG, "%s INFO:   Work Time %d sec", this->mnf_model.c_str(), this->coffee_state.work_time);
        if(this->work_time_ != nullptr)
          this->work_time_->publish_state(this->coffee_state.work_time / 3600);
        // получение потреблённой энергии
        this->coffee_state.energy = (data[9] + (data[10]<<8) + (data[11]<<16) + (data[12]<<24));
        ESP_LOGI(TAG, "%s INFO:   Energy %d Wh", this->mnf_model.c_str(), this->coffee_state.energy);
        if(this->energy_ != nullptr)
          this->energy_->publish_state(this->coffee_state.energy*0.001);
        // получение циклов включения
        this->coffee_state.work_cycles = (data[13] + (data[14]<<8) + (data[15]<<16) + (data[16]<<24));
        ESP_LOGI(TAG, "%s INFO:   Work Cycles (47) %d", this->mnf_model.c_str(), this->coffee_state.work_cycles);
        if(this->work_cycles_ != nullptr)
          this->work_cycles_->publish_state(this->coffee_state.work_cycles);
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x64: { // set time for RCM-M1509S
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x6E: { // set time
      if((data[1] == this->cmd_count) && !data[3]) {
        this->send_(0x47);
      }
      else
        err = true;
      break;
    }
    case 0xFF: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->is_authorize = true;
        ESP_LOGI(TAG, "%s INFO:   Autorized.", this->mnf_model.c_str());
        this->send_(0x01);
      }
      else
        this->send_(0xFF);
      break;
    }
    default: {
      break;
    }
  }
  if(err) {
    ESP_LOGE(TAG, "Error command 0x%02X", data[2]);
    this->send_(0x06);
  }
}

void SkyCoffee::send_(uint8_t command) {
  this->is_ready = false;
  if(++this->cmd_count == 0)
    this->cmd_count = 1;
  this->send_data_len = 0;
  this->send_data[0] = 0x55;
  this->send_data[1] = this->cmd_count;
  this->send_data[2] = command;
  switch(command) {
    case 0x01:
    case 0x03:
    case 0x04:
    case 0x06:{
      this->send_data_len = 4;
      break;
    }
    case 0x3c:
    case 0x3e:
    case 0x1b:{
      this->send_data_len = 5;
      break;
    }
    case 0x47:{
      this->send_data[3] = 0x00;
      this->send_data_len = 5;
      break;
    }
    case 0x64:{
      this->send_data_len = 10;
      this->send_data[3] = 0x18;
      this->send_data[4] = 0x01;
      this->send_data[5] = 0x01;
      this->send_data[6] = (this->notify_data_time + this->tz_offset)%86400/3600;
      this->send_data[7] = (this->notify_data_time + this->tz_offset)%3600/60;
      this->send_data[8] = (this->notify_data_time + this->tz_offset)%60;
      break;
    }
    case 0x6E:{
      this->send_data_len = 12;
      memcpy(&this->send_data[3], &this->notify_data_time, 4);
      memcpy(&this->send_data[7], &this->tz_offset, 4);
      break;
    }
    case 0xFF:{
      this->send_data[3] = 0x2F;
      for (int i = 0; i < 6; i++) 
        this->send_data[i+4] = this->remote_bda[i] ^ this->mnf_model[i];
      this->send_data[10] = 0x2F;
      this->send_data_len = 12;
      break;
    }
    default: {
      this->send_data[2] = 0x06;
      this->send_data_len = 4;
      break;
    }
  }
  this->send_data[this->send_data_len - 1] = 0xAA;
  if(this->is_authorize || (command == 0xFF)) {
    esp_err_t status = esp_ble_gattc_write_char( this->gattc_if, this->conn_id, 
              this->tx_char_handle, this->send_data_len, this->send_data,
              ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    if ((status == ESP_GATT_OK) && (this->send_data[2] != 0x06)) {
//    if (status == ESP_GATT_OK) {
        ESP_LOGD(TAG, "%s SEND:   %s", this->mnf_model.c_str(),
              format_hex_pretty(this->send_data, this->send_data_len).c_str());
        this->send_data_len = 0;
    }
  }
}

void SkyCoffee::send_power(bool state) {
  if(state) {
    if(this->coffee_state.lock == 0x00) {
      if(this->is_ready)
        this->send_(0x03);
      else
        this->coffee_state.wait_command = 0x03;
    }
  }
  else {
    if(this->is_ready)
      this->send_(0x04);
    else
      this->coffee_state.wait_command = 0x04;
  }
}

void SkyCoffee::send_buzzer(bool state) {
  if(state)
    this->send_data[3] = 0x01;
  else
    this->send_data[3] = 0x00;
  if(this->is_ready)
    this->send_(0x3c);
  else
    this->coffee_state.wait_command = 0x3c;
}

void SkyCoffee::send_lock(bool state) {
  if(state)
    this->send_data[3] = 0x01;
  else
    this->send_data[3] = 0x00;
  if(this->is_ready)
    this->send_(0x3e);
  else
    this->coffee_state.wait_command = 0x3e;
}

void SkyCoffee::send_strength(bool state) {
  if(state)
    this->send_data[3] = 0x01;
  else
    this->send_data[3] = 0x00;
  if(this->is_ready)
    this->send_(0x1b);
  else
    this->coffee_state.wait_command = 0x1b;
}

}  // namespace skycoffee
}  // namespace esphome

#endif