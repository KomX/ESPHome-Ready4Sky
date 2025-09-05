import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import(
  ready4sky,
  switch,
  sensor,
  number,
)
from esphome.const import (
	CONF_ACCURACY_DECIMALS,
	CONF_ENTITY_CATEGORY,
  CONF_ICON,
  CONF_ID,
  CONF_MAC_ADDRESS,
  CONF_MODE,
  CONF_MODEL,
  CONF_POWER,
  CONF_SIGNAL_STRENGTH,
  CONF_UNIT_OF_MEASUREMENT,
  DEVICE_CLASS_EMPTY,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  ENTITY_CATEGORY_CONFIG,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ENTITY_CATEGORY_NONE,
  ICON_BLUETOOTH,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_CELSIUS,
  UNIT_DECIBEL_MILLIWATT,
  UNIT_EMPTY,
  UNIT_PERCENT,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["switch", "sensor", "number"]

skyheat_ns = cg.esphome_ns.namespace("skyheat")
SkyHeat = skyheat_ns.class_("SkyHeat", cg.Component, ready4sky.R4SDriver)

SkyHeatPowerSwitch = skyheat_ns.class_("SkyHeatPowerSwitch", switch.Switch)
SkyHeatLockSwitch = skyheat_ns.class_("SkyHeatLockSwitch", switch.Switch)
SkyHeatRememberSwitch = skyheat_ns.class_("SkyHeatRememberSwitch", switch.Switch)

SkyHeatTargetPowerNumber = skyheat_ns.class_("SkyHeatTargetPowerNumber", number.Number)
SkyHeatTargetTemperatureNumber = skyheat_ns.class_("SkyHeatTargetTemperatureNumber", number.Number)

MODEL_TYPE = {
  "RCH-4525S":  1,  "RCH-4526S":  1,
  "RCH-4527S":  1,
  "RCH-4528S":  1,
  "RCH-4529S":  1,  "RCH-4530S":  1,  "RFS-HPL001": 1,
  "RCH-4550S":  1,  "RFH-4550S":  1,
  "RCH-4551S":  1,  "RFH-4551S":  1,
  "RCH-4560S":  4,
  "RCH-7001S":  2,  "RCH-7002S":  2,  "RCH-7003S":  2,
  "RFH-С4519S": 8,
  "RFH-С4522S": 16,
}

MULTI_CONF = 5
CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_LOCK = "lock"
CONF_REMEMBER = "remember_mode"
CONF_TARGET_POWER = "target_power"
CONF_TARGET_TEMPERATURE = "target_temperature"
CONF_WORK_CYCLES = "work_cycles"
CONF_WORK_TIME = "work_time"
ICON_WORK_CYCLES = "mdi:calendar-refresh"
ICON_WORK_TIME = "mdi:calendar-clock"
UNIT_HOURS = "h"


CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyHeat),
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
            cv.Required(CONF_POWER): switch.switch_schema(
              SkyHeatPowerSwitch,
              icon = "mdi:radiator",
            ),
            cv.Optional(CONF_LOCK): switch.switch_schema(
              SkyHeatLockSwitch,
              icon = "mdi:lock",
            ),
            cv.Optional(CONF_REMEMBER): switch.switch_schema(
              SkyHeatRememberSwitch,
              icon = "mdi:connection",
            ),
            cv.Optional(CONF_TARGET_POWER): number.number_schema(SkyHeatTargetPowerNumber).extend(
              {
                cv.Optional(CONF_ICON, default="mdi:power-settings"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_PERCENT): cv.string_strict,
                cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_NONE): cv.entity_category,
              }
            ),
            cv.Optional(CONF_TARGET_TEMPERATURE): number.number_schema(SkyHeatTargetTemperatureNumber).extend(
              {
                cv.Optional(CONF_ICON, default="mdi:thermometer-lines"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
                cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_NONE): cv.entity_category,
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
  modl = MODEL_TYPE[config[CONF_MODEL]]
  if CONF_INFORM in config:
    params = config[CONF_INFORM]
    if CONF_SIGNAL_STRENGTH in params:
      sens = await sensor.new_sensor(params[CONF_SIGNAL_STRENGTH])
      cg.add(var.set_signal_strength(sens))
    if modl & 1:
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
  if CONF_REMEMBER in params:
    conf = params[CONF_REMEMBER]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_remember(swtch))
  if modl & 2:
    if CONF_TARGET_POWER in params:
      numb = await number.new_number(params[CONF_TARGET_POWER], min_value=0.0, max_value=100.0, step=25.0)
      cg.add(numb.set_parent(var))
      cg.add(var.set_target_power(numb))
  if modl & 4:
    if CONF_TARGET_TEMPERATURE in params:
      numb = await number.new_number(params[CONF_TARGET_TEMPERATURE], min_value=10.0, max_value=35.0, step=1.0)
      cg.add(numb.set_parent(var))
      cg.add(var.set_target_temperature(numb))


