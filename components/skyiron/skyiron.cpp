#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "skyiron.h"
#include "../ready4sky/ready4sky.h"

#ifdef USE_ESP32

namespace esphome {
namespace skyiron {

static const char *const TAG = "SkyIron";

float SkyIron::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void SkyIron::setup() {
  auto err = esp_ble_gattc_app_register(this->app_id);
  if (err) {
    ESP_LOGE(TAG, "gattc app register failed. app_id=%d code=0x%X", this->app_id, err);
    this->mark_failed();
  }
  this->set_state(r4s::DrvState::IDLE);
  this->iron_state.type = this->type;
}

void SkyIron::dump_config() {
  ESP_LOGCONFIG(TAG, "SkyIron:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->usr_model.c_str());
  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->parent()->address_str(this->address).c_str());
}

void SkyIron::device_online_() {
  this->iron_state.wait_command = 0x00;
  this->send_(0xFF);
}

void SkyIron::device_offline_() {
  this->is_authorize = false;
  this->is_ready = false;
  if (this->status_ind_ != nullptr)
      this->status_ind_->publish_state("Off Line");
}

void SkyIron::sync_data_() {
  if((this->signal_strength_ != nullptr) && (this->sync_data_time > this->update_rssi_time)) {
    this->signal_strength_->publish_state(this->rssi);
    this->update_rssi_time = this->sync_data_time + this->update_rssi_period;  
  }
  if(this->is_authorize && this->is_ready) {
    if(this->iron_state.wait_command != 0) {
      uint8_t cmd = this->iron_state.wait_command;
      this->iron_state.wait_command = 0x00;
      send_(cmd);
    }
    else
      this->send_(0x06);
  }
}

void SkyIron::verify_contig_() {
  this->conf_error = true;
  size_t pos = this->mnf_model.find("RI-");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RFS-SIN");
  if (pos != std::string::npos)
    this->conf_error = false;
  if(this->conf_error)
    ESP_LOGE(TAG, "!!! %s product and %s component are not compatible !!!", this->mnf_model.c_str(), TAG);
}

void SkyIron::parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) {
  this->is_ready = false;
  if(data[2] != 0x06)
    ESP_LOGD(TAG, "%s NOTIFY: %s", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
  bool err = false;
  switch(data[2]) {
    case 0x01: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->iron_state.version = data[3];
        this->iron_state.relise = data[4];
        ESP_LOGI(TAG, "%s Version: %2.2f", this->mnf_model.c_str(), (data[3] + data[4]*0.01));
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
      this->indication.update = false;
      // обновление состояния зашишённого режима
      if(this->iron_state.safe_mode != data[10]) {
        this->iron_state.safe_mode = data[10];
        ESP_LOGI(TAG, "%s NOTIFY: %s (safe mode)", this->mnf_model.c_str(),
                    format_hex_pretty(data, data_len).c_str());
        if(this->safe_mode_ != nullptr) {
          if(this->iron_state.safe_mode == 0x00)
            this->safe_mode_->publish_state(false);
          else
            this->safe_mode_->publish_state(true);
        }
      }
      // обновление пространственного положения утюга
      if(this->iron_state.position != data[5]) {
        this->iron_state.position = data[5];
        this->iron_state.counter = 0;
        ESP_LOGI(TAG, "%s NOTIFY: %s (position)", this->mnf_model.c_str(),
                    format_hex_pretty(data, data_len).c_str());
      }
      else {
        if(++this->iron_state.counter > 5) {
          this->indication.update = true;
          if(this->iron_state.position == 0x00)
            this->indication.text_position = "Forgotten";
          else if(this->iron_state.position == 0x01)
            this->indication.text_position = "Stand";
          else if(this->iron_state.position == 0x02)
            this->indication.text_position = "Offside";
          else if(this->iron_state.position == 0x03)
            this->indication.text_position = "Upside";
        }
        else {
          if(this->indication.text_position != "Ironing") {
            this->indication.update = true;
            this->indication.text_position = "Ironing";
          }
        }
      }
      // обновление состояния выключателя утюга
      if(this->iron_state.status != data[11]) {
        this->iron_state.status = data[11];
        this->indication.update = true;
        if((this->iron_state.status == 0x00) || (this->iron_state.status == 0x07)) {
          this->iron_state.power = false;
          this->power_->publish_state(false);
          if(this->iron_state.status == 0x00)
            this->indication.text_power = "Off";
          else
            this->indication.text_power = "Auto Off";
        }
        else if(this->iron_state.status == 0x02) {
          this->iron_state.power = true;
          this->power_->publish_state(true);
          this->indication.text_power = "On";
        }
        ESP_LOGI(TAG, "%s NOTIFY: %s (state)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
      }
      if(this->indication.update)
        if (this->status_ind_ != nullptr)
          this->status_ind_->publish_state(
            this->indication.text_position + ", " + 
            this->indication.text_power);
      this->is_ready = true;
      break;
    }
    case 0x16: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0xFF: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->is_authorize = true;
        ESP_LOGI(TAG, "%s autorized.", this->mnf_model.c_str());
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

void SkyIron::send_(uint8_t command) {
  this->is_ready = false;
  if(++this->cmd_count == 0)
    this->cmd_count = 1;
  this->send_data_len = 0;
  this->send_data[0] = 0x55;
  this->send_data[1] = this->cmd_count;
  this->send_data[2] = command;
  switch(command) {
    case 0x01:
    case 0x04:
    case 0x06:{
      this->send_data_len = 4;
      break;
    }
    case 0x16:{
      this->send_data_len = 5;
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
        ESP_LOGI(TAG, "SEND: Send data: %s", format_hex_pretty(this->send_data, this->send_data_len).c_str());
        this->send_data_len = 0;
    }
  }
}

void SkyIron::send_off() {
  if(this->is_ready)
    this->send_(0x04);
  else
    this->iron_state.wait_command = 0x04;
}

void SkyIron::send_safe_mode(bool state) {
  if(state)
    this->send_data[3] = 0x01;
  else
    this->send_data[3] = 0x00;
  if(this->is_ready)
    this->send_(0x16);
  else
    this->iron_state.wait_command = 0x16;
}

}  // namespace skyiron
}  // namespace esphome

#endif