import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import(
  ready4sky,
  sensor,
  switch,
)
from esphome.const import (
  CONF_ENTITY_CATEGORY,
  CONF_ICON,
  CONF_ID,
  CONF_MAC_ADDRESS,
  CONF_MODEL,
  CONF_POWER,
  CONF_SIGNAL_STRENGTH,
  DEVICE_CLASS_EMPTY,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  ENTITY_CATEGORY_CONFIG,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ICON_BLUETOOTH,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_DECIBEL_MILLIWATT,
  UNIT_EMPTY,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["switch", "sensor"]

skyplug_ns = cg.esphome_ns.namespace("skyplug")
SkyPlug = skyplug_ns.class_("SkyPlug", cg.Component, ready4sky.R4SDriver)

SkyPlugPowerSwitch = skyplug_ns.class_("SkyPlugPowerSwitch", switch.Switch)
SkyPlugLockSwitch = skyplug_ns.class_("SkyPlugLockSwitch", switch.Switch)

MODEL_TYPE = {
  "RSP-100S":  1,
  "RSP-103S":  1,
}

CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_LOCK = "lock"
CONF_WORK_CYCLES = "work_cycles"
CONF_WORK_TIME = "work_time"
ICON_PLUG = "mdi:power-socket-eu"
ICON_WORK_CYCLES = "mdi:calendar-refresh"
ICON_WORK_TIME = "mdi:calendar-clock"
UNIT_HOURS = "h"

MULTI_CONF = 5

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyPlug),
      cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
      cv.Required(CONF_MODEL): cv.string,
      cv.Optional(CONF_INFORM): cv.All(
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
            cv.Optional(CONF_WORK_CYCLES): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_WORK_CYCLES,
                state_class=STATE_CLASS_TOTAL_INCREASING,
                unit_of_measurement=UNIT_EMPTY,
            ),
            cv.Optional(CONF_WORK_TIME): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
                icon=ICON_WORK_TIME,
                state_class=STATE_CLASS_TOTAL_INCREASING,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                unit_of_measurement=UNIT_HOURS,
            ),
          }
        ),
      ),
      cv.Required(CONF_CONTROL): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_POWER): switch.SWITCH_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyPlugPowerSwitch),
                cv.Optional(CONF_ICON, default=ICON_PLUG): switch.icon,
              }
            ),
            cv.Optional(CONF_LOCK): switch.SWITCH_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyPlugLockSwitch),
                cv.Optional(CONF_ICON, default="mdi:lock"): switch.icon,
              }
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

  if CONF_INFORM in config:
    params = config[CONF_INFORM]
    if CONF_SIGNAL_STRENGTH in params:
      sens = await sensor.new_sensor(params[CONF_SIGNAL_STRENGTH])
      cg.add(var.set_signal_strength(sens))
    if CONF_WORK_CYCLES in params:
      sens = await sensor.new_sensor(params[CONF_WORK_CYCLES])
      cg.add(var.set_work_cycles(sens))
    if CONF_WORK_TIME in params:
      sens = await sensor.new_sensor(params[CONF_WORK_TIME])
      cg.add(var.set_work_time(sens))

  params = config[CONF_CONTROL]
  conf = params[CONF_POWER]
  swtch = cg.new_Pvariable(conf[CONF_ID], var)
  await switch.register_switch(swtch, conf)
  cg.add(var.set_power(swtch))
  if CONF_LOCK in params:
    conf = params[CONF_LOCK]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_lock(swtch))

