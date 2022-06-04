### Пояснения к использованию компонента skykettle.
#### Описание
Компонент skykettle предоставляет сервисы чайников REDMOND серии Ready4Sky.  
#### Установка
В конфигурационный файл **yaml** добавить следующие строки:
```yml
skykettle:
  mac_address: F4:AC:78:85:3C:53  
  model: RK-M216S  
  cup_volume: 275
  cup_correction: 1.534286 # correction for RK-M216S
  sensors:
    temperature:
      name: ${kettle} Temperature
    cup_quantity:
      name: ${kettle} Cup(s)
    water_volume: 
      name: ${kettle} Water Volume
    signal_strength:
      name: ${kettle} RSSI 
    work_cycles:
      name: ${kettle} Work Cycles 
    work_time: # (Optional) сенсор наработки (в часах) 
      name: ${kettle} Work Time 
    switches:
      power: # (Required) тумблер включения ) 
        name: ${kettle} Power
```
#### Пояснения к коду	
>**mac_address** *(Required)* Параметр. MAC адрес чайника.  
>**model** *(Required)* Параметр. Наименование модели чайника.  
>**cup_volume** *(Optional, default=250)* Параметр. Задаёт объём воды в кружке.  
>**cup_correction** *(Optional, default=1.0)* Параметр. Коэффициент корректирующий число кружек.  
>  
>**sensors** *(Required) Заголовок группы сенсоров.  
>>**temperature** *(Required) Сенсор текущей температуры воды.  
>>**cup_quantity** *(Optional)* Сенсор числа чашек воды в чайнике.  
>>**water_volume** *(Optional)* Сенсор объёма воды в чайнике (в миллилитрах).  
>>**signal_strength** *(Optional)* Сенсор уровня сигнала от чайника.  
>>**work_cycles** *(Optional)* Сенсор количества включений.  
>>**work_time** *(Optional)* Сенсор наработки (в часах).  
>>>**name** *(Required) Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
 >>>**id** *(Required) Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  
>  
>**switches** *(Required) Заголовок группы переключателей.  
>>**power** *(Optional)* Переключатель вкл./выкл.   
>>>**name** *(Required) Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
 >>>**id** *(Required) Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  

#### Как определить параметр  *cup_correction*? 
- Описывая параметры чайника, не указывайте явно параметр *cup_correction*. В параметре *cup_volume* укажите то количество воды, которое обычно заливается в чашку из чайника при приготовлении напитка;  
- Налейте в чайник мерное количество воды и включите чайник на закипание воды;  
- В логе при нагреве воды примерно за 3 градуса до закипания появится строка вида:  
```yml
[17:56:12][I][SkyKettle:242]: Cup Quantity: 2.723451,  Water Volume: 681
```
- Разделив мерное количество воды на значение *Water Volume* из лога, получите корректирующее значение, которое и укажите в параметре *cup_correction* (не более 6 знаков после запятой).  
- После перепрошивки чайник готов к работе. Удачи!  