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

skyoven_ns = cg.esphome_ns.namespace("skyoven")
SkyOven = skyoven_ns.class_("SkyOven", cg.Component, ready4sky.R4SDriver)

SkyOvenPowerSwitch = skyoven_ns.class_("SkyOvenPowerSwitch", switch.Switch)
SkyOvenPostHeatSwitch = skyoven_ns.class_("SkyOvenPostHeatSwitch", switch.Switch)
SkyOvenTimerModeSwitch = skyoven_ns.class_("SkyOvenTimerModeSwitch", switch.Switch)
SkyOvenGrillSwitch = skyoven_ns.class_("SkyOvenGrillSwitch", switch.Switch)
SkyOvenConvectionSwitch = skyoven_ns.class_("SkyOvenConvectionSwitch", switch.Switch)
SkyOvenModeSelect = skyoven_ns.class_("SkyOvenModeSelect", select.Select)
SkyOvenSubModeSelect = skyoven_ns.class_("SkyOvenSubModeSelect", select.Select)
SkyOvenTemperatureNumber = skyoven_ns.class_("SkyOvenTemperatureNumber", number.Number)
SkyOvenTimerHoursNumber = skyoven_ns.class_("SkyOvenTimerHoursNumber", number.Number)
SkyOvenTimerMinutesNumber = skyoven_ns.class_("SkyOvenTimerMinutesNumber", number.Number)

CONF_INFORM = "informing"
CONF_CONTROL = "controlling"
CONF_OVEN_MODE = "oven_mode"
CONF_OVEN_PRODUCT = "oven_product"
CONF_LANGUAGE = "language"
CONF_POSTHEATING = "post-heating"
CONF_STATUS_INDICATOR = "status_indicator"
CONF_TEMPERATURE_SETTING = "temperature_setting"
CONF_TIMER_HOURS_SETTING = "timer_hours_setting"
CONF_TIMER_MINUTES_SETTING = "timer_minutes_setting"
CONF_TIMER_MODE = "delay/oven_time"
ICON_OVEN = "mdi:toaster-oven"
UNIT_HOUR = "h"
UNIT_MINUTE = "min"

MULTI_CONF = 1

LANGUAGE = {
  "EN":   0,
  "RU":   1,
  "UA":   2,
  "BY":   3,
  "KZ":   4,
}

MODEL_TYPE = {
  "RO-5706S":1,
  "RO-5707S":0, "RO-5717S":0, "RO-5727S":0,
}

OPTIONS_PRODUCT_DATA = [
# 0 RO-57x7S
  [
    [0,180,90,60,0], [1,18,14,16,16], [4,60,40,20,18], [9,40,30,20,18], 
    [13,20,15,12,13], [18,16,14,10,13], [19,40,30,20,18]
  ],
]

