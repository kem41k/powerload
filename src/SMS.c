// Send SMS
void send_SMS(char* text)
{
    char command[] = "AT+CMGS=\"", SMS_text[SMS_MAX_SYMBOLS]; 
    unsigned char attempts = 0, i = 0;     
    
    for(i = 0; i < SMS_MAX_SYMBOLS; i++) 
    {   
        if (i < strlen(text)) SMS_text[i] = text[i];
        else SMS_text[i] = '@';
    }
    printf("Sending SMS [%d symb]: %s", strlen(SMS_text), SMS_text);
    
    // Send subscriber number first
    if (subscr == 1) strcat(command, phone1); 
    else if (subscr == 2) strcat(command, phone2);
    strcat(command, "\"");
    while (send_command_to_SIM900(command, ">", 2, 0) != 1 && attempts < 3) 
        attempts++;
    
    if (attempts == 3)
    {
        FLAG.SIM900_AVAILABLE = 0;
        #ifdef SERIAL
             printf("\nCan not send SMS (SIM900 is not answering).\n");
        #endif
        return;
    }   
    
    // Sending the message with CTRL+Z symbol in the end
    #ifdef SERIAL
        printf("\nStart sending to USART1...\n");
    #endif
    printf("\nText: %s -->", SMS_text);   
    for (i = 0; i < strlen(SMS_text); i++)
    {
        if (SMS_text[i] != '@') putchar1(SMS_text[i]); 
        else break;
    }
    putchar1(0x1A); // CTRL+Z symbol meaning end of message
    putchar1('\0');
    #ifdef SERIAL
        printf("\nSMS from SIM900 was sent!\n");
    #endif  
    
    // Read SMS delete
    attempts = 0;    
    while(send_command_to_SIM900("AT+CMGD=1,3", "OK", 2, 0) != 1 && attempts < 3)
        attempts++;
    if (attempts == 3)  
    {
        #ifdef SERIAL
            printf("\nCan not delete sent SMS (SIM900 is not answering).\n");
        #endif
        return;
    } 
    #ifdef SERIAL
        printf("\nAll sent messages were deleted!\n");
    #endif   
}

// Initial SMS preparation
void start_SMS()
{
   char SMS_text[SMS_MAX_SYMBOLS]; 
   unsigned char time[10];   
   
   for (subscr = 1; subscr < 3; subscr++)
   {
       // SMS text preparation
        strcpy(SMS_text, "System was started at ");  
        rtc_get_time(&rtc_hour, &rtc_minute, &rtc_second); 
        sprintf(time, "%02i:%02i:%02i", rtc_hour, rtc_minute, rtc_second);
        strcat(SMS_text, time);   
        
        #ifdef SERIAL
            printf("start SMS: %s\n", SMS_text);
        #endif 
        send_SMS(SMS_text);
        memset(SMS_text, '\0', SMS_MAX_SYMBOLS);
   }
   subscr = 0; 
}

