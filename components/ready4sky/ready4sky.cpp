#ifdef USE_ESP32

#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#include "esphome/core/time.h"
#include "ready4sky.h"

#include <nvs_flash.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_bt_defs.h>

#ifdef USE_ARDUINO
#include <esp32-hal-bt.h>
#endif

namespace esphome {
namespace ready4sky {

static const char *const TAG = "Ready4Sky";

R4SEngine *global_r4s_engine = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
static esp_bt_uuid_t r4s_service_uuid{
    .len = ESP_UUID_LEN_128,
    .uuid = {
      .uuid128 = {0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x01,0x00,0x40,0x6E},
    }
};
// 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
static esp_bt_uuid_t r4s_tx_char_uuid{
    .len = ESP_UUID_LEN_128,
    .uuid = {
      .uuid128 = {0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x02,0x00,0x40,0x6E},
    }
};
// 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
static esp_bt_uuid_t r4s_rx_char_uuid{
    .len = ESP_UUID_LEN_128,
    .uuid = {
      .uuid128 = {0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x03,0x00,0x40,0x6E},
    }
};

static esp_bt_uuid_t r4s_notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
      .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
    }
};

uint64_t ble_addr_to_uint64(const esp_bd_addr_t address) {
  uint64_t u = 0;
  u |= uint64_t(address[0] & 0xFF) << 40;
  u |= uint64_t(address[1] & 0xFF) << 32;
  u |= uint64_t(address[2] & 0xFF) << 24;
  u |= uint64_t(address[3] & 0xFF) << 16;
  u |= uint64_t(address[4] & 0xFF) << 8;
  u |= uint64_t(address[5] & 0xFF) << 0;
  return u;
}

float R4SEngine::get_setup_priority() const { return setup_priority::BLUETOOTH; }

void R4SEngine::setup() {
  global_r4s_engine = this;
  if (!this->ble_setup_()) {
    ESP_LOGE(TAG, "BLE could not be set up");
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "BLE setup complete");
  this->scan_dev_count = 0;
  this->start_scan();
}

bool R4SEngine::ble_setup_() {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "nvs_flash_init failed: %d", err);
    return false;
  }
#ifdef USE_ARDUINO
  if (!btStart()) {
    ESP_LOGE(TAG, "btStart failed: %d", esp_bt_controller_get_status());
    return false;
  }
#else
  if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
    // start bt controller
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
      esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
      err = esp_bt_controller_init(&cfg);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed: %x", err);
        return false;
      }
      while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE)
        ;
    }
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
      err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_enable failed: %x", err);
        return false;
      }
    }
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
      ESP_LOGE(TAG, "esp bt controller enable failed");
      return false;
    }
  }
#endif
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  err = esp_bluedroid_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_init failed: %x", err);
    return false;
  }
  err = esp_bluedroid_enable();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_enable failed: %x", err);
    return false;
  }
  err = esp_ble_gap_register_callback(R4SEngine::gap_event_handler_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_register_callback failed: %x", err);
    return false;
  }
  err = esp_ble_gattc_register_callback(R4SEngine::gattc_event_handler_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gattc_register_callback failed: %x", err);
    return false;
  }
  delay(200);  // NOLINT
  return true;
}

void R4SEngine::loop() {
  auto now = global_r4s_engine->get_time()->now();
  if (now.is_valid()) {
    for(auto *driver : global_r4s_engine->drivers_) {
      if(now.timestamp >= driver->sync_data_time) {
        driver->sync_data_time = now.timestamp + driver->sync_data_period;
        if(driver->state() == DrvState::ESTABLISHED)
          driver->sync_data_();
      }
    }
  }
}

void R4SEngine::start_scan() {
  if(this->is_scaned_)
    return;
  bool is_connecting = false;
  for (auto *driver : this->drivers_)
    if (driver->state() == DrvState::CONNECTING || driver->state() == DrvState::DISCOVERED) {
      is_connecting = true;
      break;
    }
  if(is_connecting)
    return;
  this->scan_dev_count = 0;
  esp_ble_gap_set_scan_params(&this->scan_params_);
//  ESP_LOGD(TAG, "Start scan...");
  this->set_timeout("scan", this->scan_duration_*2000, []() {
      ESP_LOGW(TAG, "BLE scan never terminated, rebooting to restore BLE stack...");
      App.reboot();
  });
}

