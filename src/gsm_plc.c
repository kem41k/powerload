/*******************************************************
Project : GSM900 power loads control
Version : 1.2
Date    : 27.06.2016
Author  : Kamil Yagafarov
Company : SPbPU
Comments: -

Chip type               : ATmega2560
Program type            : Application
AVR Core Clock frequency: 16,000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 2048
*******************************************************/

#include <mega2560.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mega2560_init.c>  
#include <gsm_plc.h>        
#include <i2c.h>            
#include <ds1307.h>         
#include <sdcard.h>         
#include <ff.h>   
#include <add_methods.c>  
#include <USART.c>    
#include <SIM900.c>   
#include <SMS.c>         

void main(void)
{ 
    unsigned char note = 0, attempts = 0; 
    char SMS_text[SMS_MAX_SYMBOLS], str1[5];   
    unsigned char time[10];  
    unsigned char k = 0, sign = 0;
    
	// ============== INITIALIZATION ==============
	
    // Shields and ATmega2560 initialization
    i2c_init();
    rtc_init(0,0,0);
    mega2560_init();   
    
    // Pointers to work with EEPROM
    ptr_eeprom_mode = &eeprom_mode;
    ptr_eeprom_power = &eeprom_power;
    ptr_ram_mode = &ram_mode;
    ptr_ram_power = &ram_power;
    
    // Global enable interrupts
    #asm("sei")    
    
    // Set a real time
    #ifdef SET_RTC
        rtc_set_time(10,00,0); 
        rtc_get_time(&rtc_hour, &rtc_minute, &rtc_second);
        printf("RTS was set to %02i:%02i:%02i", rtc_hour, rtc_minute, rtc_second);
    #endif
    
    // Availability check
    while (!check_SIM900()) 
    {
        GSM_shield_start();
        delay(5); 
    }    
    while (!check_SD_shield() && attempts < 5)
    {
        attempts++;
        delay(5);
    }   
    
    f_mount(0, &fs);      // File system mount to work with files on SD card
    
    ram_mode = *ptr_eeprom_mode;
    ram_power = *ptr_eeprom_power;         
    subscr = 2;     	  // Main subscriber (default)
    
	start_SMS();   
    check_unread_sms(); 
	
	// ============== WORK PROCESS ==============

    while (1)
    {
        note = listen_USART1();
		// Get new message
        if (note > 0)
        {
            #ifdef SERIAL
                printf("\nNEW SMS #%d after %d sec.\n", note, seconds); 
            #endif
            read_SMS(note);  
        }
        // Change load on a schedule (graph 'a','b' or 'c')
        if (FLAG.MODE)
        {
            // Set required load
            if (FLAG.LOAD_CHANGE_REQUIRED == 1)
            {
                current_P = max_P / 100 * percent_from_file();
                #ifdef SERIAL
                    printf("\n%02i:%02i:%02i", rtc_hour, rtc_minute, rtc_second); 
                    printf(" Interval #%d, P = %d * %d%% = %dW\n", last_interval, max_P, percent_from_file(), current_P); 
                #endif
                set_power_loads(current_P, 0);
                FLAG.LOAD_CHANGE_REQUIRED = 0; 
            } 
            // Time update
            if (seconds % 60 == 59) FLAG.TIME_CHECK = 1;
            if (seconds % 60 == 0 && FLAG.TIME_CHECK) 
            {
                printf("Time check");
                rtc_get_time(&rtc_hour, &rtc_minute, &rtc_second); 
                FLAG.TIME_CHECK = 0; 
            }
            // Start of new interval
            if (last_interval != interval_number())
            {
                FLAG.LOAD_CHANGE_REQUIRED = 1; 
                last_interval = interval_number(); 
            }
        }
        // Check an availability of SIM900
        if (seconds % 600 == 599) FLAG.SIM900_CHECK = 1;
        if (seconds % 600 == 0 && FLAG.SIM900_CHECK) 
        {
            FLAG.SIM900_CHECK = 0;
            FLAG.SIM900_AVAILABLE = check_SIM900();  
            check_unread_sms();
        } 
        if (!FLAG.SIM900_AVAILABLE) GSM_shield_start();  
    } 
}