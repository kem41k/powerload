/* 	AT-command send

	Parameters:
	* ATcommand
	* exp_answer - expected answer, e.g., "ОК"
	* timeout 
	* save = 1, if need to save answer
	
	Returns: 
	* 0 - timeout
	* 1 - AT-command succeed (expected answer is received)
	* 2 - answer contains 'ERROR'
	* 3 - received answer is different from expected
*/
unsigned char send_command_to_SIM900(char* ATcommand, char* exp_answer, unsigned char timeout, unsigned char save)
{
    unsigned char answer = 3;
	unsigned int i = 0, k;
    unsigned long start; // Timing
    char c;
    char response[USART1_MAX_RESPONSE_BUF];       // Response text from SIM900

    memset(response, '\0', USART1_MAX_RESPONSE_BUF);    
	// Wait for buffer clean
    while(UCSR1A & (1<<RXC1)) 
		c = UDR1;   
    
    write_to_USART1(ATcommand);                 
    start = seconds;    
    
	do
	{
        if(UCSR1A & (1<<RXC1))
        {
            response[i] = UDR1;  
			// Buffer overflow
            if (i == (USART1_MAX_RESPONSE_BUF - 1))
            {
                for (k = 1; k < USART1_MAX_RESPONSE_BUF; k++)
                   response[k - 1] = response[k];  
            }
            else i++;  
            // Compare received and expected answers
            if (strstr(response, exp_answer) != NULL)    
		        answer = 1;
            else if (strstr(response, "ERROR") != NULL) 
                answer = 2;
        } 
    }
    while((answer == 3) && ((seconds - start) < timeout));    
   
    // Debugging
    if (i > 0)
    {
        #ifdef SERIAL
            printf("\nAnswer -->");
            for (k = 0; k < i; k++) 
                putchar(response[k]);
            printf("<--\n"); 
        #endif 
    }
    else 
    {
        #ifdef SERIAL
            printf("TIMEOUT %d sec!", timeout);
        #endif
        answer = 0;
    }  
    
    // Saving last response from SIM900
    if (save)
    {
        memset(last_response, '\0', USART1_MAX_RESPONSE_BUF); 
        for (k = 0; k < i; k++) 
            last_response[k] = response[k]; 
    } 
    return answer;
}

// Check an availability of SIM900
unsigned char check_SIM900()
{
    unsigned char attempts = 0;

    // Send test commands
    while (send_command_to_SIM900("AT", "OK", 2, 0) != 1 && attempts < 3)
        attempts++; 
    if (attempts == 3)
    {
        #ifdef SERIAL
           printf("\nSIM900 does not respond!\n");   
        #endif
        FLAG.SIM900_AVAILABLE = 0;
        return 0;
    }
    else
    {
        FLAG.SIM900_AVAILABLE = 1;
        #ifdef SERIAL
            printf("\nSIM900 is ready!\n");
        #endif
        return 1;
    }
}

// GSM shield start
void GSM_shield_start()
{
   #ifdef SERIAL
        printf("\nTrying to start GSM-shield\n");
   #endif 
   delay(2);
   PORTH |= (1<<PORTH5);    
   delay(2);
   PORTH &= (0<<PORTH5);  
   delay(5);    
   
   FLAG.SIM900_AVAILABLE = check_SIM900(); 
}