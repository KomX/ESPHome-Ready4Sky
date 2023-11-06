#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "skybaker.h"


#ifdef USE_ESP32

namespace esphome {
namespace skybaker {

static const char *const TAG = "SkyBaker";
// текстовые константы состояния мультипекаря
static const char *const  RU[] = {"Бездействие", "Настройка", "Ожидание", "Нагрев", "Загрузка", "Готовка", "Подогрев"};
static const char *const  EN[] = {"Hibernation", "Setting", "Waiting", "Heat", "Loading", "Cooking", "Heating"};

float SkyBaker::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void SkyBaker::setup() {
  auto err = esp_ble_gattc_app_register(this->app_id);
  if (err) {
    ESP_LOGE(TAG, "gattc app register failed. app_id=%d code=0x%X", this->app_id, err);
    this->mark_failed();
  }
  this->set_state(r4s::DrvState::IDLE);
  this->baker_state.type = this->type;
}

void SkyBaker::dump_config() {
  ESP_LOGCONFIG(TAG, "SkyBaker:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->usr_model.c_str());
  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->parent()->address_str(this->address).c_str());
}

void SkyBaker::device_online_() {
  this->baker_state.wait_command = 0x00;
  this->send_(0xFF);
}

void SkyBaker::device_offline_() {
  this->is_authorize = false;
  this->is_ready = false;
  if (this->status_ind_ != nullptr)
      this->status_ind_->publish_state((this->baker_state.language == 0x01) ? "Не в сети" : "Off Line");
}

void SkyBaker::sync_data_() {
  if(this->is_authorize && this->is_ready) {
    if(this->baker_state.wait_command != 0) {
      uint8_t cmd = this->baker_state.wait_command;
      this->baker_state.wait_command = 0x00;
      send_(cmd);
    }
    else
      this->send_(0x06);
  }
}

void SkyBaker::verify_contig_() {
  this->conf_error = true;
  size_t pos = this->mnf_model.find("RMB-M");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RFS-KMB");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RSP-");
  if (pos != std::string::npos)
    this->conf_error = false;
  if(this->conf_error)
    ESP_LOGE(TAG, "!!! %s product and %s component are not compatible !!!", this->mnf_model.c_str(), TAG);
}

void SkyBaker::parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) {
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
      if(data[1] == this->cmd_count) {
        this->baker_state.version = data[3];
        this->baker_state.relise = data[4];
        ESP_LOGI(TAG, "%s INFO:   Version: %2.2f", this->mnf_model.c_str(), (data[3] + data[4]*0.01));
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x03:
    case 0x04: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x05: {
      if(data[1] == this->cmd_count) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x06: {
      // обновление режима работы
      if(this->baker_state.mode != data[3]) {
        this->baker_state.mode = data[3];
        ESP_LOGI(TAG, "%s NOTIFY: %s (mode)", this->mnf_model.c_str(),
                format_hex_pretty(data, data_len).c_str());
        auto mod = this->mode_->make_call();
        mod.set_index(this->baker_state.mode);
        mod.perform();
        if(this->baker_state.power) {
          this->send_(0x04);
          this->is_ready = true;
          break;
        }
      }
      // обновление состояния статуса готовки
      if(this->baker_state.status != data[11]) {
        this->baker_state.status = data[11]; 
        ESP_LOGI(TAG, "%s NOTIFY: %s (state)", this->mnf_model.c_str(),
                format_hex_pretty(data, data_len).c_str());
        this->baker_state.power = ((data[11] == 0x02) || (data[11] == 0x03) || (data[11] == 0x05) || (data[11] == 0x06)) ? true : false;
        this->power_->publish_state(this->baker_state.power);
        
        this->indication.update = true;
        this->indication.text_power = this->baker_state.power ? 
              (this->baker_state.language ? "Вкл." : "On"):
              (this->baker_state.language ? "Выкл." : "Off");

        switch(data[11]) {
          case 0x00:
          case 0x02:
          case 0x03:
          case 0x04:
          case 0x05:
          case 0x06: {
            this->indication.text_condition = (this->baker_state.language == 0x01) ? RU[data[11]] : EN[data[11]];
            break;
          }
          case 0x01:
          default: {
            ESP_LOGI(TAG, "%s Unknown Condition: %x", this->mnf_model.c_str(), data[11]);
            break;
          }
        }
      }
      // обновление состояния пост подогрева
      if(this->baker_state.postheat != (data[10] == 0x01)) {
        ESP_LOGI(TAG, "%s NOTIFY: %s (postheat)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        this->baker_state.postheat = (data[10] == 0x01) ? true : false;
        if(this->postheat_ != nullptr)
          this->postheat_->publish_state(this->baker_state.postheat);
      }
      // обновление статусной строки
      if ((this->status_ind_ != nullptr) && (this->indication.update || (data[9] != this->baker_state.minutes))) {
        this->indication.text_time = ((data[8] < 10) ? "0" : "") + to_string(data[8]) 
              + ((data[9] < 10) ? ":0" : ":") + to_string(data[9]);
        this->status_ind_->publish_state( 
            this->indication.text_power + ", " +
            this->indication.text_condition + ", " +
            this->indication.text_time);
        this->indication.update = false;
        this->baker_state.minutes = data[9];
      }
      // обновление состояния установок времени
      if(this->baker_state.update_timer && (this->timer_hours_ != nullptr) && (this->timer_minutes_ != nullptr)) {
        this->baker_state.update_timer = false;
        this->baker_state.hours =
              (this->baker_state.delay ? this->baker_state.delay_hours :
                ((this->baker_state.mode == 0x01) ? 0x00 :
                  ((this->baker_state.mode == 0x02) ? this->baker_state.heat_hours : 0x00)));
        this->baker_state.minutes =
              (this->baker_state.delay ? this->baker_state.delay_minutes :
                ((this->baker_state.mode == 0x01) ? this->baker_state.baker_minutes :
                  ((this->baker_state.mode == 0x02) ? this->baker_state.heat_minutes : 0x00)));
        auto hcs = this->timer_hours_->make_call();
        hcs.set_value((float)this->baker_state.hours);
        hcs.perform();
        auto mcs = this->timer_minutes_->make_call();
        mcs.set_value((float)this->baker_state.minutes);
        mcs.perform();
        ESP_LOGD("", "Update Timer: (%d:%d) ", this->baker_state.hours, this->baker_state.minutes);
        this->baker_state.minutes = data[9];
      }
      this->is_ready = true;
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

void SkyBaker::send_(uint8_t command) {
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
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x16:{
      this->send_data_len = 5;
      break;
    }
    case 0x0C:
    case 0x14:{
      this->send_data_len = 6;
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

void SkyBaker::send_power(bool state) {
  if(state) {
    if(this->baker_state.mode != 0x00) {
      if(this->baker_state.status != 0x05) {
        if(this->is_ready)
          this->send_(0x03);
        else
          this->baker_state.wait_command = 0x03;
      }
    }
  }
  else {
    if(this->baker_state.status != 0x04) {
      if(this->is_ready)
        this->send_(0x04);
      else
        this->baker_state.wait_command = 0x04;
    }
  }
}

void SkyBaker::send_setting(uint8_t mode, bool heat) {
  for (size_t i = 0; i < 20; i++)
    this->send_data[i] = 0x00;
  this->send_data[10] = (heat) ? 0x01 : 0x00;
  this->send_data[3] = mode;
  this->send_data[4] = (mode == 0x02) ? this->baker_state.heat_hours : 0x00;
  this->send_data[5] = (mode == 0x02) ? this->baker_state.heat_minutes : 
                       ((mode == 0x01) ? this->baker_state.baker_minutes : 0x00);
  if(this->baker_state.delay) {
    this->send_data[7] = this->baker_state.delay_hours;
    this->send_data[8] = this->baker_state.delay_minutes;
  }
  if(this->is_ready)
    this->send_(0x05);
  else
    this->baker_state.wait_command = 0x05;
}

void SkyBaker::timer_corrector(uint8_t hrs, uint8_t mns) {
  if(hrs != 0xFF) {
    if(this->baker_state.delay) {
      if((this->baker_state.delay_hours != hrs) && !this->baker_state.power && (this->baker_state.mode != 0x00)) {
        this->baker_state.delay_hours = hrs;
        if((hrs == 0x00) && (this->baker_state.delay_minutes < 0x05))
          this->baker_state.delay_minutes = 0x05;
        this->baker_state.update_timer = true;
        ESP_LOGD("", "Update True!");
      }
    }
    else if(this->baker_state.mode == 0x02) { // heating
      if((this->baker_state.heat_hours != hrs) && !this->baker_state.power) {
        this->baker_state.heat_hours = (hrs > 0x02) ? 0x02 : hrs;
        if((hrs == 0x00) && (this->baker_state.heat_minutes == 0x00))
          this->baker_state.heat_minutes = 0x01;
        this->baker_state.update_timer = true;
        ESP_LOGD("", "Update True!");
      }
    }
    else if(!this->baker_state.power) {
      this->baker_state.update_timer = true;
      ESP_LOGD("", "Update True!");
    }
  }
  else if(mns != 0xFF) {
    if(this->baker_state.delay) {
      if((this->baker_state.delay_minutes != mns) && !this->baker_state.power && (this->baker_state.mode != 0x00)) {
        this->baker_state.delay_minutes = ((this->baker_state.delay_hours == 0x00) && (mns < 0x05)) ? 0x05 : mns;
        this->baker_state.update_timer = true;
        ESP_LOGD("", "Update True!");
      }
    }
    else if(this->baker_state.mode == 0x01) { // bakering
      if((this->baker_state.baker_minutes != mns) && !this->baker_state.power) {
        this->baker_state.baker_minutes = (mns == 0x00) ? 0x01 : mns;
        this->baker_state.update_timer = true;
        ESP_LOGD("", "Update True!");
      }
    }
    else if(this->baker_state.mode == 0x02) { // heating
      if((this->baker_state.heat_minutes != mns) && !this->baker_state.power) {
        this->baker_state.heat_minutes = ((this->baker_state.heat_hours == 0x00) && (mns == 0x00)) ? 0x01 : mns;
        this->baker_state.update_timer = true;
        ESP_LOGD("", "Update True!");
      }
    }
    else if(!this->baker_state.power) {
      this->baker_state.update_timer = true;
      ESP_LOGD("", "Update True!");
    }
  }
  
  if(this->baker_state.power || (this->baker_state.mode == 0x00)) {
    this->baker_state.update_timer = true;
    ESP_LOGD("", "Update True!");
    if(this->is_ready)
      this->send_(0x06);
    else
      this->baker_state.wait_command = 0x06;
  }
  else if(this->baker_state.update_timer)
    this->send_setting(this->baker_state.mode, this->baker_state.postheat);
}

}  // namespace skybaker
}  // namespace esphome

#endif
