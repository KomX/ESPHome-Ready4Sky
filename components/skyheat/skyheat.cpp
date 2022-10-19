#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "skyheat.h"

#ifdef USE_ESP32

namespace esphome {
namespace skyheat {

static const char *const TAG = "SkyHeat";

float SkyHeat::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void SkyHeat::setup() {
  auto err = esp_ble_gattc_app_register(this->app_id);
  if (err) {
    ESP_LOGE(TAG, "gattc app register failed. app_id=%d code=0x%X", this->app_id, err);
    this->mark_failed();
  }
  this->set_state(r4s::DrvState::IDLE);
  this->heat_state.type = this->type;
}

void SkyHeat::dump_config() {
  ESP_LOGCONFIG(TAG, "SkyHeat:");
  ESP_LOGCONFIG(TAG, "  Model: %s (type %d)", this->usr_model.c_str(), this->type);
  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->parent()->address_str(this->address).c_str());
}

void SkyHeat::device_online_() {
  this->heat_state.wait_command = 0x00;
  this->send_(0xFF);
}

void SkyHeat::device_offline_() {
  this->is_authorize = false;
  this->is_ready = false;
}

void SkyHeat::sync_data_() {
  if(this->is_authorize && this->is_ready) {
    if(this->heat_state.wait_command != 0) {
      uint8_t cmd = this->heat_state.wait_command;
      this->heat_state.wait_command = 0x00;
      send_(cmd);
    }
    else
      this->send_(0x06);
  }
}

void SkyHeat::verify_contig_() {
  this->conf_error = true;
  size_t pos = this->mnf_model.find("RCH-");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RFS-HPL");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RFH-");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RSP-"); //!!!
  if (pos != std::string::npos)       //!!!
    this->conf_error = false;         //!!!
  if(this->conf_error)
    ESP_LOGE(TAG, "!!! %s product and %s component are not compatible !!!", this->mnf_model.c_str(), TAG);
}

