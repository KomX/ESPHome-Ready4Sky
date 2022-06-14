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
  this->kettle_state.type = this->type;
}

void SkyKettle::dump_config() {
  ESP_LOGCONFIG(TAG, "SkyKettle:");
  ESP_LOGCONFIG(TAG, "  Model: %s (type %d)", this->usr_model.c_str(), this->type);
  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->parent()->address_str(this->address).c_str());
}

void SkyKettle::device_online_() {
  this->send_(0xFF);
}

void SkyKettle::device_offline_() {
  if(this->is_authorize){
    this->kettle_state.off_line_time = this->kettle_state.last_time;
    this->kettle_state.off_line_temp = this->kettle_state.last_temp;
  }
  this->is_authorize = false;
  this->is_ready = false;
}

void SkyKettle::sync_data_() {
  if(this->is_authorize && this->is_ready) {
    if(this->kettle_state.wait_command != 0) {
      uint8_t cmd = this->kettle_state.wait_command;
      this->kettle_state.wait_command = 0x00;
      send_(cmd);
    }
    else if(this->kettle_state.type & 0x09) // RK-M171S, RK-M172S, RK-G2xxS 
      this->send_(0x06);
  }
}

void SkyKettle::verify_contig_() {
  size_t pos = this->mnf_model.find("RK-");
  if (pos != std::string::npos)
    this->conf_error = false;
  pos = this->mnf_model.find("RFS-KKL");
  if (pos != std::string::npos)
    this->conf_error = false;
  if(this->conf_error)
    ESP_LOGE(TAG, "!!! %s product and %s component are not compatible !!!", this->mnf_model.c_str(), TAG);
}

