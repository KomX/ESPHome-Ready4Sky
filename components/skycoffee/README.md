### Пояснения к использованию компонента skycoffe.
#### Описание
Компонент skycoffe предоставляет сервисы кофеварок REDMOND серии Ready4Sky.  
Проверена работа со следующими моделями: **RCM-M1509S**, **RCM-M1519S** (**RFS-KCM002**), **RCM-M1525S**.  
Требуется тестирование с логированием трафика обмена с моделями: **RCM-1505S**, **RCM-M1508S**.
#### Установка
К строкам основного компонента ready4sky добавьте в скетч следующие строки:
```yml
substitutions:
  coffee: RCM-M1519S  ### Впишите наименование Вашей модели

skycoffee:
  - mac_address: XX:XX:XX:XX:XX:XX  
    model: RCM-M1519S
    informing:
      signal_strength:
        name: ${coffee} RSSI
# секция для кофеварок RCM-M1519S, RCM-M1525S
# --- start ---
      energy:
        name: ${coffee} Energy
      work_cycles:
        name: ${coffee} Work Cycles
      work_time:
        name: ${coffee} Work Time
# ---- end ----
    controlling:
      power:
        name: ${coffee} Power
      lock:
        name: ${coffee} Lock
      strength:
        name: ${coffee} Strength
# секция для кофеварок RCM-M1509S
# --- start ---
      buzzer:
        name: ${coffee} Buzzer
# ---- end ----
```
#### Пояснения к коду	
>**mac_address** *(Required)* Параметр. MAC адрес.  
>**model** *(Required)* Параметр. Наименование модели.  
>  
>**informing** *(Optional) Заголовок группы сенсоров. Не прописывайте, если ни один сенсор не выбран. 
>>**signal_strength** *(Optional)* Сенсор уровня сигнала.  
>>**energy** *(Optional)* Сенсор потреблённой энергии (kWh).  
>>**work_cycles** *(Optional)* Сенсор количества включений.  
>>**work_time** *(Optional)* Сенсор наработки (h).  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  
>  
>**controlling** *(Required) Заголовок группы переключателей.  
>>**power** *(Optional)* Переключатель вкл./выкл.   
>>**lock** *(Optional)* Переключатель блокировки.  
>>**strength** *(Optional)* Переключатель крепости напитка.  
>>**buzzer** *(Optional)* Переключатель звукового сопровождения.  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  

