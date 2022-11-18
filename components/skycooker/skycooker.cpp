#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "skycooker.h"

#ifdef USE_ESP32

namespace esphome {
namespace skycooker {

static const char *const TAG = "SkyCooker";
// текстовые константы состояния мультиварки
static const char *const  RU[] = {"Бездействие", "Настройка", "Ожидание", "Нагрев", "Помощь", "Готовка", "Подогрев"};
static const char *const  EN[] = {"Hibernation", "Setting", "Waiting", "Heat", "Assistance", "Cooking", "Heating"};

float SkyCooker::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void SkyCooker::setup() {
  auto err = esp_ble_gattc_app_register(this->app_id);
  if (err) {
    ESP_LOGE(TAG, "gattc app register failed. app_id=%d code=0x%X", this->app_id, err);
    this->mark_failed();
  }
  this->set_state(r4s::DrvState::IDLE);
  this->cooker_state.type = this->type;
  this->cooker_state.last_mode = this->mode_->size() - 1;
}

void SkyCooker::dump_config() {
  ESP_LOGCONFIG(TAG, "SkyCooker:");
  ESP_LOGCONFIG(TAG, "  Model: %s (type %d)", this->usr_model.c_str(), this->type);
  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->parent()->address_str(this->address).c_str());
}

void SkyCooker::device_online_() {
  this->cooker_state.wait_command = 0x00;
  this->cooker_state.status[this->cooker_state.bowl_num] = 0xFF;
  this->cooker_state.mode[this->cooker_state.bowl_num] = 0xFF;
  this->cooker_state.submode[this->cooker_state.bowl_num] = 0xFF;
  this->cooker_state.temperature[this->cooker_state.bowl_num] = 0xFF;
  this->cooker_state.hours[this->cooker_state.bowl_num] = 0xFF;
  this->cooker_state.minutes[this->cooker_state.bowl_num] = 0xFF;
  this->cooker_state.wait_hours[this->cooker_state.bowl_num] = 0xFF;
  this->cooker_state.wait_minutes[this->cooker_state.bowl_num] = 0xFF;
  this->send_(0xFF);
}

void SkyCooker::device_offline_() {
  this->is_authorize = false;
  this->is_ready = false;
  if (this->status_ind_ != nullptr)
      this->status_ind_->publish_state((this->cooker_state.language == 0x01) ? "Не в сети" : "Off Line");
}

void SkyCooker::sync_data_() {
  if(this->is_authorize && this->is_ready) {
    if(this->cooker_state.wait_command != 0) {
      uint8_t cmd = this->cooker_state.wait_command;
      this->cooker_state.wait_command = 0x00;
      send_(cmd);
    }
    else
      this->send_(0x06);
  }
}

void SkyCooker::verify_contig_() {
  this->conf_error = true;
  size_t pos = this->mnf_model.find("RMC-");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RFS-KMC");
  if (pos != std::string::npos)
    this->conf_error = false;
  if(this->conf_error)
    ESP_LOGE(TAG, "!!! %s product and %s component are not compatible !!!", this->mnf_model.c_str(), TAG);
}

void SkyCooker::set_mode_data(uint8_t index, uint8_t targ_temp, uint8_t hors, uint8_t minutes, uint8_t flags) {
  this->data_temp[index] = targ_temp;
  this->data_hours[index] = hors;
  this->data_mins[index] = minutes;
  this->data_flags[index] = flags;
}

void SkyCooker::set_submode_data(uint8_t index, uint8_t mode, uint8_t time1, uint8_t time2, uint8_t time3, uint8_t time4) {
  this->data_product[index][0] = mode;
  this->data_product[index][1] = time1;
  this->data_product[index][2] = time2;
  this->data_product[index][3] = time3;
  this->data_product[index][4] = time4;
}

void SkyCooker::parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) {
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
        this->cooker_state.version = data[3];
        this->cooker_state.relise = data[4];
        ESP_LOGI(TAG, "%s INFO:   Version: %2.2f", this->mnf_model.c_str(), (data[3] + data[4]*0.01));
        this->send_(0x6e);
      }
      else
        err = true;
      break;
    }
    case 0x03:
    case 0x04:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x14:
    case 0x16: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x05:
    case 0x09: {
      if((data[1] == this->cmd_count) && data[3]) {
        if(this->cooker_state.automode) {
          this->cooker_state.automode = false;
          this->send_(0x03);
        }
        else
          this->send_(0x06);
      }
      else
        err = true;
      break;
    }
    case 0x06: {
      // обновление состояния выключателя
      // и состояния режима готовки (блюда)
      if((this->cooker_state.mode[this->cooker_state.bowl_num] != (data[3] + 1)) ||
          (this->cooker_state.status[this->cooker_state.bowl_num] != data[11])) {
        this->cooker_state.status[this->cooker_state.bowl_num] = data[11];
        this->cooker_state.power = (data[11] > 0x01) ? true : false;
        this->power_->publish_state(this->cooker_state.power);
        this->indication.update = true;
        this->indication.text_power = this->cooker_state.power ? 
              (this->cooker_state.language ? "Вкл." : "On"):
              (this->cooker_state.language ? "Выкл." : "Off");
        this->cooker_state.mode[this->cooker_state.bowl_num] = data[3] + 1;
        this->cooker_state.viz_mode[this->cooker_state.bowl_num] = (data[11] == 0x00) ? 0x00 : (data[3] +1);
        auto mod = this->mode_->make_call();
        mod.set_index(this->cooker_state.viz_mode[this->cooker_state.bowl_num]);
        mod.perform();
        ESP_LOGI(TAG, "%s NOTIFY: %s (mode or state)", this->mnf_model.c_str(),
                format_hex_pretty(data, data_len).c_str());
        switch(data[11]) {
          case 0x00: {
            this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[0] : EN[0];
            break;
          }
          case 0x01: {
            this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[1] : EN[1];
            break;
          }
          case 0x02: {
            if(this->cooker_state.type & 0x01)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[5] : EN[5];
            else if(this->cooker_state.type & 0xC8)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[2] : EN[2];
            else
              this->indication.text_condition = "2";
            break;
          }
          case 0x03: {
            if(this->cooker_state.type & 0x01)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[4] : EN[4];
            else if(this->cooker_state.type & 0xC8)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[3] : EN[3];
            else
              this->indication.text_condition = "3";
            break;
          }
          case 0x04: {
            if(this->cooker_state.type & 0x01)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[6] : EN[6];
            else if(this->cooker_state.type & 0xC8)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[4] : EN[4];
            else
              this->indication.text_condition = "4";
            break;
          }
          case 0x05: {
            if(this->cooker_state.type & 0x01)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[2] : EN[2];
            else if(this->cooker_state.type & 0xC8)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[5] : EN[5];
            else
              this->indication.text_condition = "5";
            break;
          }
          case 0x06: {
            if(this->cooker_state.type & 0x01)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[3] : EN[3];
            else if(this->cooker_state.type & 0xC8)
              this->indication.text_condition = (this->cooker_state.language == 0x01) ? RU[6] : EN[6];
            else
              this->indication.text_condition = "6";
            break;
          }
          default: {
            ESP_LOGI(TAG, "%s Unknown Condition: %x", this->mnf_model.c_str(), data[11]);
            break;
          }
        }
      }
      // обновление состояния подрежима готовки (блюда)
      if(this->cooker_state.submode[this->cooker_state.bowl_num] != data[4]) {
        this->cooker_state.submode[this->cooker_state.bowl_num] = data[4];
        ESP_LOGI(TAG, "%s NOTIFY: %s (submode)", this->mnf_model.c_str(),
                format_hex_pretty(data, data_len).c_str());
        if(this->submode_ != nullptr) {
          auto smod = this->submode_->make_call();
          smod.set_index(data[4]);
          smod.perform();
        }
      }
      // обновление состояния температуры готовки
      if(this->cooker_state.temperature[this->cooker_state.bowl_num] != data[5]) {
        this->cooker_state.temperature[this->cooker_state.bowl_num] = data[5];
        ESP_LOGI(TAG, "%s NOTIFY: %s (temperature)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        if(this->temperature_ != nullptr) {
          auto tcs = this->temperature_->make_call();
          tcs.set_value((float)data[5]);
          tcs.perform();
          this->temperature_->publish_state((float)data[5]);
        }
      }
      // обновление состояния времени готовки (часы)
      if(this->cooker_state.hours[this->cooker_state.bowl_num] != data[6]) {
        this->cooker_state.hours[this->cooker_state.bowl_num] = data[6];
        ESP_LOGI(TAG, "%s NOTIFY: %s (timer hours)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        if((this->timer_hours_ != nullptr) && (!this->cooker_state.timer_mode)) {
          auto hcs = this->timer_hours_->make_call();
          hcs.set_value((float)data[6]);
          hcs.perform();
          this->timer_hours_->publish_state((float)data[6]);
        }
      }
      // обновление состояния времени готовки (минуты)
      if(this->cooker_state.minutes[this->cooker_state.bowl_num] != data[7]) {
        this->cooker_state.minutes[this->cooker_state.bowl_num] = data[7];
        ESP_LOGI(TAG, "%s NOTIFY: %s (timer minutes)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        if((this->timer_minutes_ != nullptr) && (!this->cooker_state.timer_mode)) { 
          auto mcs = this->timer_minutes_->make_call();
          mcs.set_value((float)data[7]);
          mcs.perform();
          this->timer_minutes_->publish_state((float)data[7]);
        }
      }
      // обновление состояния времени отложенного старта (часы)
      if(this->cooker_state.wait_hours[this->cooker_state.bowl_num] != (int8_t)(data[8]-data[6])) {
        this->cooker_state.wait_hours[this->cooker_state.bowl_num] = (int8_t)(data[8]-data[6]);
        ESP_LOGI(TAG, "%s NOTIFY: %s (wait hours)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        this->indication.update = true;
        if((this->timer_hours_ != nullptr) && (this->cooker_state.timer_mode)) {
          auto hcs = this->timer_hours_->make_call();
          hcs.set_value((float)(data[8]-data[6]));
          hcs.perform();
          this->timer_hours_->publish_state((float)(data[8]-data[6]));
        }
      }
      // обновление состояния времени отложенного старта (минуты)
      if(this->cooker_state.wait_minutes[this->cooker_state.bowl_num] != (int8_t)(data[9]-data[7])) {
        this->cooker_state.wait_minutes[this->cooker_state.bowl_num] = (int8_t)(data[9]-data[7]);
        ESP_LOGI(TAG, "%s NOTIFY: %s (wait minutes)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        this->indication.update = true;
        if((this->timer_minutes_ != nullptr) && (this->cooker_state.timer_mode)) { 
          auto mcs = this->timer_minutes_->make_call();
          mcs.set_value((float)(data[9]-data[7]));
          mcs.perform();
          this->timer_minutes_->publish_state((float)(data[9]-data[7]));
        }
      }
      // обновление состояния пост подогрева
      if(this->cooker_state.heat[this->cooker_state.bowl_num] != data[10]) {
        this->cooker_state.heat[this->cooker_state.bowl_num] = data[10];
        ESP_LOGI(TAG, "%s NOTIFY: %s (postheat)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        this->cooker_state.postheat = (data[10] == 0x01) ? true : false;
        if(this->postheat_ != nullptr)
          this->postheat_->publish_state(this->cooker_state.postheat);
      }
      if ((this->status_ind_ != nullptr) && this->indication.update) {
        this->indication.text_time = ((data[8] < 10) ? "0" : "") + to_string(data[8]) 
              + ((data[9] < 10) ? ":0" : ":") + to_string(data[9]);
        this->status_ind_->publish_state( 
            this->indication.text_power + ", " +
            this->indication.text_condition + ", " +
            this->indication.text_time);
        this->indication.update = false;
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

void SkyCooker::send_(uint8_t command) {
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
    case 0x05:{
      this->send_data_len = 12;
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

void SkyCooker::send_power(bool state) {
  if(state) {
    if(this->cooker_state.status[this->cooker_state.bowl_num] == 0x01) {
      this->cooker_state.timer_mode = false;
      this->cooker_state.hours[this->cooker_state.bowl_num] = 0xFF;
      this->cooker_state.minutes[this->cooker_state.bowl_num] = 0xFF;
      if(this->timer_mode_ != nullptr)
        this->timer_mode_->publish_state(false);
      if(this->is_ready)
        this->send_(0x03);
      else
        this->cooker_state.wait_command = 0x03;
    }
  }
  else {
    this->cooker_state.mode[this->cooker_state.bowl_num] = 0xFF;
    if(this->is_ready)
      this->send_(0x04);
    else
      this->cooker_state.wait_command = 0x04;
  }
}

void SkyCooker::send_postheat(bool state) {
  if((this->cooker_state.status[this->cooker_state.bowl_num] != 0x00) 
      && (this->cooker_state.temperature[this->cooker_state.bowl_num] >= 80)
      && (this->data_flags[this->cooker_state.mode[this->cooker_state.bowl_num]] & 0x01)) {
    this->send_data[3] = (state) ? 0x01:0x00;
    if(this->is_ready)
      this->send_(0x16);
    else
      this->cooker_state.wait_command = 0x16;
  }
  else {
    this->cooker_state.heat[this->cooker_state.bowl_num] = 0xFF;
    if(this->is_ready)
      this->send_(0x06);
    else
      this->cooker_state.wait_command = 0x06;
  }
}

void SkyCooker::send_timer_mode(bool state) {
  if( state
      && (this->cooker_state.status[this->cooker_state.bowl_num] == 0x01) 
      && (this->data_flags[this->cooker_state.mode[this->cooker_state.bowl_num]] & 0x02)) {
    this->cooker_state.timer_mode = true;
    this->timer_mode_->publish_state(true);
    this->cooker_state.wait_hours[this->cooker_state.bowl_num] = 0xFF;
    this->cooker_state.wait_minutes[this->cooker_state.bowl_num] = 0xFF;
  }
  else {
    this->cooker_state.timer_mode = false;
    this->timer_mode_->publish_state(false);
    this->cooker_state.hours[this->cooker_state.bowl_num] = 0xFF;
    this->cooker_state.minutes[this->cooker_state.bowl_num] = 0xFF;
  }
  if(this->is_ready)
    this->send_(0x06);
  else
    this->cooker_state.wait_command = 0x06;
}

void SkyCooker::send_mode(uint8_t idx){
  if((this->cooker_state.power) || (this->data_temp[idx] == 0x00)) {
    if(!idx) {
      if(this->is_ready)
        this->send_(0x04);
      else
        this->cooker_state.wait_command = 0x04;
    }
    else {
      this->cooker_state.mode[this->cooker_state.bowl_num] = 0xFF;
      if(this->is_ready)
        this->send_(0x06);
      else
        this->cooker_state.wait_command = 0x06;
    }
  }
  else {
    this->send_data[3] = idx - 1;
    if(this->cooker_state.autostart && (this->data_flags[idx] & 0x40))
      this->cooker_state.automode = true;
    if(this->data_flags[idx] & 0x20) {
      this->send_data[4] = (this->data_flags[idx] & 0x80) ? 0x03:0x00;
      this->send_data[5] = this->data_temp[idx];
      this->send_data[6] = this->data_hours[idx];
      this->send_data[7] = this->data_mins[idx];
      this->send_data[8] = 0x00;
      this->send_data[9] = 0x00;
      this->send_data[10] = 0x00;
      if(this->is_ready)
        this->send_(0x05);
      else
        this->cooker_state.wait_command = 0x05;
    }
    else {
      if(this->is_ready)
        this->send_(0x09);
      else
        this->cooker_state.wait_command = 0x09;
    }
  }
}

void SkyCooker::send_submode(uint8_t idx){
  if( this->cooker_state.power 
      || !(this->data_flags[this->cooker_state.viz_mode[this->cooker_state.bowl_num]] & 0x80)
      || ((this->data_flags[this->cooker_state.viz_mode[this->cooker_state.bowl_num]] & 0x80) && !idx)) {
    this->cooker_state.submode[this->cooker_state.bowl_num] = 0xFF;
    if(this->is_ready)
      this->send_(0x06);
    else
      this->cooker_state.wait_command = 0x06;
  }
  else {
    this->send_data[3] = idx;
    if(this->is_ready)
      this->send_(0x0A);
    else
      this->cooker_state.wait_command = 0x0A;
    
  }
}

void SkyCooker::send_temperature(uint8_t tt) {
  if( 
      (
        (this->data_flags[this->cooker_state.viz_mode[this->cooker_state.bowl_num]] & 0x08) 
        &&
        (this->cooker_state.status[this->cooker_state.bowl_num] == 0x01)
      )
      || 
      (
        (this->data_flags[this->cooker_state.viz_mode[this->cooker_state.bowl_num]] & 0x04) 
        &&
        (
          (
            (this->cooker_state.status[this->cooker_state.bowl_num] == 0x05)
            &&
            (this->cooker_state.type & 0xFE)
          )
          ||
          (
            (this->cooker_state.status[this->cooker_state.bowl_num] == 0x02)
            &&
            (this->cooker_state.type & 0x01)
          )
        )
      )
    ) {
    this->send_data[3] = tt;
    if(this->is_ready)
      this->send_(0x0B);
    else
      this->cooker_state.wait_command = 0x0B;
  }
  else {
    this->cooker_state.temperature[this->cooker_state.bowl_num] = tt;
    if(this->is_ready)
      this->send_(0x06);
    else
      this->cooker_state.wait_command = 0x06;
  }
}

void SkyCooker::send_cooking_time(uint8_t th, uint8_t tm) {
  if(
      (this->cooker_state.status[this->cooker_state.bowl_num] == 0x01)
      ||
      (
        (this->data_flags[this->cooker_state.viz_mode[this->cooker_state.bowl_num]] & 0x04) 
        &&
        (
          (
            (
              (this->cooker_state.status[this->cooker_state.bowl_num] == 0x02)
              ||
              (this->cooker_state.status[this->cooker_state.bowl_num] == 0x06)
            )
            &&
            (this->cooker_state.type & 0x01)
          )
          ||
          (
            (
              (this->cooker_state.status[this->cooker_state.bowl_num] == 0x05)
              ||
              (this->cooker_state.status[this->cooker_state.bowl_num] == 0x03)
            )
            &&
            (this->cooker_state.type & 0xFE)
          )
        )
      )
    ) {
    this->send_data[3] = th;
    this->send_data[4] = tm;
    if(this->is_ready)
      this->send_(0x0C);
    else
      this->cooker_state.wait_command = 0x0C;
  }
  else {
    this->cooker_state.hours[this->cooker_state.bowl_num] = 0xFF;
    this->cooker_state.minutes[this->cooker_state.bowl_num] = 0xFF;
    if(this->is_ready)
      this->send_(0x06);
    else
      this->cooker_state.wait_command = 0x06;
  }
}

void SkyCooker::send_delay_time(uint8_t wth, uint8_t wtm, uint8_t th, uint8_t tm) {
  if( (this->cooker_state.status[this->cooker_state.bowl_num] == 0x01)
      && (this->data_flags[this->cooker_state.viz_mode[this->cooker_state.bowl_num]] & 0x02)) {
    uint8_t ht = wth + th;
    uint8_t mt = wtm + tm;
    if(mt >= 60) {
      ht = ht + 1;
      mt = mt - 60;
    }
    this->send_data[3] = ht;
    this->send_data[4] = mt;
    if(this->is_ready)
      this->send_(0x14);
    else
      this->cooker_state.wait_command = 0x14;
  }
  else {
    this->cooker_state.wait_hours[this->cooker_state.bowl_num] = 0xFF;
    this->cooker_state.wait_minutes[this->cooker_state.bowl_num] = 0xFF;
    if(this->is_ready)
      this->send_(0x06);
    else
      this->cooker_state.wait_command = 0x06;
  }
}

}  // namespace skycooker
}  // namespace esphome

#endif