### Пояснения к использованию компонента ready4sky.
#### Описание
Компонент ready4sky призван заменить собой стандартный набор компонент для работы с BLE устройствами.  
**Внимание!** Работа компонента ограничена только устройствами Redmond.
#### Установка
В конфигурационный файл **yaml** добавить следующие строки:
```yml
external_components:
  - source: github://KomX/esphome/components
time:
  platform: homeassistant 
ready4sky:
  scan_parameters:
    monitor: false
    interval: 150 ms 
    window: 50 ms
    duration: 30 s
    active: true
```
##### Пояснения к коду
>**scan_parameters** *(Optional)* Заголовок блока параметров. Отсутствие заголовка выставит параметры по умолчанию.
>>**monitor** *(Optional, default=false)* Параметр. В активном состоянии производится поиск устройств.  
>>**interval** *(Optional, default=150ms)* Параметр. Задаёт интервал сканирования.  
>>**window** *(Optional, default=50ms)* Параметр. Задаёт окно сканирования.  
>>**duration** *(Optional, default=30s)* Параметр. Задаёт продолжительность сканирования.  
>>**active** *(Optional, default=true)* Параметр. Задаёт режим сканирования активный/парссивный.  

Результаты мониторинга используются в компонентах ***SkyKettle***, ***SkyIron*** и т.д.