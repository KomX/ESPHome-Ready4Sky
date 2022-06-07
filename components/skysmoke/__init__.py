import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import(
  ready4sky,
  binary_sensor,
  sensor,
)
from esphome.const import (
	CONF_ID,
	CONF_MAC_ADDRESS,
	CONF_MODEL,
	CONF_BATTERY_LEVEL,
	CONF_SIGNAL_STRENGTH,
	CONF_TEMPERATURE,
	DEVICE_CLASS_BATTERY,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  DEVICE_CLASS_TEMPERATURE,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ICON_BLUETOOTH,
  ICON_BATTERY,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_CELSIUS,
  UNIT_DECIBEL_MILLIWATT,
  UNIT_PERCENT,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["binary_sensor", "sensor"]

skysmoke_ns = cg.esphome_ns.namespace("skysmoke")
SkySmoke = skysmoke_ns.class_("SkySmoke", cg.Component, ready4sky.R4SDriver)

MODEL_TYPE = {"RSS-61S": 1, "RFS-HSS001": 1,}

MULTI_CONF = 2
CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_WARNING_SMOKE = "smoke"

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkySmoke),
      cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
      cv.Required(CONF_MODEL): cv.string,
      cv.Required(CONF_INFORM): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_WARNING_SMOKE): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement=UNIT_CELSIUS,
            ),
            cv.Optional(CONF_SIGNAL_STRENGTH): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_BLUETOOTH,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
            ),
            cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_BATTERY,
                unit_of_measurement=UNIT_PERCENT,
            ),
          }
        ),
      ),
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
  .extend(ready4sky.READY4SKY_DEVICE_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await ready4sky.register_r4s_driver(var, config)
  cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
  cg.add(var.set_model(config[CONF_MODEL]))
  cg.add(var.set_type(cv.enum(MODEL_TYPE)(config[CONF_MODEL])))

  params = config[CONF_INFORM]
  binsens = await binary_sensor.new_binary_sensor(params[CONF_WARNING_SMOKE])
  cg.add(var.set_smoke(binsens))
  if CONF_TEMPERATURE in params:
    sens = await sensor.new_sensor(params[CONF_TEMPERATURE])
    cg.add(var.set_temperature(sens))
  if CONF_SIGNAL_STRENGTH in params:
    sens = await sensor.new_sensor(params[CONF_SIGNAL_STRENGTH])
    cg.add(var.set_signal_strength(sens))
  if CONF_BATTERY_LEVEL in params:
    sens = await sensor.new_sensor(params[CONF_BATTERY_LEVEL])
    cg.add(var.set_battery_level(sens))

