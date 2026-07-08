#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include <avr/boot.h>

#define  BUFFER_SIZE 64
const unsigned int baud_table[] = {34, 51, 104, 208};
const char* baud_names[] = {"57600", "38400", "19200", "9600"};
const int num_bauds = 4;
uint32_t last_toggle_time = 0;        // Timer for the LED

// Variables for handling our buffer
volatile char rx_buffer[BUFFER_SIZE]; // Our "sheet of paper" for the sentence
volatile int rx_index = 0;            // Index pointing to where we are writing in the buffer
volatile int message_ready = 0;
volatile uint32_t ms_tick = 0;
volatile uint32_t blink_delay = 0;
volatile uint8_t blink_active = 0;    // The "party" switch

// --- Initialization functions ---

// UART initialization
void UART_init_manual(unsigned int ubrr) {
	UCSR0B = 0; // Disable before making changes
	
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	
	// Enable double speed (U2X0) for stability at 115200 baud
	UCSR0A = (1 << U2X0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	
	// Clear the buffer of garbage from previous failed attempts
	unsigned char dummy;
	while (UCSR0A & (1 << RXC0)) {
		dummy = UDR0;
	}
}

// Timer1 initialization
void SystemTime_init(){
	// 1. CTC mode and Prescaler 1024
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
	
	// 2. (1 second for 1024 prescaler)
	OCR1A = 249;
	
	// 3. Enable hardware interrupt
	TIMSK1 = (1 << OCIE1A);
}


// === "Built-in" functions ===
//  --- Time ---

// time comparison
int timeout(uint32_t time_start, int duration){
	if(ms_tick - time_start >= duration){
		return 1;
	}
	return 0;
}

// get time
uint32_t get_time(void){
	return ms_tick;
}

//  --- STRINGS ---
// string comparison
int strcomp(volatile char* input, const char* ref) {
	int i = 0;
	while(!(input[i] == '\0' || ref[i] == '\0') ){
		if(input[i] != ref[i]){
			return 0;
			} else {
			i++;
		}
	}
	if(input[i] == '\0' && ref[i] == '\0') {return 1;}
	return 0;
}

int starts_with(volatile char* input, const char* ref){
	int i = 0;
	while(ref[i] != '\0'){
		if(input[i] != ref[i]){
			return 0;
		}
		i++;
	}
	return 1;
}

int parse_number(volatile char* input, int start_index) {
	int result = 0;
	int i = start_index;
	
	while(input[i] != '\0'){
		if (input[i] >= '0' && input[i] <= '9')
		{
			result = result * 10 + (input[i] - '0');
			} else {
			return 0;
		}
		i++;
	}
	
	return result;
}

// --- UART ---
// Send a single char via UART
void UART_transmit(char data) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

// Receive a single char
char UART_receive(void) {
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

// Send a string via UART
void UART_print(const char* str) {
	while (*str != '\0') {
		UART_transmit(*str);
		str++;
	}
}

// Print hex bytes
void hexb_print (uint8_t byte){
	int nibble_h = (byte >> 4);
	int nibble_l = (byte & 0x0F);
	char hex[3];
	if (nibble_h < 10)
	{
		hex[0] = nibble_h + '0';
		} else {
		nibble_h -= 10;
		hex[0] = nibble_h + 'A';
	}
	if (nibble_l < 10)
	{
		hex[1] = nibble_l + '0';
		} else {
		nibble_l -= 10;
		hex[1] = nibble_l + 'A';
	}
	hex[2] ='\0';
	UART_print(hex);

}

// UART bps auto-negotiation
void UART_AutoBaud() {
	int current_baud_idx = 0;
	
	while(1) {
		UART_init_manual(baud_table[current_baud_idx]);
		
		// Wait for a character (it won't freeze this time!)
		while (!(UCSR0A & (1 << RXC0)));
		
		char frame_error = (UCSR0A & (1 << FE0));
		char received_char = UDR0;
		
		if (frame_error == 0 && received_char == 'U') {
			UART_print("\r\n--- AUTO-BAUD OK! (");
			UART_print(baud_names[current_baud_idx]); // Inserts e.g., "57600"
			UART_print(" bps) ---\r\n");
			return;
			} else {
			_delay_ms(20);
			current_baud_idx++;
			if (current_baud_idx >= num_bauds) {
				current_baud_idx = 0;
			}
		}
	}
}

// --- CLI Functions ---
void LED_on(void){
	DDRB |= (1 << PB5);
	PORTB |= (1 << PB5);
}

void LED_off(void){
	PORTB &= ~(1 << PB5);    // Optional: force the LED off on stop
}

void PING(void){
	UART_print("--- MCU Serial Number --- \r\n");
	for (size_t i = 0x0E; i <= 0x17; i++)
	{
		uint8_t i_byte = boot_signature_byte_get(i);
		hexb_print(i_byte);
	}
	UART_print("\r\n");
}



int main(void) {
	// Fire up our auto-calibration function
	UART_AutoBaud();
	SystemTime_init();

	UART_print("CLI ATmega in C!!!\r\n");


	sei();
	while (1) {
		if(blink_active){
			uint32_t current_time = get_time();
			// Check our timer "on the fly"
			if (timeout(last_toggle_time, blink_delay)) {
				PORTB ^= (1 << PB5);             // Blink
				last_toggle_time = current_time; // Rewind the timer
			}
		}

		// Instead of hanging in the ISR, the processor has time to print this here!
		if (message_ready == 1) {

			if (strcomp(rx_buffer, "LED on")){
				LED_on();
				UART_print("LED has been turned on \r\n");
			} else if (strcomp(rx_buffer, "LED off"))
			{
				LED_off();
				UART_print("LED has been turned off \r\n");
			} else if (strcomp(rx_buffer, "PING"))
			{
				PING();
				}else if (starts_with(rx_buffer, "BLINK ")) {
				uint32_t numb_buffor = parse_number(rx_buffer, 6);
				if(numb_buffor != 0 ) {
					blink_delay = numb_buffor;
					UART_print("Starting blinking in the background!\r\n");
					DDRB |= (1 << PB5);     // Set pin as output
					blink_active = 1;    // Start the background task
					last_toggle_time = get_time(); // Save the start time
				}
				else {
					if (strcomp(rx_buffer, "BLINK off")) {
						UART_print("End of blinking.\r\n");
						blink_active = 0;    // Disable the background task
						LED_off();              // Turn off the LED for good
						} else {
						UART_print("Invalid command\r\n");
					}
				}
				} else {
				UART_print("Unknown command\r\n");
			}
			
			
			
			// Cleanup after reading (lower the flag and reset the index)
			rx_index = 0;
			message_ready = 0;
		}
	}
	return 0;
}

// --- Interrupts ---
// UART RX Interrupt
ISR(USART0_RX_vect){
	// 1. Wait for any character from the keyboard
	char c = UDR0;
	
	// Optional: echo the character immediately so you can see what you type in Realterm
	// UART_transmit(c);

	// 2. Check if it's an Enter (End of sentence)
	if (c == '\r' || c == '\n') {
		rx_buffer[rx_index] = '\0'; // Terminate the string
		
		// INSTEAD OF PRINTING - WE RAISE THE FLAG!
		message_ready = 1;
	}
	// 3. Normal letter
	else {
		if (rx_index < (BUFFER_SIZE - 1) && message_ready == 0) {
			rx_buffer[rx_index] = c;
			rx_index++;
		}
	}
}

// Timer interrupt
ISR(TIMER1_COMPA_vect){
	ms_tick++;
}
