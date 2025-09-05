import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import(
  ready4sky,
  sensor,
  switch,
  text_sensor,
)
from esphome.const import (
  CONF_ENTITY_CATEGORY,
  CONF_ICON,
  CONF_ID,
  CONF_MAC_ADDRESS,
  CONF_MODEL,
  CONF_POWER,
  CONF_SIGNAL_STRENGTH,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  ENTITY_CATEGORY_CONFIG,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ICON_BLUETOOTH,
  ICON_NEW_BOX,
  STATE_CLASS_MEASUREMENT,
  UNIT_DECIBEL_MILLIWATT,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["switch", "sensor", "text_sensor"]

skyiron_ns = cg.esphome_ns.namespace("skyiron")
SkyIron = skyiron_ns.class_("SkyIron", cg.Component, ready4sky.R4SDriver)

SkyIronPowerSwitch = skyiron_ns.class_("SkyIronPowerSwitch", switch.Switch)
SkyIronSafeModeSwitch = skyiron_ns.class_("SkyIronSafeModeSwitch", switch.Switch)

MODEL_TYPE = {
  "RI-C250S":  1,
  "RI-C253S":  1,
  "RI-C254S":  1,
  "RI-C255S":  1,
  "RI-C265S":  1,
  "RI-C273S":  1, "RFS-SIN001":  1,
  "RI-C288S":  1,
}

CONF_STATUS_INDICATOR = "status_indicator"
CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_SAFE_MODE = "safe_mode"
ICON_IRON = "mdi:iron"

MULTI_CONF = 1

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyIron),
      cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
      cv.Required(CONF_MODEL): cv.string,
      cv.Required(CONF_INFORM): cv.All(
        cv.Schema(
          {
            cv.Optional(CONF_SIGNAL_STRENGTH): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_BLUETOOTH,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
            ),
            cv.Optional(CONF_STATUS_INDICATOR): text_sensor.text_sensor_schema(
              {
                cv.GenerateID(): cv.declare_id(text_sensor.TextSensor),
                cv.Optional(CONF_ICON, default=ICON_NEW_BOX): cv.icon,
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_DIAGNOSTIC): cv.entity_category,
              }
            ),
          }
        ),
      ),
      cv.Required(CONF_CONTROL): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_POWER): switch.switch_schema(
              SkyIronPowerSwitch,
              icon = ICON_IRON,
            ),
            cv.Optional(CONF_SAFE_MODE): switch.switch_schema(
              SkyIronSafeModeSwitch,
              icon = "mdi:shield-lock-open-outline",
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
  if CONF_SIGNAL_STRENGTH in params:
    sens = await sensor.new_sensor(params[CONF_SIGNAL_STRENGTH])
    cg.add(var.set_signal_strength(sens))
  if CONF_STATUS_INDICATOR in params:
    conf = params[CONF_STATUS_INDICATOR]
    sens = cg.new_Pvariable(conf[CONF_ID])
    await text_sensor.register_text_sensor(sens, conf)
    cg.add(var.set_status_indicator(sens))
  
  params = config[CONF_CONTROL]
  conf = params[CONF_POWER]
  swtch = cg.new_Pvariable(conf[CONF_ID], var)
  await switch.register_switch(swtch, conf)
  cg.add(var.set_power(swtch))
  if CONF_SAFE_MODE in params:
    conf = params[CONF_SAFE_MODE]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_safe_mode(swtch))


