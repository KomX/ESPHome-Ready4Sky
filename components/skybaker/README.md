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
>**language** *(Optional, default=RU)* Параметр. Задаёт язык списка меню (работ) и статусной строки.  
>  
>**informing** *(Optional)* Заголовок группы сенсоров. Не прописывайте, если ни один сенсор не выбран.  
>>**status_indicator** *(Optional)* Текстовый сенсор режима работы мультиварки.  
>>**signal_strength** *(Optional)* Сенсор уровня сигнала.  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  
>  
>**controlling** *(Required)* Заголовок группы органов управления.  
>>**power** *(Required)* Переключатель вкл./выкл.   
>>**baking_mode** *(Required)* Селектор выбора режима работы.  
>>**post-heating** *(Optional)* Переключатель подогрева после окончания готовки.  
>>**delayed_start** *(Optional)* Переключатель отложенного старта.  
>>**timer_hours_setting** *(Optional)* Установка и отображение времени готовки или отложенного старта в часах.  
>>**timer_minutes_setting** *(Optional)* Установка и отображение времени готовки или отложенного старта в минутах.  
>>>**name** *(Required)* Имя сущности в HA. Указывается для всех сущностей объявляемых для HA.  
>>>**id** *(Optional)* Идентификатор сущности для ESPHome. Указывается для использования сущности внутри скетча.  

#### Работа с интерфейсом. 
Активность того или иного органа управления зависит от выставленного значания более важного по значимости органа управления.   
Поэтому, придерживайтесь следующей последовательности действий:   
- Выберите блюдо или вид работы;  
- Подкорректируйте время готовки;  
- Переключитесь на режим установки времени отложенного старта и выставьте время.  
- Включите готовку. Приятного аппетита!  