void R4SEngine::gap_event_handler_( esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param ) {
  switch(event) {
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT: {
//      ESP_LOGD(TAG, "(%d) GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE", event);
      if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Config local privacy failed, error = %X", param->local_privacy_cmpl.status);
        break;
      }
      esp_ble_gap_set_scan_params(&global_r4s_engine->scan_params_);
      break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
//      ESP_LOGD(TAG, "(%d) GAP_BLE_SCAN_PARAM_SET_COMPLETE", event);
      if(param->scan_param_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Set scan param failed, error = %X", param->scan_param_cmpl.status);
        break;
      }
      esp_ble_gap_start_scanning(global_r4s_engine->scan_duration_);
      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
//      ESP_LOGD(TAG, "(%d) GAP_BLE_SCAN_START_COMPLETE", event);
      if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        ESP_LOGE(TAG, "Scan start failed, error = %X", param->scan_start_cmpl.status);
      else
        global_r4s_engine->is_scaned_ = true;
  	  break;
  	}
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
//      ESP_LOGD(TAG, "(%d) GAP_BLE_SCAN_RESULT", event);
      switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT: {
          uint64_t rst_address = ble_addr_to_uint64(param->scan_rst.bda);
          uint8_t *rst_name;
          uint8_t rst_name_len = 0;
          rst_name = esp_ble_resolve_adv_data(param->scan_rst.ble_adv,
			               ESP_BLE_AD_TYPE_NAME_CMPL, &rst_name_len);
          std::string rst_model = std::string(reinterpret_cast<const char *>(rst_name), rst_name_len);
          bool is_present = false;
          bool is_busy = true;
          for(auto *driver : global_r4s_engine->drivers_) {
            if(driver->address == rst_address) {
              is_present = true;
              driver->rssi = param->scan_rst.rssi;
              if(driver->state() == DrvState::IDLE) {
                if (driver->remote_bda[0] == 0) {
                  for (uint8_t i = 0; i < ESP_BD_ADDR_LEN; i++)
                    driver->remote_bda[i] = param->scan_rst.bda[i];
                  driver->address_type = param->scan_rst.ble_addr_type;
                  driver->mnf_model = rst_model;
                  driver->verify_contig_();
                }
                if(!driver->conf_error) {
                  driver->set_state(DrvState::DISCOVERED);
                  is_busy = false;
                }
                else {
                  driver->remote_bda[0] = 0;
                  is_present = false;
                }
              }
            }
          }
          if(!is_present) {
            if((rst_name_len > 0) && (rst_name[0] == 0x52) && global_r4s_engine->scan_monitor_) {
              bool is_saved = false;
              for(auto &nsd : global_r4s_engine->new_scan_dev_) { 
                if(nsd.address == rst_address) {
                  is_saved = true;
                  nsd.rssi = param->scan_rst.rssi;
                  break;
                }
              }
              if(!is_saved) {
                ScanDevice newdev;
                newdev.address = rst_address;
                newdev.name = rst_model;
                newdev.rssi = param->scan_rst.rssi;
                global_r4s_engine->new_scan_dev_.push_back(newdev);
              }
            }
            if(++global_r4s_engine->scan_dev_count > 25) {
              global_r4s_engine->scan_dev_count = 0;
              esp_ble_gap_stop_scanning();
            }
          }
          else {
            ++global_r4s_engine->scan_dev_count;
            if(!is_busy) {
              esp_ble_gap_stop_scanning();
            }
          }
          break;
        }
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:{
          break;  
        }
        default:{
          break;
        }
      }
      break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
