import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import(
  ready4sky,
  sensor,
  switch,
  select,
  number,
  text_sensor,
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
  DEVICE_CLASS_ENERGY,
  DEVICE_CLASS_SIGNAL_STRENGTH,
  ENTITY_CATEGORY_CONFIG,
  ENTITY_CATEGORY_DIAGNOSTIC,
  ICON_BLUETOOTH,
  ICON_NEW_BOX,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_CELSIUS,
  UNIT_DECIBEL_MILLIWATT,
  UNIT_EMPTY,
)

CODEOWNERS = ["@KomX"]
DEPENDENCIES = ["ready4sky"]
AUTO_LOAD = ["switch", "sensor", "select", "number", "text_sensor"]

skybaker_ns = cg.esphome_ns.namespace("skybaker")
SkyBaker = skybaker_ns.class_("SkyBaker", cg.Component, ready4sky.R4SDriver)

SkyBakerPowerSwitch = skybaker_ns.class_("SkyBakerPowerSwitch", switch.Switch)
SkyBakerPostHeatSwitch = skybaker_ns.class_("SkyBakerPostHeatSwitch", switch.Switch)
SkyBakerDelaySwitch = skybaker_ns.class_("SkyBakerDelaySwitch", switch.Switch)
SkyBakerModeSelect = skybaker_ns.class_("SkyBakerModeSelect", select.Select)
SkyBakerTimerHoursNumber = skybaker_ns.class_("SkyBakerTimerHoursNumber", number.Number)
SkyBakerTimerMinutesNumber = skybaker_ns.class_("SkyBakerTimerMinutesNumber", number.Number)


CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_BAKING_MODE = "baking_mode"
CONF_LANGUAGE = "language"
CONF_POSTHEATING = "post-heating"
CONF_STATUS_INDICATOR = "status_indicator"
CONF_TIMER_HOURS_SETTING = "timer_hours_setting"
CONF_TIMER_MINUTES_SETTING = "timer_minutes_setting"
CONF_DELAYED_START = "delayed_start"
UNIT_HOUR = "h"
UNIT_MINUTE = "min"

MULTI_CONF = 1

LANGUAGE = {
  "EN":   0,
  "RU":   1,
}

MODEL_TYPE = {
  "RMB-M656/3S":    0,
  "RMB-M657/1S":    0,
  "RMB-M658/3S":    0,
  "RMB-M659/3S":    0,
  "RFS-KMB001":     0,
  "RFS-KMB002":     0,
}

OPTIONS_MODE = [
# 0 All Models
  [
    [
      "Standby Mode",
      "Baking",
      "Heating"
    ],
    [
      "Режим ожидания",
      "Выпекание", 
      "Подогрев"
    ],
  ],
]

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyBaker),
      cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
      cv.Required(CONF_MODEL): cv.string, # cv.enum(MODEL_TYPE, upper=True),
      cv.Optional(CONF_LANGUAGE, default="RU"): cv.enum(LANGUAGE, upper=True),
      cv.Optional(CONF_INFORM): cv.All(
        cv.Schema(
          {
            cv.Optional(CONF_STATUS_INDICATOR): text_sensor.TEXT_SENSOR_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(text_sensor.TextSensor),
                cv.Optional(CONF_ICON, default=ICON_NEW_BOX): cv.icon,
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_SIGNAL_STRENGTH): sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_BLUETOOTH,
                state_class=STATE_CLASS_MEASUREMENT,
                unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
            ),
          }
        ),
      ),
      cv.Required(CONF_CONTROL): cv.All(
        cv.Schema(
          {
            cv.Required(CONF_POWER): switch.switch_schema(
              SkyBakerPowerSwitch,
              icon = "mdi:cookie",
            ),
            cv.Required(CONF_BAKING_MODE): select.SELECT_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(SkyBakerModeSelect),
                }
            ),
            cv.Optional(CONF_POSTHEATING): switch.switch_schema(
              SkyBakerPostHeatSwitch,
              icon = "mdi:heat-wave",
            ),
            cv.Optional(CONF_DELAYED_START): switch.switch_schema(
              SkyBakerDelaySwitch,
              icon = "mdi:progress-clock",
            ),
            cv.Optional(CONF_TIMER_HOURS_SETTING): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyBakerTimerHoursNumber),
                cv.Optional(CONF_ICON, default="mdi:timer-settings"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='1'): cv.int_range(min=0, max=1),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_HOUR): cv.string_strict,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_TIMER_MINUTES_SETTING): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyBakerTimerMinutesNumber),
                cv.Optional(CONF_ICON, default="mdi:timer-settings"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='1'): cv.int_range(min=0, max=1),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_MINUTE): cv.string_strict,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
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
  modl = MODEL_TYPE[config[CONF_MODEL]]
  cg.add(var.set_type(1 << modl))
  lang = LANGUAGE[config[CONF_LANGUAGE]]
  cg.add(var.set_language(lang))
  print(OPTIONS_MODE[modl][lang])
#  omd = OPTIONS_MODE_DATA[modl]
#  for i in range(len(omd)):
#    print(i, omd[i])
#    cg.add(var.set_mode_data(i, omd[i][0], omd[i][1], omd[i][2], omd[i][3] ))
  
  
  

  if CONF_INFORM in config:
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
  conf = params[CONF_BAKING_MODE]
  selct = cg.new_Pvariable(conf[CONF_ID], var)
  await select.register_select(selct, conf, options=OPTIONS_MODE[modl][lang])
  cg.add(var.set_mode(selct))
  if CONF_POSTHEATING in params:
    conf = params[CONF_POSTHEATING]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_postheat(swtch))
  if CONF_TIMER_MINUTES_SETTING in params:
    if CONF_TIMER_HOURS_SETTING in params:
      numb = await number.new_number(params[CONF_TIMER_HOURS_SETTING], min_value=0.0, max_value=11.0, step=1.0)
      cg.add(numb.set_parent(var))
      cg.add(var.set_timer_hours(numb))
  if CONF_TIMER_HOURS_SETTING in params:
    if CONF_TIMER_MINUTES_SETTING in params:
      numb = await number.new_number(params[CONF_TIMER_MINUTES_SETTING], min_value=0.0, max_value=59.0, step=1.0)
      cg.add(numb.set_parent(var))
      cg.add(var.set_timer_minutes(numb))
  if CONF_DELAYED_START in params:
    conf = params[CONF_DELAYED_START]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_delayed_start(swtch))

    