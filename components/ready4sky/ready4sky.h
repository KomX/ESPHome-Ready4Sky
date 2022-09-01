#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/time/real_time_clock.h"

#ifdef USE_ESP32

#include <string>
#include <array>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_defs.h>

namespace esphome {
namespace ready4sky {

#define BLE_BUFF_SIZE 24

enum class DrvState {
  IDLE, // Connection is idle, no device detected.
  DISCOVERED, // Device advertisement found.
  CONNECTING, // Connection in progress.
  CONNECTED, // Initial connection established.
  ESTABLISHED, // The client and sub-clients have completed setup.
};

struct ScanDevice {
  uint64_t address;
  std::string name;
  int rssi;
};

static esp_gattc_char_elem_t *char_elem_result   = NULL;

class R4SEngine;

class R4SDriver {
  public:
    virtual void parse_response_(uint8_t *data, int8_t size, uint32_t timestamp) = 0;
    virtual void device_online_() = 0;
    virtual void device_offline_() = 0;
    virtual void sync_data_() = 0;
    virtual void verify_contig_() = 0;
    
    void set_address(uint64_t address) { this->address = address; }
    void set_model(std::string model) { this->usr_model = model; }
    void set_type(uint8_t type) { this->type = type; }
    void set_state(DrvState st) { this->state_ = st; }
    void set_engine_parent(R4SEngine *parent) { this->parent_ = parent; }
    void gattc_event_handler( esp_gattc_cb_event_t event,
                              esp_gatt_if_t gattc_if, 
                              esp_ble_gattc_cb_param_t *param );
    DrvState state() const { return state_; }
    R4SEngine *parent() { return this->parent_; }
    std::string uuid_to_string(esp_bt_uuid_t uuid);

    int app_id;
    int gattc_if;
    uint16_t conn_id;
    
    esp_bd_addr_t remote_bda{ 0, };
    esp_ble_addr_type_t address_type{BLE_ADDR_TYPE_PUBLIC};
    int rssi{0};
    
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t rx_char_handle;
    uint16_t tx_char_handle;

    uint64_t address;
    std::string usr_model = ""; // User specified model
    std::string mnf_model = ""; // Model specified by the manufacturer
    uint8_t type = 0;
    
    bool    is_authorize = false;
    bool    conf_error = true;
    uint8_t cmd_count = 1;
    uint8_t notify_data[BLE_BUFF_SIZE];
    int8_t  notify_data_len = 0;
    int32_t notify_data_time = 0;
    uint8_t send_data[BLE_BUFF_SIZE];
    int8_t  send_data_len = 0;
    int32_t sync_data_time = 0;
    int32_t sync_data_period = 1;
    int32_t update_rssi_time = 0;
    int8_t  update_rssi_period = 10;
    int32_t tz_offset = 0;

  protected:
    DrvState state_;
    R4SEngine *parent_{nullptr};
};

class R4SEngine : public Component {
  public:
    float get_setup_priority() const override;
    void setup() override;
    void dump_config() override;
    
    void set_scan_duration(uint32_t scan_duration) { scan_duration_ = scan_duration; }
    void set_scan_active(bool scan_active) { scan_params_.scan_type = scan_active ? BLE_SCAN_TYPE_ACTIVE:BLE_SCAN_TYPE_PASSIVE; }
    void set_scan_parameters(uint32_t scan_interval, uint32_t scan_window) {
      scan_params_.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
      scan_params_.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
      scan_params_.scan_interval = scan_interval;
      scan_params_.scan_window = scan_window;
      scan_params_.scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE;
    }
    void set_time(time::RealTimeClock *time) { time_ = time; }
    time::RealTimeClock *get_time() const { return time_; }
    void set_monitor(bool scan_monitor) { scan_monitor_ = scan_monitor; }
    bool get_monitor() const { return scan_monitor_; }
    void register_r4s_driver(R4SDriver *driver) {
      driver->app_id = ++this->app_id_;
      driver->set_engine_parent(this);
      this->drivers_.push_back(driver);
    }
    std::string address_str(uint64_t addr) const;
    void start_scan();
    uint16_t scan_dev_count;

  protected:
    static bool ble_setup_();
    static void gap_event_handler_(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void gattc_event_handler_(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

    int app_id_;
    esp_ble_scan_params_t scan_params_;
    uint32_t scan_duration_;
    bool is_scaned_ = false;
    bool scan_monitor_ = false;
    
    std::vector<ScanDevice> new_scan_dev_;
    std::vector<R4SDriver *> drivers_;
    time::RealTimeClock *time_{nullptr};
};
// NOLINTNEXTLINE
extern R4SEngine *global_r4s_engine;

}  // namespace ready4sky
}  // namespace esphome

#endif