//      ESP_LOGD(TAG, "(%d) GAP_BLE_SCAN_STOP_COMPLETE", event);
      if(param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Scan stop failed, error = %X", param->scan_stop_cmpl.status);
        break;
      }
      global_r4s_engine->is_scaned_ = false;
      bool is_discovered = false;
      for(auto *driver : global_r4s_engine->drivers_) {
        if(driver->state() == DrvState::DISCOVERED) {
          is_discovered = true;
          driver->set_state(DrvState::CONNECTING);
          esp_ble_gattc_open(driver->gattc_if, driver->remote_bda, driver->address_type, true);
          break;
        }
      }
      if(!is_discovered) {
        if(global_r4s_engine->scan_monitor_ && (global_r4s_engine->new_scan_dev_.size() > 0)) { 
          ESP_LOGI(TAG, "New devices (%d):", global_r4s_engine->new_scan_dev_.size() );
          for(auto &nsd : global_r4s_engine->new_scan_dev_) {
            ESP_LOGI(TAG, "   Name: %s  MAC: %s  RSSI: %d dBm", 
                  nsd.name.c_str(), global_r4s_engine->address_str(nsd.address).c_str(), nsd.rssi);
          }
//          if(global_r4s_engine->new_scan_dev_.size() > 3)
            global_r4s_engine->new_scan_dev_.empty();
        }
        global_r4s_engine->start_scan();
        break;
      }
      break;
    }
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
//      ESP_LOGD(TAG, "(%d) GAP_BLE_AUTH_CMPL", event);
      if (!param->ble_security.auth_cmpl.success) {
        ESP_LOGE(TAG, "Fail reason = 0x%X",param->ble_security.auth_cmpl.fail_reason);
      } 
      else {
        uint8_t am = param->ble_security.auth_cmpl.auth_mode;
        ESP_LOGD(TAG, "Security Auth Mode = (0x%02X) ESP_LE_AUTH_%s", am,
            (!am)?"NO_BOND":
            ((am==1)?"BOND":
            ((am==4)?"REQ_MITM":
            ((am==5)?"REQ_BOND_MITM":
            ((am==8)?"REQ_SC_ONLY": 
            ((am==9)?"REQ_SC_BOND": 
            ((am==12)?"REQ_SC_MITM":"REQ_SC_MITM_BOND")))))));
        ESP_LOGD(TAG, "Security Key = %x", param->ble_security.auth_cmpl.key);
      }
      break;
    }
    case ESP_GAP_BLE_KEY_EVT: {
//      ESP_LOGD(TAG, "(%d) GAP_BLE_KEY", event);
      uint8_t kt = param->ble_security.ble_key.key_type;
      ESP_LOGD(TAG, "Security Key Type = (0x%02X) ESP_LE_KEY_%s%s", kt,
            ((!kt)?"NONE":((kt & 0x0F)?"P":"L")),
            ((kt & 0x11)?"ENC":
            ((kt & 0x22)?"ID":
            ((kt & 0x44)?"CSRK":"LK"))));
      break;
    }
    default:{
      ESP_LOGD(TAG, "GAP Event = 0x%02X(%d)", event, event);
      break;
    }
  }
}

void R4SEngine::gattc_event_handler_( esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param ) {
  for (auto *driver : global_r4s_engine->drivers_) {
    driver->gattc_event_handler(event, gattc_if, param);
  }
}

void R4SEngine::dump_config() {
  ESP_LOGCONFIG(TAG, "Ready4Sky:");
  ESP_LOGCONFIG(TAG, "  Scan Duration: %u s", this->scan_duration_);
  ESP_LOGCONFIG(TAG, "  Scan Interval: %.1f ms", this->scan_params_.scan_interval * 0.625f);
  ESP_LOGCONFIG(TAG, "  Scan Window: %.1f ms", this->scan_params_.scan_window * 0.625f);
  ESP_LOGCONFIG(TAG, "  Scan Type: %s", this->scan_params_.scan_type ? "ACTIVE" : "PASSIVE");
  ESP_LOGCONFIG(TAG, "  Scan Monitor: %s", this->scan_monitor_ ? "ON" : "OFF");
}

std::string R4SEngine::address_str(uint64_t addr) const{
  char buf[20];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", 
    (uint8_t)(addr >> 40) & 0xff,
    (uint8_t)(addr >> 32) & 0xff,
    (uint8_t)(addr >> 24) & 0xff,
    (uint8_t)(addr >> 16) & 0xff,
    (uint8_t)(addr >> 8) & 0xff,
    (uint8_t)(addr >> 0) & 0xff);
  return (std::string)buf;
}

void R4SDriver::gattc_event_handler( esp_gattc_cb_event_t event, esp_gatt_if_t esp_gattc_if, esp_ble_gattc_cb_param_t *param ) {
  if (event == ESP_GATTC_REG_EVT && this->app_id != param->reg.app_id)
    return;
  if (event != ESP_GATTC_REG_EVT && esp_gattc_if != ESP_GATT_IF_NONE && esp_gattc_if != this->gattc_if)
    return;
  uint8_t  gatt_err = 0;
  switch (event) {
    case ESP_GATTC_REG_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_REG AppID:%d", event, this->app_id);
      if (param->reg.status == ESP_GATT_OK) {
        this->gattc_if = esp_gattc_if;
        esp_ble_gap_config_local_privacy(true);
      }
      else
        ESP_LOGE(TAG, "gattc app registration failed id=%d code=%d", param->reg.app_id, param->reg.status);
      break;
    }
    case ESP_GATTC_CONNECT_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_CONNECT AppID:%d", event, this->app_id);
      esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_ONLY;
//      esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM;
//      esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
      esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
      
      esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
      esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
      
//      uint8_t key_size = 16;
//      esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
      
//      uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
//      esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    
//      uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
//      esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
      
//      uint8_t oob_support = ESP_BLE_OOB_DISABLE;
//      esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
      
      esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);
      
      
