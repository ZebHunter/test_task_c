# Device Mapper Proxy
Модуль разрабатывался на Ubuntu 20.04 LTS (достаточно старая), с версией ядра 5.15.0-107-generic и gcc 9.4.0

## Сборка и установка

Для сборки использовать

`make`

Для загрузки модуля в ядро:

`make load`

Для выгрузки модуля:

`make unload`

Для очистки:

`make clean`

## Тестирование

- Создание тестового блочного устройства

`dmsetup create zero1 --table "0 512 zero"`

- Создание device mapper proxy

`dmsetup create dmp1 --table "0 512 dmp /dev/mapper/zero1"`

- Проверить создание можно с помощью команды 

`ls -al /dev/mapper/*`

```bash
lrwxrwxrwx 1 root root       7 мая 23 01:34 /dev/mapper/dmp1 -> ../dm-1
lrwxrwxrwx 1 root root       7 мая 23 01:33 /dev/mapper/zero1 -> ../dm-0

```

- Операции на запись и чтение

`dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1`

```bash
1+0 записей получено
1+0 записей отправлено
4096 байт (4,1 kB, 4,0 KiB) скопирован, 0,00039053 s, 10,5 MB/s
   
```

`dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1`

```bash
1+0 записей получено
1+0 записей отправлено
4096 байт (4,1 kB, 4,0 KiB) скопирован, 0,000466928 s, 8,8 MB/s    
```

- Статистика

`cat /sys/module/dmp/stat/volumes`

```bash
read:
	reqs: 42
	avg size: 4388
write:
	reqs: 1
	avg size: 4096
total:
	reqs: 43
	avg size: 4381

```




