import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import time
from esphome.components.esp32 import add_idf_sdkconfig_option, const, get_esp32_variant
from esphome.const import (
  CONF_ID,
  CONF_INTERVAL,
  CONF_DURATION,
  CONF_TIME_ID,
  PLATFORM_ESP32,
)
from esphome.core import CORE
from esphome.components.esp32 import add_idf_sdkconfig_option

CODEOWNERS = ["@KomX"]
CONFLICTS_WITH = ["esp32_ble_tracker", "esp32_ble_beacon"]
DEPENDENCIES = ["esp32"]
ESP_PLATFORMS = [PLATFORM_ESP32]


CONF_SCAN_PARAMETERS = "scan_parameters"
CONF_ACTIVE = "active"
CONF_WINDOW = "window"
CONF_MONITOR = "monitor"

CONF_R4S_ID = "r4s_id"

ready4sky_ns = cg.esphome_ns.namespace("ready4sky")
R4SEngine = ready4sky_ns.class_("R4SEngine", cg.Component)
R4SDriver = ready4sky_ns.class_("R4SDriver")

def validate_scan_parameters(config):
  duration = config[CONF_DURATION]
  interval = config[CONF_INTERVAL]
  window = config[CONF_WINDOW]
  if window > interval:
    raise cv.Invalid(
      f"Scan window ({window}) needs to be smaller than scan interval ({interval})"
    )
  if interval.total_milliseconds * 3 > duration.total_milliseconds:
    raise cv.Invalid(
      "Scan duration needs to be at least three times the scan interval to"
      "cover all BLE channels."
    )
  return config

CONFIG_SCHEMA = cv.Schema(
  {
    cv.GenerateID(): cv.declare_id(R4SEngine),
    cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.Optional(CONF_SCAN_PARAMETERS, default={}): cv.All(
      cv.Schema(
        {
          cv.Optional(CONF_DURATION, default="30s"): cv.positive_time_period_seconds,
          cv.Optional(CONF_INTERVAL, default="150ms"): cv.positive_time_period_milliseconds,
          cv.Optional(CONF_WINDOW, default="50ms"): cv.positive_time_period_milliseconds,
          cv.Optional(CONF_ACTIVE, default=True): cv.boolean,
          cv.Optional(CONF_MONITOR, default=False): cv.boolean,
        }
      ),
      validate_scan_parameters,
    ),
  }
).extend(cv.COMPONENT_SCHEMA)

READY4SKY_DEVICE_SCHEMA = cv.Schema(
  {
    cv.GenerateID(CONF_R4S_ID): cv.use_id(R4SEngine),
  }
)


def as_hex(value):
  return cg.RawExpression(f"0x{value}ULL")

async def register_r4s_driver(var, config):
  paren = await cg.get_variable(config[CONF_R4S_ID])
  cg.add(paren.register_r4s_driver(var))
  return var

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    timevar = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(timevar))
    params = config[CONF_SCAN_PARAMETERS]
    cg.add(var.set_scan_duration(params[CONF_DURATION]))
    cg.add(var.set_scan_active(params[CONF_ACTIVE]))
    cg.add(var.set_scan_parameters(int(params[CONF_INTERVAL].total_milliseconds/0.625), int(params[CONF_WINDOW].total_milliseconds/0.625)))
    cg.add(var.set_monitor(params[CONF_MONITOR]))
    
    if CORE.using_esp_idf:
        add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
    
