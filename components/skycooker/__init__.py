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

skycooker_ns = cg.esphome_ns.namespace("skycooker")
SkyCooker = skycooker_ns.class_("SkyCooker", cg.Component, ready4sky.R4SDriver)

SkyCookerPowerSwitch = skycooker_ns.class_("SkyCookerPowerSwitch", switch.Switch)
SkyCookerPostHeatSwitch = skycooker_ns.class_("SkyCookerPostHeatSwitch", switch.Switch)
SkyCookerTimerModeSwitch = skycooker_ns.class_("SkyCookerTimerModeSwitch", switch.Switch)
SkyCookerModeSelect = skycooker_ns.class_("SkyCookerModeSelect", select.Select)
SkyCookerSubModeSelect = skycooker_ns.class_("SkyCookerSubModeSelect", select.Select)
SkyCookerTemperatureNumber = skycooker_ns.class_("SkyCookerTemperatureNumber", number.Number)
SkyCookerTimerHoursNumber = skycooker_ns.class_("SkyCookerTimerHoursNumber", number.Number)
SkyCookerTimerMinutesNumber = skycooker_ns.class_("SkyCookerTimerMinutesNumber", number.Number)

CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_COOKING_MODE = "cooking_mode"
CONF_COOKING_PRODUCT = "cooking_product"
CONF_LANGUAGE = "language"
CONF_POSTHEATING = "post-heating"
CONF_STATUS_INDICATOR = "status_indicator"
CONF_TEMPERATURE_SETTING = "temperature_setting"
CONF_TIMER_HOURS_SETTING = "timer_hours_setting"
CONF_TIMER_MINUTES_SETTING = "timer_minutes_setting"
CONF_TIMER_MODE = "delay/cooking_time"
ICON_COOKER = "mdi:stove"
UNIT_HOUR = "h"
UNIT_MINUTE = "min"

MULTI_CONF = 1

LANGUAGE = {
  "EN":   0,
  "RU":   1,
}

MODEL_TYPE = {
  "RMC-M40S":   3,
  "RMC-M42S":   3,
  "RMC-M92S":   6,  "RMC-M92S-A":   6,  "RMC-M92S-C": 6,  "RMC-M92S-E":   6,
  "RMC-M222S":  7,  "RMC-M222S-A":  7,
  "RMC-M223S":  7,  "RMC-M223S-E":  7,
  "RMC-M224S":  7,  "RFS-KMC001":   7,
  "RMC-M225S":  7,  "RMC-M225S-E":  7,
  "RMC-M226S":  7,  "RMC-M226S-E":  7,  "JK-MC501":   7,  "NK-MC10":  7,  
  "RMC-M227S":  7,
  "RMC-M800S":  0,
  "RMC-M903S":  5,
  "RMC-961S":   4,
  "RMC-CBD100S":1,  
  "RMC-CBF390S":2,
}

OPTIONS_PRODUCT_DATA = [
# 0 RMC-M800S
  [
    [4,18,12,15,0], [5,40,35,60,0], [11,30,25,40,0]
  ],
# 1 RMC-CBD100S
  [
    [2,60,50,40,30], [4,35,30,25,20], [5,40,30,20,18], [6,50,40,20,18], 
    [8,18,15,12,16], [18,18,16,15,13]
  ],
# 2 RMC-CBF390S
  [
    [0,180,90,60,0], [1,18,14,16,16], [4,60,40,20,18], [9,40,30,20,18], 
    [13,20,15,12,13], [18,16,14,10,13], [19,40,30,20,18]
  ],
]

OPTIONS_PRODUCT = [
# 0 RMC-M800S
  [
    [
      "No choice", "Vegetables", "Fish", "Meat"
    ],
    [
      "Нет выбора", "Овощи", "Рыба", "Мясо"
    ],
  ],
# 1 RMC-CBD100S
  [
    [
      "No choice", "Vegetables", "Fish", "Meat", "Bird"
    ],
    [
      "Нет выбора", "Овощи", "Рыба", "Мясо", "Птица"
    ],
  ],
# 2 RMC-CBF390S
  [
    [
      "No choice", "Vegetables", "Fish", "Meat", "Bird"
    ],
    [
      "Нет выбора", "Овощи", "Рыба", "Мясо", "Птица"
    ],
  ],
]

