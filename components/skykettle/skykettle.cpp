#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "skykettle.h"

#ifdef USE_ESP32

namespace esphome {
namespace skykettle {

static const char *const TAG = "SkyKettle";

float SkyKettle::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void SkyKettle::setup() {
  auto err = esp_ble_gattc_app_register(this->app_id);
  if (err) {
    ESP_LOGE(TAG, "gattc app register failed. app_id=%d code=0x%X", this->app_id, err);
    this->mark_failed();
  }
  this->set_state(r4s::DrvState::IDLE);
  
}

void SkyKettle::dump_config() {
  ESP_LOGCONFIG(TAG, "SkyKettle:");
  ESP_LOGCONFIG(TAG, "  Model: %s (type %d)", this->usr_model.c_str(), this->type);
  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->parent()->address_str(this->address).c_str());
}

void SkyKettle::device_online() {
  this->send(0xFF);

}

void SkyKettle::device_offline() {
  this->is_authorize = false;
  this->is_active = false;
}

void SkyKettle::sync_data() {
  if(this->is_authorize && this->is_active) {
    this->send(0x06);
//    ESP_LOGI("SYNC", "TS = 0x%x", this->sync_data_time);
  }
}

void SkyKettle::cup_engine(uint8_t temp, uint32_t stamp) {
//  if((this->cup_state.algorithm & 0x07) 
//      && (((temp - this->cup_state.temp_last) >= 1) 
//      && ((stamp - this->cup_state.time_last) < 1))) {
//    this->cup_state.temp_last = temp;
//    this->cup_state.time_last = stamp;
//    this->send(0x04);
//    return;
//  }
  this->cup_state.temp_last = temp;
  this->cup_state.time_last = stamp;
      
  if(this->cup_state.algorithm) {
    uint8_t takt = (this->cup_state.algorithm << 3) + this->cup_state.phase;
    switch(takt) {
      // алгоритм 0001xxxx - нагрев воды
      case 8: {
        // подготовка переменных хранения
        this->cup_state.temp_start_1 = 0;
        this->cup_state.temp_stop_1 = 0;
        this->cup_state.temp_stop_2 = 0;
        this->cup_state.time_start_1 = 0;
        this->cup_state.value_finale = 0;
        this->cup_state.phase = 0x01; 
        break;
      }
      case 9: { // 8 + 1 - оценка диапазона температур
        int ts = (this->kettle_state.target == 0) ? (97) : ((int)this->kettle_state.target);
        int dt = ts - (int)temp;
        if(dt <= 8) {
          this->cup_state.algorithm = 0;
          this->cup_state.phase = 0;
          break;
        }
        this->cup_state.temp_start_1 = temp + 1;
        this->cup_state.temp_stop_2 = ts - 2;
        if(dt > 15) {
          this->cup_state.temp_stop_1 = temp + 8;
          this->cup_state.phase = 0x02;
        }
        else
          this->cup_state.phase = 0x03;
        break;
      }
      case 10: { // 8 + 2 - старт долгого и короткого результата
        if(temp == this->cup_state.temp_start_1) {
          this->cup_state.time_start_1 = stamp;
          this->cup_state.phase = 0x04;
        }
        break;
      }
      case 11: { // 8 + 3 - старт только долгого результата
        if(temp == this->cup_state.temp_start_1) {
          this->cup_state.time_start_1 = stamp;
          this->cup_state.phase = 0x05;
        }
        break;
      }
      case 12: { // 8 + 4 - стоп короткого результата
        if(temp == this->cup_state.temp_stop_1) {
          this->cup_state.value_finale = 
                (float)(stamp - this->cup_state.time_start_1) / (float)(temp - this->cup_state.temp_start_1);
          this->cup_state.value_temp = this->cup_state.value_finale;
          this->cup_state.phase = 0x06;
        }
        break;
      }
      case 13: { // 8 + 5 - стоп долгого результата и расчет
        if(temp == this->cup_state.temp_stop_2) {
          this->cup_state.value_finale = 
                (float)(stamp - cup_state.time_start_1) / (float)(temp - cup_state.temp_start_1);
          this->cup_state.phase = 0x07;
        }
        break;
      }
      case 14: { // 8 + 6 - стоп долгого результата и расчет среднего
        this->cup_state.value_finale = 0.0;
        if(temp == this->cup_state.temp_stop_2) {
          this->cup_state.value_finale = 
                (cup_state.value_temp + 
                (float)(stamp - cup_state.time_start_1) / (float)(temp - cup_state.temp_start_1)) * 0.5;
          this->cup_state.phase = 0x07;
        }
        break;
      }
      case 15: { // 8 + 7 - завершение расчёта
        this->cup_state.value_finale = 0.0;
        this->cup_state.algorithm = 0;
        this->cup_state.phase = 0;
        break;
      }
      // алгоритм 0010xxxx - нагрев воды до заданной температуры
      case 0x16: {
        this->cup_state.algorithm = 0;
        this->cup_state.phase = 0;
        break;
      }
      // алгоритм 0100xxxx - пробный нагрев воды на 5 градусов в активном режиме и анализ остывания
      case 0x32: {
        
        break;
      }
      // алгоритм 1000xxxx - остывание воды
      case 0x64: {
        
        break;
      }
      default: {
        
        break;
      }
    }
  }
  else {
    cup_state.phase = 0;
  }
}

void SkyKettle::parse_response(uint8_t *data, int8_t data_len, uint32_t timestamp) {
  if((data[1] == this->cmd_count) && (data[2] != 0x06))
    ESP_LOGI(TAG, "%s notify value: %s", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
  if(signal_strength_ != nullptr)
    signal_strength_->publish_state(this->rssi);

  switch(data[2]) {
    case 0x01: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->kettle_state.version = data[3] + data[4]*0.01;
        ESP_LOGI(TAG, "Version: %f", this->kettle_state.version);
        this->send(0x47);
      }
      break;
    }
    case 0x03: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->power_->publish_state(true);
        this->send(0x06);
      }
      break;
    }
    case 0x04: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->power_->publish_state(false);
        this->send(0x47);
      }
      break;
    }
    case 0x06: {
      if(data[1] == this->cmd_count) {
        this->is_active = true;
        data[1] = 0;
        uint8_t res = 0;
        for(int i=0; i<data_len; i++)
          res += data[i];
        res = 0 - res;
        if(res == this->sum_06)
          break;
        this->sum_06 = res;
        ESP_LOGI(TAG, "%s notify value: %s", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        bool new_data = false;
        // обновление целевая температура
        if((data_len == 13) && (this->kettle_state.numeric_target != data[4])) {
          this->kettle_state.numeric_target = data[4];
          uint8_t target;
          if((data[4] > 1) && (data[4] < 5))
            target = 40 + (data[4] - 1) * 15;
          else
            target = (data[4] == 5) ? 95 : 0;
          this->kettle_state.target = target;
          new_data = true;
        }
        else if((data_len == 20) && (this->kettle_state.target != data[5])) {
          this->kettle_state.target = data[5];
          new_data = true;
        }
        if(new_data) {
          
          new_data = false;
        }

        // обновление состояния выключателя чайника
        if(this->kettle_state.status != data[11]) {
          this->kettle_state.status = data[11];
          if(this->kettle_state.status) {
            this->kettle_state.power = true;
            this->power_->publish_state(true);
            if(this->kettle_state.status & 0x02)
              this->cup_state.algorithm = 1;
          }
          else {
            this->kettle_state.power = false;
            this->power_->publish_state(false);
            this->cup_state.algorithm = 0;
          }
        }

        // обновление температура воды
        if((data_len == 13) && (this->kettle_state.temperature != data[5])) {
          this->kettle_state.temperature = data[5];
          new_data = true;
        }
        else if((data_len == 20) && (this->kettle_state.temperature != data[8])) {
          this->kettle_state.temperature = data[8];
          new_data = true;
        }
        if(new_data) {
          this->temperature_->publish_state(this->kettle_state.temperature);
          cup_engine(this->kettle_state.temperature, timestamp);
          if(cup_state.value_finale != 0.0) {
            float cw = this->cup_state.value_finale * this->cup_state.cup_correct;
            if(cw != this->kettle_state.cup_quantity) {
              this->kettle_state.cup_quantity = cw;
              this->kettle_state.water_volume = cw * this->cup_state.cup_volume;
              ESP_LOGI(TAG, "Cup Quantity: %f,  Water Volume: %d", 
                      this->kettle_state.cup_quantity, 
                      this->kettle_state.water_volume);
              if(this->cup_quantity_ != nullptr)
                this->cup_quantity_->publish_state(this->kettle_state.cup_quantity);
              if(this->water_volume_ != nullptr)
                this->water_volume_->publish_state(this->kettle_state.water_volume);
            }
          }
          new_data = false;
        }
      }
      break;
    }
    case 0x47: {
      if((data[1] == this->cmd_count) && (data_len == 20)) {
        this->kettle_state.work_time = (data[5] + (data[6]<<8) + (data[7]<<16) + (data[8]<<24));
        if(work_time_ != nullptr)
          this->work_time_->publish_state(this->kettle_state.work_time / 3600);
        this->kettle_state.energy = (data[9] + (data[10]<<8) + (data[11]<<16) + (data[12]<<24));
        if(energy_ != nullptr)
          this->energy_->publish_state(this->kettle_state.energy*0.001);
        this->kettle_state.work_cycles = (data[13] + (data[14]<<8) + (data[15]<<16) + (data[16]<<24));
        if(work_cycles_ != nullptr)
          this->work_cycles_->publish_state(this->kettle_state.work_cycles);
        this->send(0x06);
      }
      break;
    }
    case 0xFF: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->is_authorize = true;
        ESP_LOGI(TAG, "%s autorized.", this->kettle_state.name.c_str());
        this->send(0x01);
      }
      else
        this->send(0xFF);
      break;
    }
    default: {
      break;
    }
  }
}

void SkyKettle::send(uint8_t command) {
  this->is_active = false;
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
    case 0x06:
    case 0x6F:
    case 0x73:{
      this->send_data_len = 4;
      break;
    }
    case 0x47:
    case 0x50:{
      this->send_data[3] = 0x00;
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
        ESP_LOGI(TAG, "SEND: Send data: %s", format_hex_pretty(this->send_data, this->send_data_len).c_str());
        this->send_data_len = 0;
    }
  }
}



}  // namespace skykettle
}  // namespace esphome

#endif