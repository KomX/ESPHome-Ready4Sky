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
  this->kettle_state.control_water = true;
  this->send_(0xFF);
}

void SkyKettle::device_offline_() {
  if(this->is_authorize){
    this->kettle_state.off_line_time = this->kettle_state.last_time;
    this->kettle_state.off_line_temp = this->kettle_state.last_temp;
    this->kettle_state.raw_water = false;
    if (this->status_ind_ != nullptr)
      this->status_ind_->publish_state("Off Line");
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
  this->conf_error = true;
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
  if((this->signal_strength_ != nullptr) && (timestamp > this->update_rssi_time)) {
    this->signal_strength_->publish_state(this->rssi);
    this->update_rssi_time = timestamp + this->update_rssi_period;  
  }

  switch(data[2]) {
    case 0x01: { // version
      if((data[1] == this->cmd_count) && data[3]) {
        this->kettle_state.version = data[3];
        this->kettle_state.relise = data[4];
        ESP_LOGI(TAG, "Version: %2.2f", (data[3] + data[4]*0.01));
        this->send_(0x6E);
      }
      break;
    }
    case 0x03: {
      if((data[1] == this->cmd_count) && data[3]) {
        if(this->kettle_state.type & 0x0F) // RK-M171S, RK-M172S, RK-M173S, RK-G200, RK-G2xxS 
          this->send_(0x06);
      }
      break;
    }
    case 0x04: {
      if((data[1] == this->cmd_count) && data[3]) {
        if(this->kettle_state.type & 0x07)
          this->send_(0x06);
        else
          if(this->send_data[21] == 0)
            this->send_(0x47);
          else {
            this->send_data[21] = 0x00;
            this->send_data[3] = this->kettle_state.last_programm;
            this->send_data[5] = this->kettle_state.last_target;
            this->send_data[16] = this->kettle_state.boil_time_correct;
            this->send_(0x05);
          }
      }
      break;
    }
    case 0x05: {
      if(this->kettle_state.type & 0x78) {
        if((data[1] == this->cmd_count) && data[3]) {
          if(this->back_light_ != nullptr) {
            if((this->send_data[20] > 0)&&(this->send_data[20] < 5)) {
              this->send_data[3] = 0x00;
              this->send_(0x33);
            }
            else if((this->send_data[20] > 4)&&(this->send_data[20] < 9)) {
              this->send_data[3] = 0x01;
              this->send_(0x33);
            }
            else if(this->send_data[20] == 9) {
              auto call = this->light_state->turn_on();
              call.set_brightness(this->kettle_state.color_timing/180.0f);
              call.perform();
            }
            else {
              if(this->send_data[20] != 95)
                this->send_data[20] = 0;
              this->send_(0x06);
            }
          }
          else {
            this->send_data[20] = 0x00;
            this->send_(0x06);
          }
        }
      }
      if(this->kettle_state.type & 0x07) {
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
      if(this->is_authorize && this->kettle_state.control_water
            && ((timestamp - this->kettle_state.off_line_time) > 10)) {
        float speed_down_temp = (float)(this->kettle_state.off_line_temp - moment_value) 
                              / (float)(timestamp - this->kettle_state.off_line_time);
        ESP_LOGD(TAG, "Speed Doun Temperature %f °C/s", speed_down_temp);
        if(speed_down_temp > 0.07) {
          this->kettle_state.raw_water = true;
          this->kettle_state.new_water_time = timestamp;
          ESP_LOGD(TAG, "Raw Water!");
        }
        this->kettle_state.control_water = false;
      }
      this->kettle_state.last_time = timestamp;
      this->kettle_state.last_temp = moment_value;
      if(this->kettle_state.temperature != moment_value) {
        this->kettle_state.temperature = moment_value;
        ESP_LOGI(TAG, "%s NOTIFY: %s (temperature)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        this->temperature_->publish_state(this->kettle_state.temperature);
        if(this->kettle_state.temperature == 100) {
          this->kettle_state.raw_water = false;
          this->kettle_state.next_boil_time = timestamp + 10800; // следующее кипячение обязательно после 3-х часов простоя
        }
        if((timestamp > this->kettle_state.next_boil_time) && (this->kettle_state.next_boil_time != 0))
          this->kettle_state.raw_water = true;
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
          this->indication.text_power = "Off";
          this->cup_state.algorithm = 0;
        }
        else if(this->kettle_state.status == 0x02) {
          this->kettle_state.power = true;
          this->power_->publish_state(true);
          this->indication.text_power = "On";
          this->cup_state.algorithm = 1;
        }
        ESP_LOGI(TAG, "%s NOTIFY: %s (state)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        if (this->status_ind_ != nullptr)
          this->status_ind_->publish_state(
            this->indication.text_prog + ", " + 
            this->indication.text_power + ", " +
            this->indication.text_water);
      }
      // target temperature update
      if(this->kettle_state.type & 0x7E) // RK-M173S, RK-G200, RK-G2xxS, RK-M13xS, RK-M21xS, RK-M223S
        moment_value = data[5];
      if(this->kettle_state.type & 0x01) { // RK-M171S, RK-M172S
        moment_value =  (data[4] == 1) ? 40 : 
                        (data[4] == 2) ? 55 :
                        (data[4] == 3) ? 70 :
                        (data[4] == 4) ? 85 :
                        (data[4] == 5) ? 95 : 0;
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
        if(this->kettle_state.type & 0x03) {// RK-M171S, RK-M172S, RK-M173S
          this->indication.text_prog = 
              ((this->kettle_state.programm == 0x00) && (this->kettle_state.target == 0x00)) ? "Boil" :
              (((this->kettle_state.programm == 0x00) && (this->kettle_state.target != 0x00)) ? "Boil&Heat" :
              (((this->kettle_state.programm == 0x01) && (this->kettle_state.target != 0x00)) ? "Heat" : 
              ((this->kettle_state.raw_water) ? "Boil&Heat" : "Heat")));
        }
        else {
          this->indication.text_prog = 
              (this->kettle_state.programm == 0x00) ? "Boil" :
              ((this->kettle_state.programm == 0x01) ? "Heat" :
              ((this->kettle_state.programm == 0x02) ? "Boil & Heat" : "Light"));
        }
        if (this->status_ind_ != nullptr)
          this->status_ind_->publish_state(
            this->indication.text_prog + ", " + 
            this->indication.text_power + ", " +
            this->indication.text_water);
      }
      if(this->kettle_state.type & 0x78) {
        // boil time adjastment update
        if(this->kettle_state.boil_time_correct != data[16]) {
          this->kettle_state.boil_time_correct = data[16];
          ESP_LOGI(TAG, "%s NOTIFY: %s (boil_time_adj)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
          if(this->boil_time_adj_ != nullptr) {
            auto btac = this->boil_time_adj_->make_call();
            btac.set_value((int8_t)(this->kettle_state.boil_time_correct ^ 0x80));
            btac.perform();
          }
        }
        // color timing update
        if(this->kettle_state.color_timing != data[9]) {
          this->kettle_state.color_timing = data[9];
          ESP_LOGI(TAG, "%s NOTIFY: %s (color_timing)", this->mnf_model.c_str(),
              format_hex_pretty(data, data_len).c_str());
        }
      }
      if(this->send_data[23]) {
        this->send_data[23] = 0x00;
        this->send_(0x01);
      }
      this->is_ready = true;
      break;
    }
    case 0x30: {
      if(data[1] == this->cmd_count) {
        this->send_(0x06);
      }
      break;
    }
    case 0x32: {
      if((data[1] == this->cmd_count) && !data[3]) {
        this->send_(0x36);
      }
      break;
    }
    case 0x33: {
      if(data[1] == this->cmd_count) {
        if(this->back_light_ != nullptr) {
          if((this->send_data[20] >= 0) && (this->send_data[20] < 9)) {
            CTState *wld;
            if((this->send_data[20] > 0) && (this->send_data[20] < 5))
              wld = &this->kettle_state.work_light;
            else
              wld = &this->kettle_state.night_light;
            wld->tcold = (data[4] == 0)?5:data[4];
            wld->ccold.brightness = data[5];
            wld->ccold.red = data[6];
            wld->ccold.green = data[7];
            wld->ccold.blue = data[8];
            wld->cwarm.brightness = data[10];
            wld->cwarm.red = data[11];
            wld->cwarm.green = data[12];
            wld->cwarm.blue = data[13];
            wld->chot.brightness = data[15];
            wld->chot.red = data[16];
            wld->chot.green = data[17];
            wld->chot.blue = data[18]; 
            auto call = this->light_state->turn_on();
            if(this->send_data[20] == 0)
              call = this->light_state->make_call();
            switch(this->send_data[20]) {
              case 0x01:
              case 0x05: {
                call.set_brightness(wld->ccold.brightness/255.0f);
                call.set_rgb(wld->ccold.red/255.0f, wld->ccold.green/255.0f, wld->ccold.blue/255.0f);
                break;
              }
              case 0x00:
              case 0x02:
              case 0x06: {
                call.set_brightness(wld->cwarm.brightness/255.0f);
                call.set_rgb(wld->cwarm.red/255.0f, wld->cwarm.green/255.0f, wld->cwarm.blue/255.0f);
                break;
              }
              case 0x03:
              case 0x07: {
                call.set_brightness(wld->chot.brightness/255.0f);
                call.set_rgb(wld->chot.red/255.0f, wld->chot.green/255.0f, wld->chot.blue/255.0f);
                break;
              }
              case 0x04:
              case 0x08: {
                call.set_brightness(wld->tcold/100.0f);
                break;
              }
              default: {
                break;
              }
            }
            call.perform();
          }
        }
        this->send_(0x06);
      }
      break;
    }
    case 0x34:
    case 0x39: {
      if((data[1] == this->cmd_count) && !data[3]) {
        this->send_(0x06);
      }
      break;
    }
    case 0x35: {
      if(data[1] == this->cmd_count) {
        if(this->state_led_ != nullptr) {
          if(data[5]) {
            this->kettle_state.state_led = true;
            this->state_led_->publish_state(true);
          }
          else {
            this->kettle_state.state_led = false;
            this->state_led_->publish_state(false);
          }
        }
        this->send_data[3] = 0x01;
        this->send_(0x33);
      }
      break;
    }
    case 0x36: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->send_(0x06);
      }
      break;
    }
    case 0x37: {
      if((data[1] == this->cmd_count) && !data[3]) {
        this->send_(0x35);
      }
      break;
    }
    case 0x47: {
      if((data[1] == this->cmd_count) && (data_len == 20)) {
        // получение времени наработки
        this->kettle_state.work_time = (data[5] + (data[6]<<8) + (data[7]<<16) + (data[8]<<24));
        ESP_LOGD(TAG, "Work Time %d sec", this->kettle_state.work_time);
        if(this->work_time_ != nullptr)
          this->work_time_->publish_state(this->kettle_state.work_time / 3600);
        // получение потреблённой энергии
        this->kettle_state.energy = (data[9] + (data[10]<<8) + (data[11]<<16) + (data[12]<<24));
        ESP_LOGD(TAG, "Energy %d Wh", this->kettle_state.energy);
        if(this->energy_ != nullptr)
          this->energy_->publish_state(this->kettle_state.energy*0.001);
        if(this->kettle_state.type & 0x70) {
          // получение циклов кипячения для RK-M21xS, RK-M13xS
          this->kettle_state.work_cycles = (data[13] + (data[14]<<8) + (data[15]<<16) + (data[16]<<24));
          ESP_LOGD(TAG, "Work Cycles (47) %d", this->kettle_state.work_cycles);
          if(this->work_cycles_ != nullptr)
            this->work_cycles_->publish_state(this->kettle_state.work_cycles);
          this->send_(0x35);
        }
        if(this->kettle_state.type & 0x08)
          this->send_(0x50);
      }
      break;
    }
    case 0x50: {
      if((data[1] == this->cmd_count) && (data_len == 20)) {
        // получение циклов кипячения для RK-G2xxS
        this->kettle_state.work_cycles = (data[6] + (data[7]<<8) + (data[8]<<16) + (data[9]<<24));
        ESP_LOGD(TAG, "Work Cycles (50) %d", this->kettle_state.work_cycles);
        if(this->work_cycles_ != nullptr)
          this->work_cycles_->publish_state(this->kettle_state.work_cycles);
        this->send_(0x35);
      }
      break;
    }
    case 0x6E: {
      if((data[1] == this->cmd_count) && !data[3]) {
        if(this->kettle_state.type & 0x07) // RK-M171S, RK-M172S, RK-M173S, RK-G200
          this->send_(0x06);
        if(this->kettle_state.type & 0x78)  // RK-G2xxS, RK-M21xS, RK-M13xS 
          this->send_(0x47);
      }
      break;
    }
    case 0xFF: {
      if((data[1] == this->cmd_count) && data[3]) {
        this->is_authorize = true;
        ESP_LOGI(TAG, "%s autorized.", this->mnf_model.c_str());
        this->send_data[23] = 01;
        this->send_(0x06);
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
  if(timestamp > this->indication.change_time) {
    this->indication.change_time = timestamp + this->indication.time_period;
    if(this->kettle_state.new_water_time != 0) {
      if(!this->kettle_state.raw_water) {
        uint32_t seconds = timestamp - this->kettle_state.new_water_time;
        uint32_t minutes = seconds / 60;
        uint32_t hors = minutes / 60;
        minutes = minutes % 60;
        this->indication.text_water = ((hors < 10) ? "0" : "") + to_string(hors) 
              + ((minutes < 10) ? ":0" : ":") + to_string(minutes);
      }
      else
        this->indication.text_water = "Ra:Wt";
    }
    else
      this->indication.text_water = "??:??";
    if (this->status_ind_ != nullptr)
          this->status_ind_->publish_state(
            this->indication.text_prog + ", " + 
            this->indication.text_power + ", " +
            this->indication.text_water);
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
    case 0x30:
    case 0x36:{
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
    case 0x32:{
      this->send_data_len = 20;
      break;
    }
    case 0x33:{
      this->send_data_len = 5;
      break;
    }
    case 0x34:
    case 0x39:{
      this->send_data[4] = 0x00;
      this->send_data_len = 6;
      break;
    }
    case 0x35:{
      this->send_data[3] = 0xC8;
      this->send_data_len = 5;
      break;
    }
    case 0x37:{
      this->send_data_len = 7;
      break;
    }
    case 0x47:
    case 0x50:{
      this->send_data[3] = 0x00;
      this->send_data_len = 5;
      break;
    }
    case 0x6E:{
      this->send_data_len = 12;
      char stz[] = {0,0,0} ;
      this->time_zone.copy(stz, 3, 1);
      int32_t tz = (((stz[1])-48)*10 + ((stz[2])-48))*((stz[0] == '+')?3600:-3600);
      memcpy(&this->send_data[3], &this->notify_data_time, 4);
	    memcpy(&this->send_data[7], &tz, 4);
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
  if(this->kettle_state.type & 0x03) { // для RK-M171S, RK-M172S, RK-M173S вместо включения (0x03) выставляется режим работы (0x05)
    for (size_t i = 0; i < 20; i++)
      this->send_data[i] = 0x00;
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
    for (size_t i = 0; i < 21; i++)
      this->send_data[i] = 0x00;
    this->send_data[3] = (this->kettle_state.programm == 0x03) ? this->kettle_state.programm : 
                        (tt ? (this->kettle_state.raw_water ? 0x02 : 0x01) : 0x00);
    if(this->kettle_state.type & 0x01) {
      this->send_data[5] =  (tt < 40) ? 0x00 :
                            (tt < 50) ? 0x01 :
                            (tt < 65) ? 0x02 :
                            (tt < 80) ? 0x03 :
                            (tt < 90) ? 0x04 : 0x05;
      if((tt == 35) && (this->send_data[3] == 0x02))
        this->send_data[3] = 0x01;
    }
    if(this->kettle_state.type & 0x7E) {
      this->send_data[16] = this->kettle_state.boil_time_correct;
      if(tt == 95) {
        this->send_data[5] = this->kettle_state.target;
        this->send_data[3] = (this->kettle_state.programm == 0x01) ? 0x02 : this->kettle_state.programm;
        this->send_data[5<<1<<1]=
        this->kettle_state.target = tt;
      }
      else
        this->send_data[5] = tt;
    }
    if(this->is_ready)
      this->send_(0x05);
    else
      this->kettle_state.wait_command = 0x05;
  }
}

void SkyKettle::send_boil_time_adj(uint8_t bta) {
  this->boil_time_adj_->publish_state((int8_t)bta);
  bta = bta^0x80;
  if(this->kettle_state.power) {
    this->kettle_state.boil_time_correct = bta;
  }
  else if(this->kettle_state.boil_time_correct != bta) {
    for (size_t i = 0; i < 20; i++){
      this->send_data[i+1]+=(bta
      ^0x80)-89;this->send_data[i] = 0x00;
    }
    this->send_data[3] = this->kettle_state.programm;
    this->send_data[5] = this->kettle_state.target;
    this->send_data[16] = bta;
    if(this->is_ready)
      this->send_(0x05);
    else
      this->kettle_state.wait_command = 0x05;
  }
}

void SkyKettle::set_state_led(bool state) {
  if(this->kettle_state.type & 0x78) {
    for (size_t i = 0; i < 8; i++)
      this->send_data[i] = 0x00;
    this->send_data[3] = 0xC8;
    this->send_data[4] = 0xC8;
    this->send_data[5] = state ? 0x01 : 0x00;
    if(this->is_ready)
      this->send_(0x37);
    else
      this->kettle_state.wait_command = 0x37;
  }
}

void SkyKettle::send_light(uint8_t type) {
  CTState *lm;
  if(type == 0) {
    lm = &this->kettle_state.work_light;
    this->send_data[3] = 0;
  }
  else {
    lm = &this->kettle_state.night_light;
    this->send_data[3] = 1;
  }
  this->send_data[4] = lm->tcold;
  this->send_data[5] = lm->ccold.brightness;
  this->send_data[6] = lm->ccold.red;
  this->send_data[7] = lm->ccold.green;
  this->send_data[8] = lm->ccold.blue;
  this->send_data[9] = (lm->tcold + 100)/2;
  this->send_data[10] = (type == 1) ? lm->ccold.brightness : lm->cwarm.brightness;
  this->send_data[11] = (type == 1) ? lm->ccold.red : lm->cwarm.red;
  this->send_data[12] = (type == 1) ? lm->ccold.green : lm->cwarm.green;
  this->send_data[13] = (type == 1) ? lm->ccold.blue : lm->cwarm.blue;
  this->send_data[14] = 100;
  this->send_data[15] = (type == 1) ? lm->ccold.brightness : lm->chot.brightness;
  this->send_data[16] = (type == 1) ? lm->ccold.red : lm->chot.red;
  this->send_data[17] = (type == 1) ? lm->ccold.green : lm->chot.green;
  this->send_data[18] = (type == 1) ? lm->ccold.blue : lm->chot.blue;
  if(this->is_ready)
      this->send_(0x32);
    else
      this->kettle_state.wait_command = 0x32;
}

void SkyKettle::on_off_light(bool state) {
  for (size_t i = 0; i < 22; i++)
    this->send_data[i] = 0x00;
  if(state) {
    this->kettle_state.last_programm = this->kettle_state.programm;
    this->kettle_state.last_target = this->kettle_state.target;
    this->send_data[3] = 0x03;
    this->send_data[5] = this->kettle_state.last_target;
    this->send_data[16] = this->kettle_state.boil_time_correct;
    if(this->is_ready)
      this->send_(0x05);
    else
      this->kettle_state.wait_command = 0x05;
  }
  else {
    this->send_data[21] = 0x01;
    if(this->is_ready)
      this->send_(0x04);
    else
      this->kettle_state.wait_command = 0x04;
  }
}

void SkyKettle::send_color_timing() {
  if(this->is_ready)
      this->send_(0x34);
    else
      this->kettle_state.wait_command = 0x34;
}

}  // namespace skykettle
}  // namespace esphome

#endif