//      esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
      break;
    }
    case ESP_GATTC_OPEN_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_OPEN AppID:%d", event, this->app_id);
      if (param->open.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Connect to %s failed, status=%d", 
            this->parent()->address_str(this->address).c_str(), param->open.status);
        this->set_state(DrvState::IDLE);
        this->is_authorize = false;
        global_r4s_engine->start_scan();
        break;
      }
      this->conn_id = param->open.conn_id;
      this->set_state(DrvState::CONNECTED);
      auto ret = esp_ble_gattc_send_mtu_req(this->gattc_if, this->conn_id);
      if (ret) {
        ESP_LOGE(TAG, "esp_ble_gattc_send_mtu_req failed, status=%d", ret);
      }
      break;
    }
    case ESP_GATTC_CFG_MTU_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_CFG_MTU AppID:%d", event, this->app_id);
      if (param->cfg_mtu.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "cfg_mtu to %s failed, status %d", 
            this->parent()->address_str(this->address).c_str(), param->cfg_mtu.status);
        gatt_err = 1;
        break;
      }
      esp_ble_gattc_search_service(this->gattc_if, this->conn_id, &r4s_service_uuid);//nullptr);
      break;
    }
    case ESP_GATTC_SEARCH_RES_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_SEARCH_RES AppID:%d", event, this->app_id);
      this->service_start_handle = param->search_res.start_handle;
      this->service_end_handle = param->search_res.end_handle;