OPTIONS_PRODUCT = [
# 0 RO-57x7S
  [
    ["No choice", "Vegetables", "Fish", "Bird", "Meat"],
    ["Нет выбора", "Овощи", "Рыба", "Птица", "Мясо"],
    ["Немає вибору", "Овочі", "Риба", "Птиця", "М'ясо"],
    ["Няма выбару", "Гародніна", "Рыба", "Птушка", "Мяса"],
    ["Таңдау жоқ", "Көкөністер", "Балықты", "Құс етін", "Еттi"],
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

# 0 RO-57x7S
  [ 
    [0,0,0,0],
    [100,0,30,15], [100,0,35,7], [97,3,0,7], [110,1,0,7],
    [180,0,15,133], [100,1,0,135], [100,0,8,5], [95,0,35,7],
    [99,1,0,7], [40,8,0,6], [145,0,45,7], [100,0,40,135],
    [100,0,40,7]
  ],
# 1 RO-5706S
  [
    [0,0,0,0],
    [100,0,30,15], [100,0,30,7], [100,1,0,135], [100,1,30,7],
    [100,0,35,135], [100,0,40,135], [100,0,50,135], [97,3,0,7],
    [170,0,18,133], [145,1,0,7], [150,0,30,7], [110,0,35,7],
    [38,8,0,6], [150,3,0,7], [100,0,8,4], [98,0,15,7],
    [40,0,10,7], [63,2,30,6], [160,0,18,132], [98,0,20,7],
    [100,0,20,64]
  ],
]

OPTIONS_MODE = [
# 0 RO-57x7S
  [
    [
      "Standby Mode",
      "Multi-chef", "Omelette", "Bread", "Charlotte",
      "Bubble and Squeak", "Roasting", "Languor", "Baking in pots",
      "Pizza", "Cake", "Bouzhenina"
    ],
    [
      "Режим ожидания",
      "Мультиповар", "Омлет", "Хлеб", "Шарлотка",
      "Жаркое", "Запекание", "Томление", "Запекание в горшочках",
      "Пицца", "Кекс", "Буженина"
    ],
    [
      "Режим очікування",
      "Мульти кухар", "Омлет", "Хліб", "Шарлотка",
      "Печеня", "Запікання", "Тушкування", "Запікання в горщиках",
      "Піца", "Кекс", "Буженина"
    ],
    [
      "Рэжым чакання",
      "Мульты кухар", "Амлет", "Хлеб", "Шарлотка",
      "Смажаніна", "Запяканне", "Томление", "Запяканне ў збанках",
      "Піца", "Кекс", "Бужаніна"
    ],
    [
      "Күту режимі",
      "Көп аспаз", "Омлет", "Нан", "Шарлотка",
      "Қуырдақ", "Қыздырып пісіру", "Жасыту", "Құмыраларға қыздырып пісіру",
      "Пицца", "Торт", "Қақтама"
    ],
  ],
# 1 RO-5706S
  [
    [
      "Standby Mode",
      "Multi-chef", "Roasting meat", "Baking fish", "Poultry Roasting",
      "Roasting vegetables", "Languor", "White bread", "Rye bread",
      "Omelette", "Baking in pots", "Bubble and Squeak", "Bouzhenina",
      "Grilled chicken", "French baguette", "Ciabatta", "Cheesecake"
    ],
    [
      "Режим ожидания",
      "Мультиповар", "Запекание мяса", "Запекание рыбы", "Запекание птицы",
      "Запекание овощей", "Томление", "Хлеб белый", "Хлеб ржаной",
      "Омлет", "Запекание в горшочках", "Жаркое", "Буженина",
      "Курица-гриль", "Французский багет", "Чиабата", "Чизкейк"
    ],
    [
      "Режим очікування",
      "Мульти кухар", "Запікання м'яса", "Запікання риби", "Запікання птиці",
      "Запікання овочів", "Тушкування", "Хліб білий", "Житній хліб",
      "Омлет", "Запікання в горщиках", "Печеня", "Буженина",
      "Курка-гриль", "Французький багет", "Чіабата", "Чізкейк"
    ],
    [
      "Рэжым чакання",
      "Мульты кухар", "Запяканне мяса", "Запяканне рыбы", "Запяканне птушкi",
      "Запяканне гародніны", "Знямога", "Хлеб белы", "Хлеб жытні",
      "Амлет", "Запяканне ў збанках", "Смажаніна", "Бужаніна",
      "Курыца-грыль", "Французскі багет", "Чiабата", "Чiзкейк"
    ],
    [
      "Күту режимі",
      "Көп аспаз", "Етті қыздырып пісіру", "Балықты қыздырып пісір", "Құс етін қыздырып пісір",
      "Көкөністі қыздырып пісіру", "Жасыту", "Ақ нан", "Қарабидай наны",
      "Омлет", "Құмыраларға қыздырып пісіру", "Қуырдақ", "Қақтама",
      "Тауық-гриль", "Француз багеті", "Чиабата", "Чизкейк"
    ],
  ],
]

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(SkyOven),
      cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
      cv.Required(CONF_MODEL): cv.string, # cv.enum(MODEL_TYPE, upper=True),
      cv.Optional(CONF_LANGUAGE, default="EN"): cv.enum(LANGUAGE, upper=True),
      cv.Optional(CONF_INFORM): cv.All(
        cv.Schema(
          {
            cv.Optional(CONF_STATUS_INDICATOR): text_sensor.text_sensor_schema().extend(
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
              SkyOvenPowerSwitch,
              icon = ICON_OVEN,
            ),
            cv.Required(CONF_OVEN_MODE): select.select_schema(SkyOvenModeSelect),
            cv.Optional(CONF_POSTHEATING): switch.switch_schema(
              SkyOvenPostHeatSwitch,
              icon = "mdi:heat-wave",
            ),
            cv.Optional(CONF_TIMER_MODE): switch.switch_schema(
              SkyOvenTimerModeSwitch,
              icon = "mdi:progress-clock",
            ),
            cv.Optional(CONF_OVEN_PRODUCT): select.select_schema(SkyOvenSubModeSelect),
            cv.Optional(CONF_TEMPERATURE_SETTING): number.number_schema(SkyOvenTemperatureNumber).extend(
              {
                cv.Optional(CONF_ICON, default="mdi:thermometer-lines"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_TIMER_HOURS_SETTING): number.number_schema(SkyOvenTimerHoursNumber).extend(
              {
                cv.Optional(CONF_ICON, default="mdi:timer-settings"): cv.icon,
                cv.Optional(CONF_ACCURACY_DECIMALS, default='0'): cv.int_range(min=0, max=2),
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_HOUR): cv.string_strict,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
              }
            ),
            cv.Optional(CONF_TIMER_MINUTES_SETTING): number.number_schema(SkyOvenTimerMinutesNumber).extend(
              {
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
  conf = params[CONF_OVEN_MODE]
  selct = cg.new_Pvariable(conf[CONF_ID], var)
  await select.register_select(selct, conf, options=OPTIONS_MODE[modl][lang])
  cg.add(var.set_mode(selct))
  if modl < 3:
    if CONF_OVEN_PRODUCT in params:
      conf = params[CONF_OVEN_PRODUCT]
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

    
