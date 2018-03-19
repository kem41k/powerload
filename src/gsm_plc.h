#define DATA_REGISTER_EMPTY (1<<UDRE0)
#define RX_COMPLETE (1<<RXC0)
#define FRAMING_ERROR (1<<FE0)
#define PARITY_ERROR (1<<UPE0)
#define DATA_OVERRUN (1<<DOR0)
#define USART1_MAX_RESPONSE_BUF 255
#define SMS_MAX_SYMBOLS 160				// Amount of symbols in SMS
#define POWER_LOADS_AMOUNT 7 
//#define SERIAL                   		// Uncomment if need to print debug info in serial
//#define SET_RTC						// Uncomment if need to set real time

#pragma warn-
eeprom unsigned int eeprom_mode = 0;		// Current mode
eeprom unsigned int eeprom_power = 0;		// Current load value
unsigned int eeprom *ptr_eeprom_mode = 0;
unsigned int eeprom *ptr_eeprom_power = 0;
#pragma warn+
unsigned int ram_mode = 0,  // 0 - const power load value, 1 - mode 'a', 2 - 'b', 3 - 'c'
             ram_power = 0; // Last power value
unsigned int *ptr_ram_mode;
unsigned int *ptr_ram_power;

// Flags
struct struct_flags
{
     unsigned char SIM900_AVAILABLE:1;
     unsigned char SIM900_CHECK:1;         // Necessity of SIM900 check
     unsigned char SD_AVAILABLE:1;          
     unsigned char TIME_CHECK:1;           // Necessity of DS3231 shield check
     unsigned char MODE:1;                 
     unsigned char LOAD_CHANGE_REQUIRED:1; 
}FLAG;

unsigned char not_buf_i = 0,    	// SIM900 notifications iterator
              subscr = 0,       	// Number of current subscriber 
              rtc_hour = 0,     
              rtc_minute = 0,   
              rtc_second = 0,   
              mode_file_name = 'a', // Current mode name
              last_interval = 0;	// Last chosen interval
              
unsigned int seconds = 0,       	// Time of work in seconds
             time_stamp = 0,    	// Start of time interval
             current_P = 0,     	// Current power load, W
             max_P = 0,         	// Maximum power load, W (from file)
             br;                	// Iterator for r/w operations

char last_response[255],        	// Last response from SIM900
     phone1[] = "+7XXXXXXXXXX", 
     phone2[] = "+7XXXXXXXXXX",
     notification_buffer[20],   
     sd_buffer[4];              	// microSD card buffer
     
unsigned int loads[] = {100, 200, 200, 500, 1000, 1000, 3000},		// Power loads
             pins[] = {0, 0, 0, 0, 0, 0, 0};                   		// Power loads states

FATFS fs;                       // Object to work with microSD

FIL fp;                         // Object to work with file from microSD
