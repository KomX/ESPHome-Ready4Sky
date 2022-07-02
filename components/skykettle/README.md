### Пояснения к использованию компонента skykettle.
#### Описание
Компонент skykettle предоставляет сервисы чайников REDMOND серии Ready4Sky.  
#### Установка
В конфигурационный файл **yaml** добавить следующие строки:
```yml
skykettle:
  - mac_address: F4:AC:78:85:3C:53  
    model: RK-M216S  
    cup_volume: 250
    cup_correction: 1.0
    informing:
      temperature:
        name: ${kettle} Temperature
      cup_quantity:
        name: ${kettle} Cup(s)
      water_volume:
        name: ${kettle} Water Volume
      signal_strength:
        name: ${kettle} RSSI
      energy:
        name: ${kettle} Energy
      work_cycles:
        name: ${kettle} Work Cycles
      work_time:
        name: ${kettle} Work Time
      status_indicator:
        name: ${kettle} Status
    controlling:
      power:
        name: ${kettle} Power
      target_temperature:
        name: ${kettle} Target
      boil_time_adjustment:   ###ВНИМАНИЕ!### Не работает с моделями RK-M17xS и RK-G200.
        name: ${kettle} Boil Adj
```
#### Пояснения к коду	
>**mac_address** *(Required)* Параметр. MAC адрес чайника.  
>**model** *(Required)* Параметр. Наименование модели чайника.  
>**cup_volume** *(Optional, default=250)* Параметр. Задаёт объём воды в кружке.  
>**cup_correction** *(Optional, default=1.0)* Параметр. Коэффициент корректирующий число кружек.  
>  
>**informing** *(Required) Заголовок группы сенсоров.  
>>**temperature** *(Required) Сенсор текущей температуры воды.  
>>**cup_quantity** *(Optional)* Сенсор числа чашек воды в чайнике.  
>>**water_volume** *(Optional)* Сенсор объёма воды в чайнике (ml).  
>>**signal_strength** *(Optional)* Сенсор уровня сигнала от чайника.  
>>**energy** *(Optional)* Сенсор потреблённой энергии (kWh).  
>>**work_cycles** *(Optional)* Сенсор количества включений.  
>>**work_time** *(Optional)* Сенсор наработки (h).  
>>**status_indicator** *(Optional)* Текстовый сенсор режима работы чайника ("Boil", "Heat", "Boil & Heat").  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  
>  
>**controlling** *(Required) Заголовок группы переключателей.  
>>**power** *(Optional)* Переключатель вкл./выкл.   
>>**target_temperature** *(Optional)* Уставка целевой температуры. Выбор 35...90 с шагом 5 для режимов "Heat" и "Boil & Heat", выбор 100 для режима "Boil".  
>>**boil_time_adjustment** *(Optional)* Уставка корректировки времени кипения -5...+5, шаг 1.  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  

#### Как определить параметр  *cup_correction*? 
- Описывая параметры чайника, не указывайте явно параметр *cup_correction* или выставьте явно значение 1.0. В параметре *cup_volume* укажите то количество воды, которое обычно заливается в чашку из чайника при приготовлении напитка;  
- Налейте в чайник мерное количество воды и включите чайник на закипание воды;  
- В логе при нагреве воды примерно за 5 градусов до закипания появится строка вида:  
```yml
[17:56:12][I][SkyKettle:242]: Cup Quantity: 2.723451,  Water Volume: 681
```
- Разделите мерное количество воды на значение *Water Volume* из лога и получите корректирующее значение, которое и укажите в параметре *cup_correction* (не более 6 знаков после запятой).  
- После перепрошивки чайник готов к работе. Удачи!  