//      ESP_LOGD(TAG, "UUID: %s, start handle: 0x%X, end handle: 0x%X.", 
//            this->uuid_to_string(param->search_res.srvc_id.uuid).c_str(),
//            param->search_res.start_handle, param->search_res.end_handle);
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_SEARCH_CMPL AppID:%d", event, this->app_id);
      if (param->search_cmpl.status != ESP_GATT_OK){
        ESP_LOGE(TAG, "search_cmpl to %s failed, status %d", 
            this->parent()->address_str(this->address).c_str(), param->search_cmpl.status);
        gatt_err = 1;
        break;
      }
      uint16_t count = 0;
      esp_gatt_status_t status = esp_ble_gattc_get_attr_count( this->gattc_if, this->conn_id,
              ESP_GATT_DB_CHARACTERISTIC, this->service_start_handle,
              this->service_end_handle, 0, &count);
      if ((status != ESP_GATT_OK) || (count == 0)){
        ESP_LOGE(TAG, "get_attr_count error or characteristics not found.");
        gatt_err = 1;
        break;
      }
      uint16_t count_tx = count;
      esp_gattc_char_elem_t char_elem_result;
      status = esp_ble_gattc_get_char_by_uuid( this->gattc_if, this->conn_id,
              this->service_start_handle, this->service_end_handle,
              r4s_tx_char_uuid, &char_elem_result, &count_tx);
      if ((status != ESP_GATT_OK) || (count_tx == 0)){
        ESP_LOGE(TAG, "TX: get_char_by_uuid error or not present");
        gatt_err = 1;
        break;
      }
      if (char_elem_result.properties & ESP_GATT_CHAR_PROP_BIT_WRITE) {
        this->tx_char_handle = char_elem_result.char_handle;
//        ESP_LOGD(TAG, "UUID: %s, TX handle: 0x%X.",
//              this->uuid_to_string(r4s_tx_char_uuid).c_str(),
//              this->tx_char_handle);
      }
      uint16_t count_rx = count;
      status = esp_ble_gattc_get_char_by_uuid( this->gattc_if, this->conn_id,
              this->service_start_handle, this->service_end_handle,
              r4s_rx_char_uuid, &char_elem_result, &count_rx);
      if ((status != ESP_GATT_OK) || (count_rx == 0)){
        ESP_LOGE(TAG, "RX: get_char_by_uuid error or not present");
        gatt_err = 1;
        break;
      }
      if (char_elem_result.properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
        this->rx_char_handle = char_elem_result.char_handle;
//        ESP_LOGD(TAG, "UUID: %s, RX handle: 0x%X.",
//              this->uuid_to_string(r4s_rx_char_uuid).c_str(),
//              this->rx_char_handle);
        esp_ble_gattc_register_for_notify(this->gattc_if, this->remote_bda, this->rx_char_handle);
      }
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_REG_FOR_NOTIFY AppID:%d", event, this->app_id);
      if (param->reg_for_notify.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "reg_for_notify to %s failed, status %d",
            this->parent()->address_str(this->address).c_str(), param->reg_for_notify.status);
        gatt_err = 1;
        break;
      }
      uint16_t count = 0;
      uint16_t notify_en = 1;
      esp_gatt_status_t status = esp_ble_gattc_get_attr_count( this->gattc_if, this->conn_id,
              ESP_GATT_DB_DESCRIPTOR, this->service_start_handle, this->service_end_handle,
              this->rx_char_handle, &count);
      if ((status != ESP_GATT_OK) || (count == 0)) {
        ESP_LOGE(TAG, "get_attr_count error or descriptors not found.");
        gatt_err = 1;
        break;
      }
      esp_gattc_descr_elem_t descr_elem_result;
      status = esp_ble_gattc_get_descr_by_char_handle( this->gattc_if, this->conn_id,
            param->reg_for_notify.handle, r4s_notify_descr_uuid, &descr_elem_result, &count);
      if (status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "get_descr_by_char_handle error.");
        gatt_err = 1;
        break;
      }
      if (count > 0 && descr_elem_result.uuid.len == ESP_UUID_LEN_16 
                    && descr_elem_result.uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
        esp_err_t err = esp_ble_gattc_write_char_descr( this->gattc_if, this->conn_id,
              descr_elem_result.handle, sizeof(notify_en), (uint8_t *)&notify_en,
              ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
        if (err != ESP_GATT_OK) {
          ESP_LOGE(TAG, "write_char_descr error.");
          gatt_err = 1;
          break;
        }
      }
      break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_WRITE_DESCR AppID:%d", event, this->app_id);
      if (param->write.status != ESP_GATT_OK){
        ESP_LOGE(TAG, "write_descr to %s failed, status %d",
            this->parent()->address_str(this->address).c_str(), param->write.status);
        gatt_err = 1;
        break;
      }
      this->device_online_();
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_WRITE_CHAR AppID:%d", event, this->app_id);
      if (param->write.status != ESP_GATT_OK){
        ESP_LOGE(TAG, "Write char error = %d", param->write.status);
//        if (param->write.status == 5)
//          esp_ble_remove_bond_device(this->remote_bda);
        gatt_err = 1;
      }
      else {
        this->set_state(DrvState::ESTABLISHED);
        global_r4s_engine->start_scan();
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_NOTIFY AppID:%d", event, this->app_id);
      if ((param->notify.is_notify) && (param->notify.handle == this->rx_char_handle)) {
        auto now = global_r4s_engine->get_time()->now();
        if (now.is_valid()) {
          this->notify_data_time = now.timestamp;
          if(this->tz_offset == 0) {
            this->tz_offset = ESPTime::timezone_offset();
          }
        }
        memcpy(this->notify_data, param->notify.value, param->notify.value_len);
        this->notify_data_len = param->notify.value_len;
        this->parse_response_(this->notify_data, this->notify_data_len, this->notify_data_time);
      }
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_DISCONNECT AppID:%d", event, this->app_id);
      break;
    }
    case ESP_GATTC_CLOSE_EVT: {
//      ESP_LOGD(TAG, "(%d) GATTC_CLOSE AppID:%d", event, this->app_id);
      this->device_offline_();
      this->set_state(DrvState::IDLE);
      global_r4s_engine->start_scan();
      break;
    }
    default:{
      ESP_LOGD(TAG, "GATTC Event = 0x%02X(%d) App_ID:%d", event, event, this->app_id);
      break;
    }
  }
  if(gatt_err) {
    ESP_LOGE(TAG, "GLOBAL GATTC ERROR %d", this->app_id);
    esp_ble_gap_disconnect(this->remote_bda);
  }
}

std::string R4SDriver::uuid_to_string(esp_bt_uuid_t ud) {
  char sbuf[48];
  switch (ud.len) {
    case ESP_UUID_LEN_16:
      sprintf(sbuf, "0x%02X%02X", ud.uuid.uuid16 >> 8, ud.uuid.uuid16 & 0xff);
      break;
    case ESP_UUID_LEN_32:
      sprintf(sbuf, "0x%02X%02X%02X%02X", ud.uuid.uuid32 >> 24, (ud.uuid.uuid32 >> 16 & 0xff),
              (ud.uuid.uuid32 >> 8 & 0xff), ud.uuid.uuid32 & 0xff);
      break;
    default:
    case ESP_UUID_LEN_128:
      char *bpos = sbuf;
      for (int8_t i = 15; i >= 0; i--) {
        sprintf(bpos, "%02X", ud.uuid.uuid128[i]);
        bpos += 2;
        if (i == 6 || i == 8 || i == 10 || i == 12)
          sprintf(bpos++, "-");
      }
      sbuf[47] = '\0';
      break;
  }
  return sbuf;
}

}  // namespace ready4sky
}  // namespace esphome

#endif
