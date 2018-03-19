# Power Load Simulator #

### General description ###

* The program is written for power load simulator based on ATmega2560 board;
* Authorized user could set the power load change program via SMS (ATmega2560 is connected to GSM shield for that purpose) and get current info.
* User is authorised, if his/her number is written in program.
* Power loads programs are saved on micro SD card and are titled as "a.txt", "b.txt" and "c.txt".

### Hardware ###

* ATmega2560 board;
* GSM shield (SIM900);
* DS3231 chip (real time);
* microSD card shield.

### SMS variations ###

1) Set a constant power load

Send: 	'X', where X - is a value from 0 to 5000 corresponding to 0-5 kW.

Answer:	'Loads: X1,X2,X3, ..., total: X, time HH:MM:SS', where X1-Xn correspond to chosen loads.

2) Choose a power load graph A, B or C

Send:	'z-X', where z could be 'a', 'b' or 'c', X - maximum value of power load.

Answer:	'Mode: z, Pnom: X, time HH:MM:SS'.

3)Status check

Send: 	'status'

Answer: depends on current mode'

* Loads: X1,X2,X3, ..., total: X, time HH:MM:SS'
* 'Loads: X1,X2,X3, ..., total: X, mode: z, Pnom: P, time HH:MM:SS' 