OPTIONS_MODE_DATA = [
# [
#   [temp, hors, minute, bit-flags]
# ],
# bit-flags (uint8_t):
#   B[7] - submode enable             (0x80)
#   B[6] - autopower enable           (0x40)
#   B[5] - expansion of modes enable  (0x20)
#   B[4] - two bowl enable            (0x10)
#   B[3] - preset temperature enable  (0x08)
#   B[2] - masterchef light enable    (0x04)
#   B[1] - delay start enable         (0x02)
#   B[0] - postheat enable            (0x01)

# 0 RMC-M800S
  [ 
    [0,0,0,0],
    [100,0,30,15], [100,0,35,7], [97,3,0,7], [110,1,0,7],
    [180,0,15,133], [100,1,0,135], [100,0,8,5], [95,0,35,7],
    [99,1,0,7], [40,8,0,6], [145,0,45,7], [100,0,40,135],
    [100,0,40,7]
  ],
# 1 RMC-CBD100S
  [
    [0,0,0,0],
    [100,0,30,15], [100,0,30,7], [100,1,0,135], [100,1,30,7],
    [100,0,35,135], [100,0,40,135], [100,0,50,135], [97,3,0,7],
    [170,0,18,133], [145,1,0,7], [150,0,30,7], [110,0,35,7],
    [38,8,0,6], [150,3,0,7], [100,0,8,4], [98,0,15,7],
    [40,0,10,7], [63,2,30,6], [160,0,18,132], [98,0,20,7],
    [100,0,20,64]
  ],
# 2 RMC-CBF390S
  [
    [0,0,0,0],
    [100,3,0,135], [170,0,18,133], [100,0,8,4], [145,1,0,7],
    [100,1,0,135], [38,8,0,6], [100,0,30,15], [40,0,10,7],
    [110,0,35,7], [100,0,40,135], [140,1,0,7], [98,0,20,7],
    [150,3,0,7], [100,0,20,135], [100,0,15,7], [98,0,30,7],
    [97,3,0,7], [100,0,30,4], [160,0,16,132], [100,0,40,135],
    [100,0,30,64], [70,0,0,64]
  ],
# 3 RMC-M4xS
  [
    [0,0,0,0],
    [100,0,30,15], [101,0,30,7], [100,1,0,7], [165,0,18,5],
    [100,1,0,7], [100,0,35,7], [100,0,8,4], [98,3,0,7],
    [100,0,40,7], [140,1,0,7], [100,0,25,7], [110,1,0,7],
    [40,8,0,6], [145,0,20,7], [140,3,0,7], [0,0,0,0], #15-го нет!!!
    [100,1,0,64], [62,2,30,6], [70,0,0,64]
  ],
# 4 RMC-961S
  [
    [0,0,0,0],
    [100,0,10,7], [150,0,15,5], [100,0,25,7], [140,1,0,7],
    [100,1,0,7], [100,0,30,15], [110,1,0,7], [100,1,0,7],
    [100,0,30,7], [38,8,0,6], [100,0,0,64]
  ],
# 5 RMC-M903S
  [
    [0,0,0,0],
    [100,0,30,15], [97,0,10,7], [100,1,0,7], [170,0,15,5],
    [99,1,0,7], [100,0,20,7], [100,0,8,4], [97,5,0,7],
    [100,0,40,7], [145,1,0,7], [100,0,35,7], [110,1,0,7],
    [38,8,0,6], [150,0,25,7], [150,3,0,7], [98,0,20,7],
    [100,0,20,64]
  ],
# 6 RMC-M92S
  [
    [0,0,0,0],
    [100,0,30,15], [97,0,10,7], [100,1,0,7], [170,0,15,5],
    [99,1,0,7], [100,0,20,7], [100,0,8,4], [97,5,0,7],
    [100,0,40,7], [145,1,0,7], [100,0,35,7], [110,1,0,7],
    [38,8,0,6], [150,0,25,7], [150,3,0,7], [98,0,20,7],
    [100,0,0,64], [100,70,30,64]
  ],
# 7 RMC-M22xS
  [
    [0,0,0,0],
    [150,0,15,5], [100,0,25,7], [100,0,30,15], [110,1,0,7],
    [100,0,25,7], [140,1,0,7], [100,1,0,7], [100,1,0,7],
    [100,0,30,7], [40,8,0,6], [100,0,20,64], [70,0,30,64]
  ],
]

