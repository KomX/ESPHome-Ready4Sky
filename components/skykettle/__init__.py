import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import(
  ready4sky,
  light,
  number,
  sensor,
  switch,
  text_sensor,
)
from esphome.const import (
  CONF_ACCURACY_DECIMALS,
  CONF_DEFAULT_TRANSITION_LENGTH,
  CONF_ENERGY,
  CONF_ENTITY_CATEGORY,
  CONF_GAMMA_CORRECT,
  CONF_ICON,
  CONF_ID,
  CONF_MAC_ADDRESS,
  CONF_MODE,
  CONF_MODEL,
  CONF_OUTPUT_ID,
  CONF_POWER,
  CONF_SIGNAL_STRENGTH,
  CONF_TEMPERATURE,
  CONF_UNIT_OF_MEASUREMENT,
  DEVICE_CLASS_EMPTY,
  DEVICE_CLASS_ENERGY,
  DEVICE_CLASS_POWER,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  DEVICE_CLASS_TEMPERATURE,
  ENTITY_CATEGORY_CONFIG,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ICON_BLUETOOTH,
  ICON_COUNTER,
  ICON_NEW_BOX,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_CELSIUS,
  UNIT_DECIBEL_MILLIWATT,
  UNIT_EMPTY,
  UNIT_KILOWATT_HOURS,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["switch", "sensor", "number", "text_sensor", "light"]

skykettle_ns = cg.esphome_ns.namespace("skykettle")
SkyKettle = skykettle_ns.class_("SkyKettle", cg.Component, ready4sky.R4SDriver)

SkyKettlePowerSwitch = skykettle_ns.class_("SkyKettlePowerSwitch", switch.Switch)
SkyKettleBackgroundSwitch = skykettle_ns.class_("SkyKettleBackgroundSwitch", switch.Switch)
SkyKettleLockSwitch = skykettle_ns.class_("SkyKettleLockSwitch", switch.Switch)
#SkyKettleBeepSwitch = skykettle_ns.class_("SkyKettleBeepSwitch", switch.Switch)

SkyKettleTargetNumber = skykettle_ns.class_("SkyKettleTargetNumber", number.Number)
SkyKettleBoilTimeAdjNumber = skykettle_ns.class_("SkyKettleBoilTimeAdjNumber", number.Number)

SkyKettleBackgroundLight = skykettle_ns.class_("SkyKettleBackgroundLight", light.LightOutput)

MODEL_TYPE = {
  "RK-G200":   4,
  "RK-G200S":  8,
  "RK-G201S":  8,
  "RK-G202S":  8,
  "RK-G203S":  8,
  "RK-G204S":  8,
  "RK-G210S":  8,
  "RK-G211S":  8,
  "RK-G212S":  8, "RFS-KKL002":  8,
  "RK-G213S":  8,
  "RK-G214S":  8, "RFS-KKL003":  8,
  "RK-G215S":  8,
  "RK-G233S": 16,
  "RK-G240S": 16,
  "RK-M136S": 64,
  "RK-M139S": 64,
  "RK-M170S":  1,
  "RK-M171S":  1,
  "RK-M173S":  2,
  "RK-M215S": 32,
  "RK-M216S": 32,
  "RK-M223S": 32,
}

CONF_CUP_CORRECT = "cup_correction"
CONF_CUP_QUANTITY = "cup_quantity"
CONF_CUP_VOLUME = "cup_volume"
CONF_BACK_LIGHT = "background_light"
CONF_BOIL_TIME_ADJ = "boil_time_adjustment"
CONF_TARGET_TEMP = "target_temperature"
CONF_WATER_VOLUME = "water_volume"
CONF_WORK_CYCLES = "work_cycles"
CONF_WORK_TIME = "work_time"
CONF_STATUS_INDICATOR = "status_indicator"
CONF_STATE_LED = "state_led"
CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
ICON_COFFEE =  "mdi:coffee"
ICON_CUP =  "mdi:cup"
ICON_KETTLE = "mdi:kettle"
ICON_TEMP_WATER = "mdi:thermometer-lines"
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
      cv.Optional(CONF_CUP_VOLUME, default="250"): cv.int_range(min=100, max=500),
      cv.Optional(CONF_CUP_CORRECT, default="1.0"): cv.positive_float,
      cv.Required(CONF_INFORM): cv.All(
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
                unit_of_measurement=UNIT_EMPTY,
            ),
            cv.Optional(CONF_WATER_VOLUME): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
                icon=ICON_CUP,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement="ml",
            ),
            cv.Optional(CONF_STATUS_INDICATOR): text_sensor.TEXT_SENSOR_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(text_sensor.TextSensor),
                cv.Optional(CONF_ICON, default=ICON_NEW_BOX): cv.icon,
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
          }
        ),
      ),
      cv.Required(CONF_CONTROL): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_POWER): switch.SWITCH_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyKettlePowerSwitch),
                cv.Optional(CONF_ICON, default=ICON_KETTLE): switch.icon,
              }
            ),
            cv.Optional(CONF_STATE_LED): switch.SWITCH_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyKettleBackgroundSwitch),
                cv.Optional(CONF_ICON, default="mdi:led-variant-on"): switch.icon,
              }
            ),
            cv.Optional(CONF_TARGET_TEMP): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyKettleTargetNumber),
                cv.Optional(CONF_ICON, default="mdi:thermometer-lines"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
                cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_BOIL_TIME_ADJ): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyKettleBoilTimeAdjNumber),
                cv.Optional(CONF_ICON, default="mdi:timeline-clock"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_EMPTY): cv.string_strict,
                cv.Optional(CONF_MODE, default="AUTO"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_BACK_LIGHT): light.RGB_LIGHT_SCHEMA.extend(
              {
                cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(SkyKettleBackgroundLight),
                cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
                cv.Optional(CONF_DEFAULT_TRANSITION_LENGTH, default="0s"): cv.positive_time_period_milliseconds,
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

  params = config[CONF_INFORM]
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
  if CONF_STATE_LED in params:
    conf = params[CONF_STATE_LED]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_state_led(swtch))
  if CONF_TARGET_TEMP in params:
    numb = await number.new_number(params[CONF_TARGET_TEMP], min_value=35.0, max_value=100.0, step=5.0)
    cg.add(numb.set_parent(var))
    cg.add(var.set_target(numb))
  if CONF_BACK_LIGHT in params:
    lght = cg.new_Pvariable(params[CONF_BACK_LIGHT][CONF_OUTPUT_ID])
    await light.register_light(lght, params[CONF_BACK_LIGHT])
    cg.add(lght.set_parent(var))
    cg.add(var.set_back_light(lght))
    
  if (cv.enum(MODEL_TYPE)(config[CONF_MODEL]) > '7'):
    if CONF_BOIL_TIME_ADJ in params:
      numb = await number.new_number(params[CONF_BOIL_TIME_ADJ], min_value=-5, max_value=5, step=1)
      cg.add(numb.set_parent(var))
      cg.add(var.set_boil_time_adj(numb))