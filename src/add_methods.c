// Timer1 handler (1 s)
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
    TCNT1H=0;
    TCNT1L=0;  
    seconds++;
}

// Timer3 handler (10 ms)
interrupt [TIM3_COMPA] void timer3_compa_isr(void)
{
   TCNT3H=0;
   TCNT3L=0; 
   disk_timerproc();
}

/* 	Substring method

	Parameters:
	* string1 - substring
	* string2 - string
	
	Returns: 
	* number of last comformed symbol
	* 255 - otherwise
*/
unsigned char find_str(char* string1, char* string2)
{
     unsigned char id1 = 0, 		// Symbol number in substring
				   id2 = 0, 		// Current symbol
				   i = 0, 
				   coinsedence = 0; // Amount of conformed symbols
     
     while(i < strlen(string2))
     {
        // Match is found
        if (string1[id1] == string2[i])
        {
           if (coinsedence == 0) id2 = i; 
           coinsedence++;
           if (coinsedence == strlen(string1)) return id2;
           id1++;
        }
        else 
        {
           if (coinsedence > 0) 
           {
                i -= coinsedence;
                coinsedence = 0;
           } 
           id1 = 0;
        } 
        i++;
     } 
     return 255;
}

// Calculate current interval number using current hour and minute
unsigned char interval_number()
{
   return rtc_hour * 6 + rtc_minute / 10; 
}

/* 	Set the required power load

	Parameters: 
	* target_value - required power load
	* sign = 0, if no need of reply 
*/
void set_power_loads(int target_value, unsigned char sign)
{
    unsigned int temp_value = 0;     // Current power load
    signed char k = POWER_LOADS_AMOUNT - 1; 
    
    #ifdef SERIAL
        printf("\nSetting power loads to required %dW\n", target_value);
    #endif  
    for (k = POWER_LOADS_AMOUNT - 1; k >= 0; k--)
    {   
        pins[k] = 0;    
        // Set next load
        if(loads[k] <= (target_value - temp_value)) 
        {
            temp_value += loads[k];
            pins[k] = 1; 
            #ifdef SERIAL
                printf("%d W, ", loads[k]);
            #endif
        } 
        
    }  
    #ifdef SERIAL 
        printf("total power=%d W\nPins: ", temp_value);
        for (k = 0; k < POWER_LOADS_AMOUNT; k++)
            printf("%d", pins[k]);
        printf("\n");     
    #endif
    // Set a load state (on/off) with inverted signal
    PORTA.0 = !pins[0];
    PORTA.1 = !pins[1];
    PORTA.2 = !pins[2];
    PORTA.3 = !pins[3]; 
    PORTA.4 = !pins[4];
    PORTA.5 = !pins[5]; 
    PORTA.6 = !pins[6]; 

    current_P = temp_value; 
    // Fan usage if power is more than 0.5 kW
    if (current_P >= 500) PORTA.7 = 0;        
    else PORTA.7 = 1;  
    
    if (sign) answer_SMS_request("setload");
}

// Get the % value from file in 1-3 modes
unsigned char percent_from_file()
{
    unsigned char pos = 0;
    
    // File open
    if (mode_file_name == 'a') f_open(&fp, "0:/a.txt", FA_OPEN_EXISTING | FA_READ);
    else if (mode_file_name == 'b') f_open(&fp, "0:/b.txt", FA_OPEN_EXISTING | FA_READ);
    else if (mode_file_name == 'c') f_open(&fp, "0:/c.txt", FA_OPEN_EXISTING | FA_READ);
    else return 0;
    // Seek the required position in file
    pos = interval_number();
    if (pos >= 0 && pos <= 143)
    {
        f_lseek (&fp, pos * 5);  		// Each line has 5 symbols
        f_read(&fp, sd_buffer, 3, &br); // Read first 3 symbols
        return atoi(sd_buffer);
    }  
    else return 0;
}

// Check an availability of SD shield
unsigned char check_SD_shield()
{
   if (disk_initialize(0) != 0)
   {
       #ifdef SERIAL
           printf("\nSD shield does not respond!\n");   
       #endif
       FLAG.SD_AVAILABLE = 0;    
       return 0;
   }     
   else 
   {
       #ifdef SERIAL
           printf("\nSD shield is ready!\n");   
       #endif 
       FLAG.SD_AVAILABLE = 1;   
       return 1;
   }
}

// Time delay (in seconds)
void delay(unsigned int interval)
{
    unsigned long start = seconds; 
    while (seconds < start + interval);
}
