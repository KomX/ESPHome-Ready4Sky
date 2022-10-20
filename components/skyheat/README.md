### Пояснения к использованию компонента skyheat.
#### Описание
Компонент skyheat предоставляет сервисы мультварок REDMOND серии Ready4Sky.  
Проверена работа со следующими моделями:  
**RCH-700xS**.  
Требуется проверка корректности работы компонента со следующими моделями:  
**RCH-452xS**, **RCH-4530S**, **RCH-455xS**.  
Требуют тестирования (логи трафика обмена) модели:  
**RCH-4560S**, **RFH-С4519S**, **RFH-С4522S**.
#### Установка
К строкам основного компонента ready4sky добавьте в скетч следующие строки:

**Для изделий RCH-452xS, RCH-4530S, RFH-455xS, RCH-455xS:**
```yml
substitutions:
  heat: RCH-4530S  ### Впишите наименование Вашей модели

skyheat:
  - mac_address: XX:XX:XX:XX:XX:XX
    model: RCH-4530S
    informing:
      signal_strength:
        name: ${heat} RSSI
      work_cycles:
        name: ${heat} Work Cycles
      work_time:
        name: ${heat} Work Time
    controlling:
      power:
        name: ${heat} Power
      lock:
        name: ${heat} Lock
      remember_mode:
        name: ${heat} Remember Mode
```

**Для изделий RCH-700xS:**
```yml
substitutions:
  heat: RCH-7001S  ### Впишите наименование Вашей модели

skyheat:
  - mac_address: XX:XX:XX:XX:XX:XX  
    model: RCH-7001S
    informing:
      signal_strength:
        name: ${heat} RSSI
    controlling:
      power:
        name: ${heat} Power
      target_power:
        name: ${heat} Target Power 
      lock:
        name: ${heat} Lock
      remember_mode:
        name: ${heat} Remember Mode
```

**Для изделий RCH-4560S, RFH-С4519S, RFH-С4522S:**  
Описание параметров для данных моделей будет добавлено после получения достоверной информации из логов изделий.  


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