OPTIONS_MODE = [
# 0 RMC-M800S
  [
    [
      "Standby Mode",
      "Multi-chef", "Rice/Cereals", "Languor", "Pilaf",
      "Frying", "Stewing", "Pasta", "Milk porridge",
      "Soup", "Yogurt", "Baking", "Steam",
      "Cooking/Legumes"
    ],
    [
      "Режим ожидания",
      "Мультиповар", "Рис/Крупы", "Томление", "Плов",
      "Жарка", "Тушение", "Паста/Макароны", "Молочная каша",
      "Суп", "Йогурт", "Выпечка", "На пару",
      "Варка/Бобовые"
    ],
  ],
# 1 RMC-CBD100S
  [
    [
      "Standby Mode",
      "Multi-chef", "Rice/Cereals", "Soup", "Wildfowl",
      "Steam", "Cooking", "Stewing", "Languor",
      "Frying", "Baking", "Pizza", "Pilaf",
      "Yogurt", "Bread", "Pasta", "Milk porridge",
      "Baby food", "Sous-vide", "Deep frying", "Desserts",
      "Express"
    ],
    [
      "Режим ожидания",
      "Мультиповар", "Рис/Крупы", "Суп", "Дичь",
      "Пар", "Варка", "Тушение", "Томление",
      "Жарка", "Выпечка", "Пицца", "Плов",
      "Йогурт", "Хлеб", "Паста", "Молочная каша",
      "Детское питание", "Вакуум", "Фритюр", "Десерты",
      "Экспресс"
    ],
  ],
# 2 RMC-CBF390S
  [
    [
      "Standby Mode",
      "Galantine", "Frying", "Pasta", "Baking",
      "Stewing", "Yogurt/Dough", "Multi-chef", "Baby food",
      "Pilaf", "Soup", "Cheesecake", "Milk porridge",
      "Bread", "Steam", "Rice/Cereals", "Desserts",
      "Languor", "Sous", "Deep frying", "Cooking",
      "Express", "Warming up"
    ],
    [
      "Режим ожидания",
      "Холодец", "Жарка", "Макароны", "Выпечка",
      "Тушение", "Йогурт/Тесто", "Мультиповар", "Детское питание",
      "Плов", "Суп", "Запеканка/Чизкейк", "Молочная каша",
      "Хлеб", "На пару", "Рис/Крупы", "Десерты",
      "Томление", "Соус", "Фритюр", "Варка",
      "Экспресс", "Разогрев"
    ],
  ],
# 3 RMC-M4xS
  [
    [
      "Standby Mode",
      "Multi-chef", "Milk porridge", "Stewing", "Frying", 
      "Soup", "Steam", "Pasta", "Languor",
      "Cooking", "Baking", "Rice/Cereals", "Pilaf",
      "Yogurt", "Pizza", "Bread", "-----",
      "Express", "Sous-vide", "Warming up"
    ],
    [
      "Режим ожидания",
      "Мультиповар", "Молочная каша", "Тушение", "Жарка", 
      "Суп", "Пар", "Паста", "Томление",
      "Варка", "Выпечка", "Рис/Крупы", "Плов",
      "Йогурт", "Пицца", "Хлеб", "-----",
      "Экспресс", "Вакуум", "Разогрев"
    ],
  ],
# 4 RMC-961S
  [
    [
      "Standby Mode",
      "Rice/Cereals", "Frying", "Steam", "Baking",
      "Stewing", "Multi-chef", "Pilaf", "Soup",
      "Milk porridge", "Yogurt", "Express"
    ],
    [
      "Режим ожидания",
      "Рис/Крупы", "Жарка", "На пару", "Выпечка",
      "Тушение", "Мультиповар", "Плов", "Суп",
      "Молочная каша", "Йогурт", "Экспресс"
    ],
  ],
# 5 RMC-M903S
  [
    [
      "Standby Mode",
      "Multi-chef", "Milk porridge", "Stewing", "Frying",
      "Soup", "Steam", "Pasta", "Languor", 
      "Cooking", "Baking", "Rice/Cereals", "Pilaf",
      "Yogurt", "Pizza", "Bread", "Desserts",
      "Express"
    ],
    [
      "Режим ожидания",
      "Мультиповар", "Молочная каша", "Тушение",  "Жарка",
      "Суп", "На пару", "Макароны", "Томление", 
      "Варка", "Выпечка", "Крупы", "Плов",
      "Йогурт", "Пицца", "Хлеб", "Десерты",
      "Экспресс"
    ],
  ],
# 6 RMC-M92S
  [
    [
      "Standby Mode",
      "Multi-chef", "Milk porridge", "Stewing", "Frying",
      "Soup", "Steam", "Pasta", "Languor", 
      "Cooking", "Baking", "Rice/Cereals", "Pilaf",
      "Yogurt", "Pizza", "Bread", "Desserts",
      "Express", "Warming"
    ],
    [
      "Режим ожидания",
      "Мультиповар", "Молочная каша", "Тушение", "Жарка",
      "Суп", "На пару", "Макароны", "Томление", 
      "Варка", "Выпечка", "Крупы", "Плов",
      "Йогурт", "Пицца", "Хлеб", "Десерты",
      "Экспресс", "Подогрев"
    ],
  ],
# 7 RMC-M22xS
  [
    [
      "Standby Mode",
      "Frying", "Rice/Cereals", "Multi-chef", "Pilaf",
      "Steam", "Baking", "Stewing", "Soup",
      "Milk porridge", "Yogurt", "Express", "Warming up"
    ],
    [
      "Режим ожидания",
      "Жарка", "Рис/Крупы", "Мультиповар", "Плов",
      "На пару", "Выпечка", "Тушение", "Суп",
      "Молочная каша", "Йогурт", "Экспресс", "Разогрев"
    ],
  ],
]

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyCooker),
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
              SkyCookerPowerSwitch,
              icon = ICON_COOKER,
            ),
            cv.Required(CONF_COOKING_MODE): select.SELECT_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(SkyCookerModeSelect),
                }
            ),
            cv.Optional(CONF_POSTHEATING): switch.switch_schema(
              SkyCookerPostHeatSwitch,
              icon = "mdi:heat-wave",
            ),
            cv.Optional(CONF_TIMER_MODE): switch.switch_schema(
              SkyCookerTimerModeSwitch,
              icon = "mdi:progress-clock",
            ),
            cv.Optional(CONF_COOKING_PRODUCT): select.SELECT_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(SkyCookerSubModeSelect),
                }
            ),
            cv.Optional(CONF_TEMPERATURE_SETTING): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyCookerTemperatureNumber),
                cv.Optional(CONF_ICON, default="mdi:thermometer-lines"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_TIMER_HOURS_SETTING): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyCookerTimerHoursNumber),
                cv.Optional(CONF_ICON, default="mdi:timer-settings"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_HOUR): cv.string_strict,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_TIMER_MINUTES_SETTING): number.NUMBER_SCHEMA.extend(
              {
                cv.GenerateID(): cv.declare_id(SkyCookerTimerMinutesNumber),
                cv.Optional(CONF_ICON, default="mdi:timer-settings"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
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
  lang = LANGUAGE[config[CONF_LANGUAGE]]
  omd = OPTIONS_MODE_DATA[modl]
  print(OPTIONS_MODE[modl][lang])
  for i in range(len(omd)):
    print(i, omd[i])
    cg.add(var.set_mode_data(i, omd[i][0], omd[i][1], omd[i][2], omd[i][3] ))
  
  cg.add(var.set_type(1 << modl))
  cg.add(var.set_language(lang))

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
  conf = params[CONF_COOKING_MODE]
  selct = cg.new_Pvariable(conf[CONF_ID], var)
  await select.register_select(selct, conf, options=OPTIONS_MODE[modl][lang])
  cg.add(var.set_mode(selct))
  if modl < 3:
    if CONF_COOKING_PRODUCT in params:
      conf = params[CONF_COOKING_PRODUCT]
      selct = cg.new_Pvariable(conf[CONF_ID], var)
      await select.register_select(selct, conf, options=OPTIONS_PRODUCT[modl][lang])
      cg.add(var.set_submode(selct))
      osmd = OPTIONS_PRODUCT_DATA[modl]
      print(OPTIONS_PRODUCT[modl][lang])
      for i in range(len(osmd)):
        print(i, osmd[i])
        cg.add(var.set_submode_data(i, osmd[i][0], osmd[i][1], osmd[i][2], osmd[i][3], osmd[i][4]))
  if CONF_POSTHEATING in params:
    conf = params[CONF_POSTHEATING]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_postheat(swtch))
  if CONF_TEMPERATURE_SETTING in params:
    numb = await number.new_number(params[CONF_TEMPERATURE_SETTING], min_value=35.0, max_value=180.0, step=1.0)
    cg.add(numb.set_parent(var))
    cg.add(var.set_temperature(numb))
  if CONF_TIMER_MINUTES_SETTING in params:
    if CONF_TIMER_HOURS_SETTING in params:
      numb = await number.new_number(params[CONF_TIMER_HOURS_SETTING], min_value=0.0, max_value=23.0, step=1.0)
      cg.add(numb.set_parent(var))
      cg.add(var.set_timer_hours(numb))
  if CONF_TIMER_HOURS_SETTING in params:
    if CONF_TIMER_MINUTES_SETTING in params:
      numb = await number.new_number(params[CONF_TIMER_MINUTES_SETTING], min_value=0.0, max_value=59.0, step=1.0)
      cg.add(numb.set_parent(var))
      cg.add(var.set_timer_minutes(numb))
  if CONF_TIMER_MODE in params:
    conf = params[CONF_TIMER_MODE]
    swtch = cg.new_Pvariable(conf[CONF_ID], var)
    await switch.register_switch(swtch, conf)
    cg.add(var.set_timer_mode(swtch))

    