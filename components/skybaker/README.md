 ### Пояснения к использованию компонента skybaker.

#### Установка
К строкам основного компонента ready4sky добавьте в скетч следующие строки:

```yml
substitutions:
  baker: "RMB-M656/3S"  ### Впишите наименование Вашей модели

skybaker:
  - mac_address: XX:XX:XX:XX:XX:XX  
    model: "RMB-M656/3S"
    language: EN    # Выберите из EN и RU. RU по умолчанию.
    informing:
      status_indicator:
        name: ${baker} Status
      signal_strength:
        name: ${baker} RSSI
    controlling:
      power:
        name: ${baker} Power
      baking_mode:
        name: ${baker} Cooking mode
      post-heating:
        name: ${baker} Post Heating
      delayed_start:
        name: ${baker} Delayed Start
      timer_hours_setting:
        name: ${baker} Time (Hours)
      timer_minutes_setting:
        name: ${baker} Time (Minutes)
```

#### Пояснения к коду	
>**mac_address** *(Required)* Параметр. MAC адрес изделия.  
>**model** *(Required)* Параметр. Наименование модели изделия.  
>  
>**informing** *(Optional)* Заголовок группы сенсоров. Не прописывайте, если ни один сенсор не выбран.  
>>**signal_strength** *(Optional)* Сенсор уровня сигнала.  
>>**work_cycles** *(Optional)* Сенсор количества включений.  
>>**work_time** *(Optional)* Сенсор наработки (h).  
>  
>**controlling** *(Required)* Заголовок группы органов управления.  
>>**power** *(Required)* Переключатель вкл./выкл.   
>>**target_power** *(Optional)* Уставка мощности обогрева 0...100, шаг 25.  
>>**lock** *(Optional)* Переключатель блоктровки органов управления.  
>>**remember_state** *(Optional)* Переключатель восстановления состояния при отключении электричества.  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча. 

