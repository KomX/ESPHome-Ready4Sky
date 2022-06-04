import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import(
  ready4sky,
#  binary_sensor,
#  climate,
#  cover,
#  fan,
#  light,
#  number,
  sensor,
  switch,
#  text_sensor,
)
from esphome.const import (
	CONF_ENERGY,
	CONF_ICON,
	CONF_ID,
	CONF_MAC_ADDRESS,
	CONF_MODEL,
	CONF_POWER,
	CONF_SENSORS,
	CONF_SIGNAL_STRENGTH,
	CONF_SWITCHES,
	CONF_TEMPERATURE,
	DEVICE_CLASS_EMPTY,
	DEVICE_CLASS_ENERGY,
	DEVICE_CLASS_POWER,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  DEVICE_CLASS_TEMPERATURE,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ICON_BLUETOOTH,
  ICON_COUNTER,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_CELSIUS,
  UNIT_DECIBEL_MILLIWATT,
  UNIT_EMPTY,
  UNIT_KILOWATT_HOURS,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["switch", "sensor"]

skykettle_ns = cg.esphome_ns.namespace("skykettle")
SkyKettle = skykettle_ns.class_("SkyKettle", cg.Component, ready4sky.R4SDriver)

SkyKettlePowerSwitch = skykettle_ns.class_("SkyKettlePowerSwitch", switch.Switch)

MODEL_TYPE = {"RK-G200":    1,
              "RK-G200S":   2, "RK-G201S":  2, "RK-G202S": 2, "RK-G203S": 2, "RK-G204S": 2, 
              "RK-G210S":   4, "RK-G211S":  4, "RK-G212S": 4, "RK-G213S": 4, "RK-G214S": 4, "RK-G215S": 4, 
              "RK-G233S":   8, "RK-G240S":  8, 
              "RK-M136S":  16, "RK-M139S":  16,
              "RK-M170S":  32, "RK-M171S":  32, 
              "RK-M173S":  64, 
              "RK-M215S": 128, "RK-M216S": 128, "RK-M223S": 128,
}

CONF_CUP_CORRECT = "cup_correction"
CONF_CUP_QUANTITY = "cup_quantity"
CONF_CUP_VOLUME = "cup_volume"
CONF_WATER_VOLUME = "water_volume"
CONF_WORK_CYCLES = "work_cycles"
CONF_WORK_TIME = "work_time"
ICON_COFFEE =  "mdi:coffee"
ICON_CUP =  "mdi:cup"
ICON_KETTLE = "mdi:kettle"
ICON_WORK_CYCLES = "mdi:calendar-refresh"
ICON_WORK_TIME = "mdi:calendar-clock"
UNIT_HOURS = "h"
MULTI_CONF = 2

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyKettle),
      cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
      cv.Required(CONF_MODEL): cv.string,
      cv.Optional(CONF_CUP_VOLUME, default="250"): cv.int_range(min=100, max=350),
      cv.Optional(CONF_CUP_CORRECT, default="1.534286"): cv.positive_float,
      cv.Required(CONF_SENSORS): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_TEMPERATURE): sensor.sensor_schema(
                accuracy_decimals=0,
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
            cv.Optional(CONF_ENERGY): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_COUNTER,
                state_class=STATE_CLASS_TOTAL_INCREASING,
                unit_of_measurement=UNIT_KILOWATT_HOURS,
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
            cv.Optional(CONF_CUP_QUANTITY): sensor.sensor_schema(
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_EMPTY,
                icon=ICON_COFFEE,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement="cup(s)",
            ),
            cv.Optional(CONF_WATER_VOLUME): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
                icon=ICON_CUP,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement="ml",
            ),
          }
        ),
      ),
      cv.Required(CONF_SWITCHES): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_POWER): switch.SWITCH_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyKettlePowerSwitch),
                cv.Optional(CONF_ICON, default=ICON_KETTLE): switch.icon,
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
  cg.add(var.set_cup_volume(config[CONF_CUP_VOLUME]))
  cg.add(var.set_cup_correct(config[CONF_CUP_CORRECT]))

  params = config[CONF_SENSORS]
  sens = await sensor.new_sensor(params[CONF_TEMPERATURE])
  cg.add(var.set_temperature(sens))
  if CONF_SIGNAL_STRENGTH in params:
    sens = await sensor.new_sensor(params[CONF_SIGNAL_STRENGTH])
    cg.add(var.set_signal_strength(sens))
  if CONF_ENERGY in params:
    sens = await sensor.new_sensor(params[CONF_ENERGY])
    cg.add(var.set_energy(sens))
  if CONF_WORK_CYCLES in params:
    sens = await sensor.new_sensor(params[CONF_WORK_CYCLES])
    cg.add(var.set_work_cycles(sens))
  if CONF_WORK_TIME in params:
    sens = await sensor.new_sensor(params[CONF_WORK_TIME])
    cg.add(var.set_work_time(sens))
  if CONF_CUP_QUANTITY in params:
    sens = await sensor.new_sensor(params[CONF_CUP_QUANTITY])
    cg.add(var.set_cup_quantity(sens))
  if CONF_WATER_VOLUME in params:
    sens = await sensor.new_sensor(params[CONF_WATER_VOLUME])
    cg.add(var.set_water_volume(sens))
  params = config[CONF_SWITCHES]
  conf = params[CONF_POWER]
  sens = cg.new_Pvariable(conf[CONF_ID], var)
  await switch.register_switch(sens, conf)
  cg.add(var.set_power(sens))
    
    
    
    