// Answer to subscriber when new SMS is received
void answer_SMS_request(char* command)
{
    char SMS_text[SMS_MAX_SYMBOLS], str1[5];
    unsigned char time[10];
    unsigned char k = 0, sign = 0;
    
    for (k = 0; k < strlen(command); k++)
        command[k] = tolower(command[k]);
    
    memset(SMS_text, '\0', SMS_MAX_SYMBOLS);
	// If subscriber wants to set load 
    if (strstr(command, "setload") != NULL) 
    {
        // SMS text
        strcpy(SMS_text, "Loads: ");
        for (k = 0; k < POWER_LOADS_AMOUNT; k++)
        {
            if (pins[k]) 
            {
                itoa(loads[k], str1);
                strcat(SMS_text, str1);
                strcat(SMS_text, ", ");  
                memset(str1, '\0', strlen(str1));  
                sign++;
            }  
        } 
        if (sign == 0) strcat(SMS_text, "off, ");
	    strcat(SMS_text, " total: "); 
        itoa(current_P, str1);
        strcat(SMS_text, str1);
	    strcat(SMS_text, "W, time "); 
        rtc_get_time(&rtc_hour, &rtc_minute, &rtc_second); 
        sprintf(time, "%02i:%02i:%02i", rtc_hour, rtc_minute, rtc_second);
        strcat(SMS_text, time);
    }   
    // If subscriber wants to get current status
    else if (strstr(command, "status") != NULL)
    {
        strcpy(SMS_text, "Loads: ");
        for (k = 0; k < POWER_LOADS_AMOUNT; k++)
        {
            if (pins[k]) 
            {
                itoa(loads[k], str1);
                strcat(SMS_text, str1);
                strcat(SMS_text, ", ");  
                memset(str1, '\0', strlen(str1));  
                sign++;
            }  
        } 
        if (sign == 0) strcat(SMS_text, "off, ");
	    strcat(SMS_text, " total: "); 
        itoa(current_P, str1);
        strcat(SMS_text, str1);
	    strcat(SMS_text, "W, time "); 
        rtc_get_time(&rtc_hour, &rtc_minute, &rtc_second); 
        sprintf(time, "%02i:%02i:%02i", rtc_hour, rtc_minute, rtc_second);
        strcat(SMS_text, time);
        if (FLAG.MODE)
        {
            strcat(SMS_text, ", mode: ");
            if (mode_file_name == 'a') strcat(SMS_text, " A, Pnom: ");
            else if (mode_file_name == 'b') strcat(SMS_text, " B, Pnom: "); 
            else if (mode_file_name == 'c') strcat(SMS_text, " C, Pnom: ");
            itoa(max_P, str1);
            strcat(SMS_text, str1);
            strcat(SMS_text, "W");
        }
        if (!FLAG.SD_AVAILABLE)
        {
            strcat(SMS_text, ", SD failed!");
        }
    } 
    // If mode from file is chosen
    else if (strstr(command, "a-") != NULL || strstr(command, "b-") != NULL || strstr(command, "c-") != NULL)  
    {
        if (FLAG.SD_AVAILABLE) 
        {
            if (strstr(command, "a-") != NULL) 
            {
                mode_file_name = 'a'; 
                ram_mode = 1;
            }
            else if (strstr(command, "b-") != NULL) 
            {
                mode_file_name = 'b'; 
                ram_mode = 2;
            }
            else if (strstr(command, "c-") != NULL)
            {
                mode_file_name = 'c';
                ram_mode = 3;
            }
            else return;    // No such file 
            
            FLAG.MODE = 1;    
            FLAG.LOAD_CHANGE_REQUIRED = 1;   
            rtc_get_time(&rtc_hour, &rtc_minute, &rtc_second);    
            last_interval = interval_number();
            
            // Read the maximum load value from SMS
            for (k = 2; k < 6; k++)
                if (isdigit(command[k])) str1[k - 2] = command[k];  
            max_P = atoi(str1);  
            memset(str1, '\0', strlen(str1));
            ram_power = max_P;  
            
            // Write to EEPROM
            *ptr_eeprom_mode = *ptr_ram_mode;
            *ptr_eeprom_power = *ptr_ram_power;  
            
            // Answer SMS text
            strcpy(SMS_text, "Mode: ");
            if (mode_file_name == 'a') strcat(SMS_text, " A, Pnom: ");
            else if (mode_file_name == 'b') strcat(SMS_text, " B, Pnom: "); 
            else if (mode_file_name == 'c') strcat(SMS_text, " C, Pnom: ");
            itoa(max_P, str1);
            strcat(SMS_text, str1);
            strcat(SMS_text, "W, time "); 
            rtc_get_time(&rtc_hour, &rtc_minute, &rtc_second); 
            sprintf(time, "%02i:%02i:%02i", rtc_hour, rtc_minute, rtc_second);
            strcat(SMS_text, time);  
        }
        // SD card problem
        else
        {
            strcpy(SMS_text, "SD shield does not respond. This mode is unavailable. ");
        }
    }       
    // No answer if get an unknown command
    else return;
    #ifdef SERIAL
            printf("\nPreparing answer [%d]: %s<--\n", strlen(SMS_text), SMS_text); 
    #endif
    send_SMS(SMS_text);
}

