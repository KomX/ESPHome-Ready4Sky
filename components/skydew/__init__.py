import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.log import color, Fore
from esphome.util import safe_print
from esphome.components import(
  ready4sky,
  switch,
  sensor,
  number,
  text_sensor,
  select,
)

from esphome.const import (
  CONF_ACCURACY_DECIMALS,
  CONF_AUTO_MODE,
  CONF_BEEPER,
  CONF_ENERGY,
  CONF_ENTITY_CATEGORY,
  CONF_HUMIDITY,
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
  DEVICE_CLASS_HUMIDITY,
  DEVICE_CLASS_POWER,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  DEVICE_CLASS_TEMPERATURE,
  ENTITY_CATEGORY_CONFIG,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ENTITY_CATEGORY_NONE,
  ICON_BLUETOOTH,
  ICON_COUNTER,
  ICON_NEW_BOX,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_CELSIUS,
  UNIT_DECIBEL_MILLIWATT,
  UNIT_EMPTY,
  UNIT_KILOWATT_HOURS,
  UNIT_PERCENT,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["switch", "sensor", "number", "text_sensor", "select"]

skydew_ns = cg.esphome_ns.namespace("skydew")
SkyDew = skydew_ns.class_("SkyDew", cg.Component, ready4sky.R4SDriver)

SkyDewPowerSwitch = skydew_ns.class_("SkyDewPowerSwitch", switch.Switch)
SkyDewBeeperSwitch = skydew_ns.class_("SkyDewBeeperSwitch", switch.Switch)
SkyDewAutoModeSwitch = skydew_ns.class_("SkyDewAutoModeSwitch", switch.Switch)
SkyDewNightModeSwitch = skydew_ns.class_("SkyDewNightModeSwitch", switch.Switch)
SkyDewWarmSteamSwitch = skydew_ns.class_("SkyDewWarmSteamSwitch", switch.Switch)

SkyDewTargetHumidityNumber = skydew_ns.class_("SkyDewTargetHumidityNumber", number.Number)
SkyDewSteamLevelNumber = skydew_ns.class_("SkyDewSteamLevelNumber", number.Number)
SkyDewTimerHoursNumber = skydew_ns.class_("SkyDewTimerHoursNumber", number.Number)
SkyDewTimerMinutesNumber = skydew_ns.class_("SkyDewTimerMinutesNumber", number.Number)

SkyDewModeSelect = skydew_ns.class_("SkyDewModeSelect", select.Select)

MODEL_TYPE = {
  "RHF-3310S":  1,
  "RHF-3317S":  2,
  "RHF-3318S":  2,
  "RHF-3320S":  4,
}

CONF_DEW_MODE = "mode"
CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_STEAM_INTENSITY = "steam_intensity"
CONF_LANGUAGE = "language"
CONF_NIGHT_MODE = "night_mode"
CONF_STATUS_INDICATOR = "status_indicator"
CONF_STEAM_LEVEL = "steam_level"
CONF_WARM_STEAM = "warm_steam"
CONF_TARGET_HUMIDITY = "target_humidity"
CONF_TIMER_HOURS_SETTING = "timer_hours_setting"
CONF_TIMER_MINUTES_SETTING = "timer_minutes_setting"
CONF_WORK_CYCLES = "work_cycles"
CONF_WORK_TIME = "work_time"
ICON_DEW = "mdi:air-humidifier"
ICON_WORK_CYCLES = "mdi:calendar-refresh"
ICON_WORK_TIME = "mdi:calendar-clock"
UNIT_HOUR = "h"
UNIT_MINUTE = "min"

MULTI_CONF = 2

LANGUAGE = {
  "EN":   0,
  "RU":   1,
}

OPTIONS_DEW_MODE = [
  [
    "Normal", "Sleep", "Turbo", "Auto"
  ],
  [
    "Норма", "Сон", "Турбо", "Авто"
  ],
]

def validate_control_parameters(config):
  mdlnm = config[CONF_MODEL]
  mdl = MODEL_TYPE[mdlnm]
  prmc = config[CONF_CONTROL]
  prmi = config[CONF_INFORM]
  npr = ""
  if mdl == 1:
    if CONF_AUTO_MODE in prmc:
      npr = CONF_AUTO_MODE
      print(npr)
#    if CONF_STEAM_LEVEL in prmc:
#      npr = CONF_STEAM_LEVEL
    if CONF_NIGHT_MODE in prmc:
      npr = CONF_NIGHT_MODE
    if CONF_TEMPERATURE in prmi:
      npr = CONF_TEMPERATURE
  elif mdl == 2:
    if CONF_WARM_STEAM in prms:
      npr = CONF_WARM_STEAM
    if CONF_DEW_MODE in prms:
      npr = CONF_DEW_MODE
    
  if not npr == "":
    raise cv.Invalid(
      f"The [{npr}] parameter must not be present in this model {mdlnm}"
    )
  return config

CONFIG_SCHEMA = cv.All(
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyDew),
      cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
      cv.Required(CONF_MODEL): cv.string,
      cv.Optional(CONF_LANGUAGE, default="RU"): cv.enum(LANGUAGE, upper=True),
      cv.Required(CONF_INFORM): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_HUMIDITY): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement=UNIT_PERCENT,
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
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
                unit_of_measurement=UNIT_HOUR,
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
            cv.Required(CONF_POWER): switch.switch_schema(
              SkyDewPowerSwitch,
              icon = ICON_DEW,
            ),
            cv.Optional(CONF_AUTO_MODE): switch.switch_schema(
              SkyDewAutoModeSwitch,
              icon = "mdi:auto-mode",
            ),
            cv.Optional(CONF_NIGHT_MODE): switch.switch_schema(
              SkyDewNightModeSwitch,
              icon = "mdi:weather-night",
            ),
            cv.Optional(CONF_BEEPER): switch.switch_schema(
              SkyDewBeeperSwitch,
              icon = "mdi:volume-high",
            ),
            cv.Optional(CONF_WARM_STEAM): switch.switch_schema(
              SkyDewWarmSteamSwitch,
              icon = "mdi:heat-wave",
            ),
            cv.Optional(CONF_TARGET_HUMIDITY): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyDewTargetHumidityNumber),
                cv.Optional(CONF_ICON, default="mdi:cloud-percent"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_PERCENT): cv.string_strict,
                cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_STEAM_LEVEL): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyDewSteamLevelNumber),
                cv.Optional(CONF_ICON, default="mdi:soundcloud"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_EMPTY): cv.string_strict,
                cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_DEW_MODE): select.SELECT_SCHEMA.extend(
              {
                  cv.GenerateID(): cv.declare_id(SkyDewModeSelect),
              }
            ),
          }
        ),
      ), 
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
  .extend(ready4sky.READY4SKY_DEVICE_SCHEMA),
  validate_control_parameters,
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await ready4sky.register_r4s_driver(var, config)
  cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
  cg.add(var.set_model(config[CONF_MODEL]))
  modl = MODEL_TYPE[config[CONF_MODEL]]
  lang = LANGUAGE[config[CONF_LANGUAGE]]
  omd = OPTIONS_DEW_MODE[lang]
  cg.add(var.set_type(modl))
  cg.add(var.set_language(lang))
  
  params = config[CONF_INFORM]
  sens = await sensor.new_sensor(params[CONF_HUMIDITY])
  cg.add(var.set_humidity(sens))
  if CONF_TEMPERATURE in params:
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
  if CONF_AUTO_MODE in params:
    conf = params[CONF_AUTO_MODE]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_auto_mode(swtch))
  if CONF_NIGHT_MODE in params:
    conf = params[CONF_NIGHT_MODE]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_night_mode(swtch))
  if CONF_BEEPER in params:
    conf = params[CONF_BEEPER]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_beeper(swtch))
  if CONF_TARGET_HUMIDITY in params:
    if modl == 1:
      numb = await number.new_number(params[CONF_TARGET_HUMIDITY], min_value=35.0, max_value=70.0, step=5.0)
    else:
      numb = await number.new_number(params[CONF_TARGET_HUMIDITY], min_value=40.0, max_value=80.0, step=5.0)
    cg.add(numb.set_parent(var))
    cg.add(var.set_target_humidity(numb))
  if CONF_STEAM_LEVEL in params:
    numb = await number.new_number(params[CONF_STEAM_LEVEL], min_value=1.0, max_value=4.0, step=1.0)
    cg.add(numb.set_parent(var))
    cg.add(var.set_steam_level(numb))
  if CONF_WARM_STEAM in params:
    conf = params[CONF_WARM_STEAM]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_warm_steam(swtch))
  if CONF_DEW_MODE in params:
    conf = params[CONF_DEW_MODE]
    selct = cg.new_Pvariable(conf[CONF_ID], var)
    await select.register_select(selct, conf, options=omd)
    cg.add(var.set_mode(selct))