void SkyHeat::parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) {
  this->is_ready = false;
  if(data[2] != 0x06)
    ESP_LOGI(TAG, "%s NOTIFY: %s", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
  if((this->signal_strength_ != nullptr) && (timestamp > this->update_rssi_time)) {
    this->signal_strength_->publish_state(this->rssi);
    this->update_rssi_time = timestamp + this->update_rssi_period;  
  }
  bool err = false;
  switch(data[2]) {
    case 0x01: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->heat_state.version = data[3];
        this->heat_state.relise = data[4];
        ESP_LOGI(TAG, "%s INFO:   Version: %2.2f", this->mnf_model.c_str(), (data[3] + data[4]*0.01));
        this->send_(0x6e);
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
        if(this->heat_state.type & 0x01)
          this->send_(0x47);
        else
          this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x05: {
      if((data[1] == this->cmd_count) && data[3])
        this->send_(0x06);
      else
        err = true;
      break;
    }
    case 0x06: {
      // обновление состояния защищённого режима
      if(this->heat_state.lock != data[10]) {
        this->heat_state.lock = data[10];
        ESP_LOGI(TAG, "%s NOTIFY: %s (lock)", this->mnf_model.c_str(),
                    format_hex_pretty(data, data_len).c_str());
        if(this->lock_ != nullptr)
          this->lock_->publish_state(this->heat_state.lock != 0x00);
      }
      // обновление состояния выключателя
      if(this->heat_state.status != data[11]) {
        this->heat_state.status = data[11];
        this->power_->publish_state(this->heat_state.status == 0x02);
        ESP_LOGI(TAG, "%s NOTIFY: %s (state)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
      }
      if(this->heat_state.type & 0x02) {// RCH-700xS
        // target power update
        if(this->heat_state.target_power != data[5]) {
          this->heat_state.target_power = data[5];
          ESP_LOGI(TAG, "%s NOTIFY: %s (target_power)", this->mnf_model.c_str(),
                format_hex_pretty(data, data_len).c_str());
          if(this->target_power_ != nullptr) {
            auto tgc = this->target_power_->make_call();
            tgc.set_value(this->heat_state.target_power);
            tgc.perform();
          }
        }
      }
      // remember mode update
      if( ( (this->heat_state.remember != data[13]) && (this->heat_state.type & 0x01) )
          ||
          ( (this->heat_state.remember != data[6]) && (this->heat_state.type & 0x02) )
        ) {
        this->heat_state.remember = (this->heat_state.type & 0x01) ? data[13] : data[6];
          ESP_LOGI(TAG, "%s NOTIFY: %s (remember mode)", this->mnf_model.c_str(),
                format_hex_pretty(data, data_len).c_str());
          if(this->heat_state.remember != 0x01)
            this->remember_->publish_state(false);
          else
            this->remember_->publish_state(true);
        }
      this->is_ready = true;
      break;
    }
    case 0x16:
    case 0x44: {
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
        this->heat_state.work_time = (data[5] + (data[6]<<8) + (data[7]<<16) + (data[8]<<24));
        ESP_LOGI(TAG, "%s INFO:   Work Time %d sec", this->mnf_model.c_str(), this->heat_state.work_time);
        if(this->work_time_ != nullptr)
          this->work_time_->publish_state(this->heat_state.work_time / 3600);
        // получение циклов включения
        this->heat_state.work_cycles = (data[13] + (data[14]<<8) + (data[15]<<16) + (data[16]<<24));
        ESP_LOGI(TAG, "%s INFO:   Work Cycles (47) %d", this->mnf_model.c_str(), this->heat_state.work_cycles);
        if(this->work_cycles_ != nullptr)
          this->work_cycles_->publish_state(this->heat_state.work_cycles);
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x6E: { // set time
      if((data[1] == this->cmd_count) && !data[3]) {
        if(this->heat_state.type & 0x01)
          this->send_(0x47);
        else
          this->send_(0x06);
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

void SkyHeat::send_(uint8_t command) {
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
    case 0x05:{
      this->send_data_len = 20;
      break;
    }
    case 0x16:
    case 0x44:{
      this->send_data_len = 5;
      break;
    }
    case 0x47:{
      this->send_data[3] = 0x00;
      this->send_data_len = 5;
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

void SkyHeat::send_power(bool state) {
  if(state) {
    if(this->heat_state.lock == 0x00) {
      if(this->heat_state.type & 0x01) {
        if(this->is_ready)
          this->send_(0x03);
        else
          this->heat_state.wait_command = 0x03;
      }
      else {
        for (size_t i = 0; i < 20; i++)
          this->send_data[i] = 0x00;
        this->send_data[5] = 100;
        if(this->is_ready)
          this->send_(0x05);
        else
          this->heat_state.wait_command = 0x05;
      }
    }
  }
  else {
    if(this->is_ready)
      this->send_(0x04);
    else
      this->heat_state.wait_command = 0x04;
  }
}

void SkyHeat::send_lock(bool state) {
  if(state)
    this->send_data[3] = 0x01;
  else
    this->send_data[3] = 0x00;
  if(this->is_ready)
    this->send_(0x16);
  else
    this->heat_state.wait_command = 0x16;
}

void SkyHeat::send_remember(bool state) {
  if(state)
    this->send_data[3] = 0x01;
  else
    this->send_data[3] = 0x00;
  if(this->is_ready)
    this->send_(0x44);
  else
    this->heat_state.wait_command = 0x44;
}

void SkyHeat::send_target_power(uint8_t tt) {
  for (size_t i = 0; i < 20; i++)
      this->send_data[i] = 0x00;
  this->send_data[5] = tt;
  if(this->is_ready)
    this->send_(0x05);
  else
    this->heat_state.wait_command = 0x05;
/*  tt = (tt == 100) ? 0x00 : tt;
  if(this->kettle_state.power || (this->kettle_state.programm == 0x03)) {
    this->kettle_state.target = tt;
    if(this->is_ready)
      this->send_(0x06);
    else
      this->kettle_state.wait_command = 0x06;
  }
  else if(this->kettle_state.target != tt) {
    for (size_t i = 0; i < 21; i++)
      this->send_data[i] = 0x00;
    this->send_data[3] = tt ? (this->kettle_state.raw_water ? 0x02 : 0x01) : 0x00;
    if(this->kettle_state.type & 0x1A) {
      this->send_data[16] = this->kettle_state.boil_time_correct;
      if(tt == 95) {
        this->send_data[5] = this->kettle_state.target;
        this->send_data[3] = (this->kettle_state.programm == 0x01) ? 0x02 : this->kettle_state.programm;
        if(this->kettle_state.type & 0x18)
          this->send_data[5<<1<<1] = ((this->kettle_state.target)?0:tt);
        this->kettle_state.target = tt;
      }
      else
        this->send_data[5] = tt;
      if(this->is_ready)
        this->send_(0x05);
      else
        this->kettle_state.wait_command = 0x05;
    }
  }*/
}

void SkyHeat::send_target_temperature(uint8_t tt) {
  this->target_temperature_->publish_state(tt);

}

}  // namespace skyheat
}  // namespace esphome

#endif