void SkyKettle::cup_engine_(uint8_t temp, uint32_t stamp) {
  if(this->cup_state.algorithm) {
    uint8_t takt = (this->cup_state.algorithm << 3) + this->cup_state.phase;
    switch(takt) {
      // алгоритм 0001xxxx - нагрев воды
      case 8: {
        // подготовка переменных
        this->cup_state.temp_start_1 = 0;
        this->cup_state.temp_stop_1 = 0;
        this->cup_state.temp_stop_2 = 0;
        this->cup_state.time_start_1 = 0;
        this->cup_state.value_finale = 0;
        this->cup_state.phase = 0x01; 
        break;
      }
      case 9: { // 8 + 1 - оценка диапазона температур
        int ts = 97; //(this->kettle_state.target == 0) ? (97) : ((int)this->kettle_state.target);
        int dt = ts - (int)temp;
        if(dt <= 8) {
          this->cup_state.algorithm = 0;
          this->cup_state.phase = 0;
          break;
        }
        this->cup_state.temp_start_1 = temp + 1;
        this->cup_state.temp_stop_2 = ts - 2;
        if(dt > 15) {
          this->cup_state.temp_stop_1 = temp + 5;
          this->cup_state.phase = 0x02;
        }
        else
          this->cup_state.phase = 0x03;
        break;
      }
      case 10: { // 8 + 2 - старт долгого и короткого результата
        if(temp >= this->cup_state.temp_start_1) {
          this->cup_state.time_start_1 = stamp;
          this->cup_state.phase = 0x04;
        }
        break;
      }
      case 11: { // 8 + 3 - старт только долгого результата
        if(temp >= this->cup_state.temp_start_1) {
          this->cup_state.time_start_1 = stamp;
          this->cup_state.phase = 0x05;
        }
        break;
      }
      case 12: { // 8 + 4 - стоп короткого результата
        if(temp >= this->cup_state.temp_stop_1) {
          this->cup_state.value_finale = 
                (float)(stamp - this->cup_state.time_start_1) / (float)(temp - this->cup_state.temp_start_1);
          this->cup_state.value_temp = this->cup_state.value_finale;
          this->cup_state.phase = 0x06;
        }
        break;
      }
      case 13: { // 8 + 5 - стоп долгого результата и расчет
        if(temp >= this->cup_state.temp_stop_2) {
          this->cup_state.value_finale = 
                (float)(stamp - cup_state.time_start_1) / (float)(temp - cup_state.temp_start_1);
          this->cup_state.phase = 0x07;
        }
        break;
      }
      case 14: { // 8 + 6 - стоп долгого результата и расчет среднего
        this->cup_state.value_finale = 0.0;
        if(temp >= this->cup_state.temp_stop_2) {
          this->cup_state.value_finale = 
                (cup_state.value_temp + 
                (float)(stamp - cup_state.time_start_1) / (float)(temp - cup_state.temp_start_1)) * 0.5;
          this->cup_state.phase = 0x07;
        }
        break;
      }
      case 15: { // 8 + 7 - finish
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

void SkyKettle::parse_response_(uint8_t *data, int8_t data_len, uint32_t timestamp) {
  this->is_ready = false;
  if(data[2] != 0x06)
    ESP_LOGI(TAG, "%s NOTIFY: %s", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
  if((this->signal_strength_ != nullptr) && (timestamp >= this->update_rssi_time)) {
    this->signal_strength_->publish_state(this->rssi);
    this->update_rssi_time = timestamp + this->update_rssi_period;  
  }

  switch(data[2]) {
    case 0x01: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->kettle_state.version = data[3];
        this->kettle_state.relise = data[4];
        ESP_LOGI(TAG, "Version: %2.2f", (data[3] + data[4]*0.01));
        if(this->kettle_state.type & 0x01)
          this->send_(0x06);
        else
          this->send_(0x47);
      }
      break;
    }
    case 0x03: {
      if((data[1] == this->cmd_count) && data[3]) {
        if(this->kettle_state.type & 0x09) // RK-M171S, RK-M172S, RK-G2xxS 
          this->send_(0x06);
      }
      break;
    }
    case 0x04: {
      if((data[1] == this->cmd_count) && data[3]) {
        if(this->kettle_state.temperature == 100)
          this->kettle_state.raw_water = false;
        if(this->kettle_state.type & 0x01)
          this->send_(0x06);
        else
          this->send_(0x47);
      }
      break;
    }
    case 0x05: {
      if(this->kettle_state.type & 0x78) {
        if((data[1] == this->cmd_count) && data[3])
          this->send_(0x06);
      }
      else if(this->kettle_state.type & 0x07) {
        if((data[1] == this->cmd_count) && !data[3])
          this->send_(0x06);
      }
      break;
    }
    case 0x06: {
      uint8_t moment_value = 0 ;
      // обновление температура воды
      if(this->kettle_state.type & 0x78) // RK-G2xxS, RK-M13xS, RK-M21xS, RK-M223S
        moment_value = data[8];
      if(this->kettle_state.type & 0x06) // RK-M173S, RK-G200
        moment_value = data[13];
      if(this->kettle_state.type & 0x01) // RK-M171S, RK-M172S
        moment_value = data[5];
      this->kettle_state.last_time = timestamp;
      this->kettle_state.last_temp = moment_value;
      if(this->is_authorize && !this->kettle_state.raw_water
                            && ((this->kettle_state.off_line_temp - moment_value) > 5)
                            && ((timestamp - this->kettle_state.off_line_time) < 180)) {
        this->kettle_state.raw_water = true;
        this->kettle_state.new_water_time = timestamp;
      }
      if(this->kettle_state.temperature != moment_value) {
        this->kettle_state.temperature = moment_value;
        ESP_LOGI(TAG, "%s NOTIFY: %s (temperature)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        this->temperature_->publish_state(this->kettle_state.temperature);
        
        cup_engine_(this->kettle_state.temperature, timestamp);
        if(cup_state.value_finale != 0.0) {
          float cw = this->cup_state.value_finale * this->cup_state.cup_correct;
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
      // обновление состояния выключателя чайника
      if(this->kettle_state.status != data[11]) {
        this->kettle_state.status = data[11];
        if(this->kettle_state.status == 0x00) {
          this->kettle_state.power = false;
          this->power_->publish_state(false);
          this->cup_state.algorithm = 0;
        }
        else if(this->kettle_state.status == 0x02) {
          this->kettle_state.power = true;
          this->power_->publish_state(true);
          this->cup_state.algorithm = 1;
        }
      }
      // target temperature update
      if(this->kettle_state.type & 0x7E) // RK-M173S, RK-G200, RK-G2xxS, RK-M13xS, RK-M21xS, RK-M223S
        moment_value = data[5];
      if(this->kettle_state.type & 0x01) { // RK-M171S, RK-M172S
        if((data[4] > 1) && (data[4] < 5))
          moment_value = 40 + (data[4] - 1) * 15;
        else
          moment_value = (data[4] == 5) ? 95 : 0;
        if(this->kettle_state.numeric_target != data[4])
          this->kettle_state.numeric_target = data[4];
      }
      if(this->kettle_state.target != moment_value) {
        this->kettle_state.target = moment_value;
        ESP_LOGI(TAG, "%s NOTIFY: %s (target)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        if(this->target_ != nullptr) { // is present target temperature number entity
          moment_value = (this->kettle_state.target == 0) ? 100 : this->kettle_state.target;
          auto tgc = this->target_->make_call();
          tgc.set_value(moment_value);
          tgc.perform();
        }
      }
      // обновление программного режима
      if(this->kettle_state.programm != data[3]) {
        this->kettle_state.programm = data[3];
        ESP_LOGI(TAG, "%s NOTIFY: %s (programm)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        if (this->status_ind_ != nullptr) {
          this->status_ind_->publish_state(
              (this->kettle_state.programm == 0x00) ? "Boil" :
              (this->kettle_state.programm == 0x01) ? "Heat" : "Boil & Heat");
        }
        if((this->kettle_state.programm == 0x02) && (this->kettle_state.temperature == 100))
          this->kettle_state.raw_water = false;
      }
      // boil time adjastment update
      if(this->kettle_state.type & 0x78) {
        if(this->kettle_state.boil_time != data[16]) {
          this->kettle_state.boil_time = data[16];
          ESP_LOGI(TAG, "%s NOTIFY: %s (boil_time_adj)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
          if(this->boil_time_adj_ != nullptr) { // is present boil time adjastment number entity
//            int8_t btad = (int8_t)(this->kettle_state.boil_time ^ 0x80);
            auto btac = this->boil_time_adj_->make_call();
            btac.set_value((int8_t)(this->kettle_state.boil_time ^ 0x80));
            btac.perform();
          }
        }
      }
      this->is_ready = true;
      break;
    }
    case 0x47: {
      if((data[1] == this->cmd_count) && (data_len == 20)) {
        this->kettle_state.work_time = (data[5] + (data[6]<<8) + (data[7]<<16) + (data[8]<<24));
        if(this->work_time_ != nullptr)
          this->work_time_->publish_state(this->kettle_state.work_time / 3600);
        this->kettle_state.energy = (data[9] + (data[10]<<8) + (data[11]<<16) + (data[12]<<24));
        if(this->energy_ != nullptr)
          this->energy_->publish_state(this->kettle_state.energy*0.001);
        this->kettle_state.work_cycles = (data[13] + (data[14]<<8) + (data[15]<<16) + (data[16]<<24));
        if(this->work_cycles_ != nullptr)
          this->work_cycles_->publish_state(this->kettle_state.work_cycles);
        this->send_(0x06);
      }
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
      this->send_(0x06);
      break;
    }
  }
}

void SkyKettle::send_(uint8_t command) {
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
    case 0x06:
    case 0x6F:
    case 0x73:{
      this->send_data_len = 4;
      break;
    }
    case 0x05:{
      if(this->kettle_state.type & 0x78)      // RK-G2xxS, RK-M13xS, RK-M21xS, RK-M223S
        this->send_data_len = 20;
      else {
        this->send_data_len = 7;              // RK-M171S, RK-M172S, RK-M173S, RK-G200
        if(this->kettle_state.type & 0x03) {  // RK-M171S, RK-M172S, RK-M173S
          if(this->send_data[3] == 0x02)
            this->send_data[3] = 0x00;
        }
        if(this->kettle_state.type & 0x01) {  // RK-M171S, RK-M172S
          this->send_data[4] = this->send_data[5];
          this->send_data[5] = 0x00;
        }
      }
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
        ESP_LOGI(TAG, "%s SEND:   %s", this->mnf_model.c_str(),
            format_hex_pretty(this->send_data, this->send_data_len).c_str());
    }
  }
}

void SkyKettle::send_on() {
  if(this->kettle_state.type & 0x03) {
    for (size_t i = 1; i < 20; i++)
      this->send_data[i] = 0x00;
    this->send_data[3] = this->kettle_state.programm;
    this->send_data[5] = this->kettle_state.target;
    if(this->is_ready)
      this->send_(0x05);
    else
      this->kettle_state.wait_command = 0x05;
  }
  else {
    if(this->is_ready)
      this->send_(0x03);
    else
      this->kettle_state.wait_command = 0x03;
  }
}

void SkyKettle::send_off() {
  if(this->is_ready)
    this->send_(0x04);
  else
    this->kettle_state.wait_command = 0x04;
}

void SkyKettle::send_target_temp(uint8_t tt) {
  this->target_->publish_state(tt);
  tt = (tt == 100) ? 0x00 : tt;
  if(this->kettle_state.power) {
    this->kettle_state.target = tt;
  }
  else if(this->kettle_state.target != tt) {
    for (size_t i = 1; i < 20; i++)
      this->send_data[i] = 0x00;
    if(tt != 95) {
      this->send_data[3] = tt ? (this->kettle_state.raw_water ? 0x02 : 0x01) : 0x00;
      this->send_data[5] = tt;
      this->send_data[16] = this->kettle_state.boil_time;
      if(this->kettle_state.type & 0x7E) {
        if(this->is_ready)
          this->send_(0x05);
        else
          this->kettle_state.wait_command = 0x05;
      }
    }
    else
      this->kettle_state.target = tt;
  }
}

void SkyKettle::send_boil_time_adj(uint8_t bta) {
  this->boil_time_adj_->publish_state((int8_t)bta);
  bta = bta^0x80;
  if(this->kettle_state.power) {
    this->kettle_state.boil_time = bta;
  }
  else if(this->kettle_state.boil_time != bta) {
    for (size_t i = 1; i < 20; i++)
      this->send_data[i] = 0x00;
    this->send_data[3] = this->kettle_state.programm;
    this->send_data[5] = this->kettle_state.target;
    this->send_data[16] = bta;
    if(this->is_ready)
      this->send_(0x05);
    else
      this->kettle_state.wait_command = 0x05;
  }
}

}  // namespace skykettle
}  // namespace esphome

#endif