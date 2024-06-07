### Пояснения к использованию компонента skyplug.
#### Описание
Компонент skyplug предоставляет сервисы розеток REDMOND серии Ready4Sky.  
Проверена работа со следующими моделями: **RSP-100S**, **RSP-103S**.
#### Установка
К строкам основного компонента ready4sky добавьте в скетч следующие строки:
```yml
substitutions:
  plug: RSP-103S  ### Впишите наименование Вашей модели

skyplug:
  - mac_address: XX:XX:XX:XX:XX:XX  
    model: RSP-103S  
    informing:
      signal_strength:
        name: ${plug} RSSI
      work_cycles:
        name: ${plug} Work Cycles
      work_time:
        name: ${plug} Work Time
    controlling:
      power:
        name: ${plug} Power
      lock:
        name: ${plug} Lock
      remember_state:
        name: ${plug} Remember State
```
#### Пояснения к коду	
>**mac_address** *(Required)* Параметр. MAC адрес.  
>**model** *(Required)* Параметр. Наименование модели.  
>  
>**informing** *(Optional) Заголовок группы сенсоров. Не прописывайте, если ни один сенсор не выбран. 
>>**signal_strength** *(Optional)* Сенсор уровня сигнала.  
>>**work_cycles** *(Optional)* Сенсор количества включений.  
>>**work_time** *(Optional)* Сенсор наработки (h).  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  
>  
>**controlling** *(Required) Заголовок группы переключателей.  
>>**power** *(Optional)* Переключатель вкл./выкл.   
>>**lock** *(Optional)* Переключатель блокировки.  
>>**remember_state** *(Optional)* Переключатель восстановления состояния после аварийноо отключении электричества.  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  