// Read SMS with current number
void read_SMS(unsigned int number)
{
    char sms_id_char[2], command[] = "AT+CMGR=", value[10];  
    unsigned char start = 0, 
                  end = 0, 
                  attempts = 0, 
                  k = 0, 
                  temp = 0, 
                  sign = 0;     // if sign = 1, then SMS contains letters, if sign = 2, then SMS contains digits
    subscr = 0;
    
    // AT-command creation
    itoa(number, sms_id_char);    
    strcat(command, sms_id_char);  
    strcat(command, ",0"); 
    memset(last_response, '\0', USART1_MAX_RESPONSE_BUF); 
    
    // 3 attempts to read SMS
    while(send_command_to_SIM900(command, "OK", 5, 1) != 1 && attempts < 3)
        attempts++;
    if (attempts == 3)
    {
        #ifdef SERIAL
            printf("\nCan not read SMS#%d (SIM900 is not answering).\n", number);
        #endif    
        FLAG.SIM900_AVAILABLE = 0;
        return;
    }   
    
    // Check the subscriber number (subscriber should be authorized)
    temp = find_str(phone1, last_response);  
    if (temp > 0 && temp < 255) 
    {
        start = temp;     
        subscr = 1;
    }
    else
    {
        temp = find_str(phone2, last_response);
        if (temp > 0 && temp < 255) 
        {
            start = temp;     
            subscr = 2;
        } 
    }    
    
    // Received SMS analysis
    if (start > 0 && start < 255)
    {
        #ifdef SERIAL
            if (subscr == 1) printf("\nSender: %s.\n", phone1);
            else if (subscr == 2) printf("\nSender: %s.\n", phone2);        
        #endif 
        start += 14;	// Date position
        while (last_response[start] != ',') start++;
        start++; 
        start++; 
        // Read datetime
        #ifdef SERIAL
            printf("Datetime: ");
            end = start + 16;
            while (start <= end)
            {
                putchar(last_response[start]); 
                start++;
            }      
        #else
            start += 16;
        #endif      
        // SMS text position
        start += 4;  
        
        // Read SMS
        while(start < strlen(last_response))
        {
			// If current symbol is a letter, digit or '-'
            if (isalnum(last_response[start]) || last_response[start] == '-')
            {
                value[k] = last_response[start]; 
                if (isdigit(value[k]) && sign == 0) sign = 1;    	// First word in SMS is a number
                else if (isalpha(value[k]) && sign == 0) sign = 2;  // First word in SMS is not a number
                k++;
            }     
            else if (k > 0) break; 
            if (k > 3 && sign == 1) break;      // Cut unnecessary letters in SMS
            if (k > 9 && sign == 2) break;      // Cut unnecessary letters in SMS
            start++;
        }
        #ifdef SERIAL
            printf("\nFirst word in SMS: %s\n", value);
        #endif
        // Set constant load if SMS contains only numbers
        if (sign == 1) 
        {
            FLAG.MODE = 0;   
            mode_file_name = 'z';   
            FLAG.LOAD_CHANGE_REQUIRED = 0;        
            max_P = 0;       
            // Write to EEPROM
            ram_mode = 0;
            ram_power = atoi(value);
            *ptr_eeprom_mode = *ptr_ram_mode;
            *ptr_eeprom_power = *ptr_ram_power; 
            set_power_loads(atoi(value), 1);     
        } 
        else if (sign == 2) answer_SMS_request(value);  
    }    
    // Read SMS delete
    attempts = 0;
    while(send_command_to_SIM900("AT+CMGD=1,1", "OK", 2, 0) != 1 && attempts < 3)
        attempts++;
    if (attempts == 3)  
    {
        #ifdef SERIAL
            printf("\nCan not delete read SMS (SIM900 is not answering).\n");
        #endif    
        FLAG.SIM900_AVAILABLE = 0;
        return;
    } 
    #ifdef SERIAL
        printf("\nAll read messages were deleted!\n");
    #endif      
}

// Check for unread SMS
void check_unread_sms()
{
    unsigned int sms_id = 0;   // Number of unread SMS (1-20) 
    unsigned char i = 0, attempts = 0;        
    char sms_id_char[2];  
    
    // If the answer contains "+CGML", then there is an unread message
    while(send_command_to_SIM900("AT+CMGL=\"REC UNREAD\",1", "OK", 10, 1) != 0)
    {
        i = find_str("+CMGL:", last_response); 
        if (i == 255) 
        {    
            #ifdef SERIAL
                puts("\nThere is no new messages!\n");
            #endif 
            break;
        }
        else
        {   
            // Search for number of unread SMS (after "+CMGL: " phrase)
            i += 7;
            sms_id_char[0] = last_response[i];
            sms_id_char[1] = last_response[i + 1];  
            sms_id = atoi(sms_id_char);
            memset(sms_id_char, '\0', 2);
            itoa(sms_id, sms_id_char); 
            #ifdef SERIAL
                printf("\nUnread SMS #%s.\n", sms_id_char);
            #endif   
            read_SMS(sms_id);
        }    
    }  
    
    // Read SMS delete
    while(send_command_to_SIM900("AT+CMGD=1,1", "OK", 2, 0) != 1 && attempts < 3)
        attempts++;
    if (attempts == 3)  
    {
        #ifdef SERIAL
            printf("\nCan not delete read SMS (SIM900 is not answering).\n");
        #endif
        FLAG.SIM900_AVAILABLE = 0;
        return;
    } 
    #ifdef SERIAL
        printf("\nAll read messages were deleted!\n");
    #endif   
}