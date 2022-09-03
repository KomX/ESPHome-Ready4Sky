### Пояснения к использованию компонента skyiron.
#### Описание
Компонент skyiron предоставляет сервисы утюгов REDMOND серии Ready4Sky.  
#### Установка
В конфигурационный файл **yaml** добавить следующие строки:
```yml
substitutions:
  iron: RI-C273S  ### Впишите наименование Вашей модели утюга

skyiron:
  - mac_address: XX:XX:XX:XX:XX:XX  
    model: RI-C273S  
    informing:
      signal_strength:
        name: ${iron} RSSI
      status_indicator:
        name: ${iron} Status
    controlling:
      power:
        name: ${iron} Power
      safe_mode:
        name: ${iron} Safe Mode
```
#### Пояснения к коду	
>**mac_address** *(Required)* Параметр. MAC адрес утюга.  
>**model** *(Required)* Параметр. Наименование модели утюга.  
>  
>**informing** *(Required) Заголовок группы сенсоров.  
>>**signal_strength** *(Optional)* Сенсор уровня сигнала от утюга.  
>>**status_indicator** *(Optional)* Текстовый сенсор режима работы утюга ("Stand", "Offside", "Upside" и "Forgotten").  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  
>  
>**controlling** *(Required) Заголовок группы переключателей.  
>>**power** *(Required)* Переключатель только Выкл.   
>>**safe_mode** *(Optional)* Переключатель безопасного режима.  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  
  