// Read char from USART1
#pragma used+
char getchar1(void)
{
    char status,data;
    while (1)
    {
        while (((status=UCSR1A) & RX_COMPLETE)==0);
        data=UDR1;
        if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
            return data;
    }
}
#pragma used-

// Send char to USART1
#pragma used+
void putchar1(char c)
{
    while ((UCSR1A & DATA_REGISTER_EMPTY)==0);
    UDR1=c;
}
#pragma used-

// Write command to USART1
void write_to_USART1(char* command)
{
    unsigned i = 0; 
    #ifdef SERIAL
        printf("\nSending command: %s\n", command);
    #endif
    for (i = 0; i < strlen(command); i++)
        putchar1(command[i]); 
    // Send line-feed symbol
    putchar1(0x0D);  // '\r'
    putchar1(0x0A);  // '\n'
}

/* 	Wait for new data in USART1

	Returns: 
	* 0 - no SMS
	* number of new SMS (1-20)
*/
unsigned int listen_USART1()
{ 
    unsigned char start = 0, i = 0;         
    unsigned int sms_id;
    
    while(UCSR1A & (1<<RXC1))
    {
        time_stamp = seconds;   
        notification_buffer[not_buf_i] = UDR1; 
        if (not_buf_i < 19) not_buf_i++; 
    } 
	// Check the buffer after timeout
    if (seconds - time_stamp >= 2 && not_buf_i > 3)
    {
        #ifdef SERIAL
            printf("\nBUFFER -->");
            for (i = 0; i < strlen(notification_buffer); i++)
                putchar(notification_buffer[i]);
            printf("<--\n");
        #endif 
        
        // SMS notification
        if (strstr(notification_buffer, "+CMTI: \"SM\"") != NULL)   
        {
            start = find_str("SM", notification_buffer);
            start += 4;  
            
            sms_id = notification_buffer[start] - '0';      // Convert to int
            #ifdef SERIAL
                printf("\nNEW SMS #%d.\n", sms_id);   
            #endif
            return sms_id;
        }
        // SMS send notification     
        else if (strstr(notification_buffer, "+CMGS: \"SM\"") != NULL)  
        {
            #ifdef SERIAL
                printf("\nSIM900 send SMS with an answer to request!\n");
            #endif 
        }    
        // Buffer clean
        not_buf_i = 0;
        memset(notification_buffer, '\0', 20);  
        #ifdef SERIAL
            printf("\nBuffer was cleared!\n");
        #endif 
        
        return 0;
    }   
    
    return 0;
}