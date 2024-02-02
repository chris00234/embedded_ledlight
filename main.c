#include <stdint.h>

// Define additional registers and masks
#define GPIOA_BASE 0x40004000 // Base address for GPIO Port A
#define GPIO_DIR (*((volatile unsigned long *)(GPIOA_BASE + 0x400)))
#define GPIO_DEN (*((volatile unsigned long *)(GPIOA_BASE + 0x51C)))
#define GPIO_DATA (*((volatile unsigned long *)(GPIOA_BASE + 0x3FC)))
#define PA2 0x04 // PA2 pin mask for the LED
#define PA4 0x10 // PA4 pin mask for the pushbutton


//address = 0x4000C000 for UART0
#define UART0 (*((volatile unsigned long *)0x4000C000))

//address = 0x4000C000, offset = 0x024 Integer baud rate divisor
#define UARTIBRD (*((volatile unsigned long *)0x4000C024))

//address = 0x4000C000, offset = 0x028 fractional baud rate divisor
#define UARTFBRD (*((volatile unsigned long *)0x4000C028))

//address = 0x4000C000, offset = 0x030 to control behavior
#define UARTCLT (*((volatile unsigned long *)0x4000C030))

//address = 0x4000C000, offset = 0x02C to line control
#define UARTLCRH (*((volatile unsigned long *)0x4000C02C))

// UARTFR register for checking TXFF and RXFE bits
#define UARTFR (*((volatile unsigned long *)0x4000C018))

// UARTDR register for transmitting and receiving data
#define UARTDR (*((volatile unsigned long *)0x4000C000))

//address = ox400F`E000, offset = 0x618 RCGCUART control clock sent to UART modules
#define RCGCUART (*((volatile unsigned long *)0x400FE618))

//address =  0x400FE000, offset = 0x608 RCGCGPIO is for clock gate
#define RCGCGPIO (*((volatile unsigned long *) 0x400FE608))

//address = 0x40025000, offset = 0x420 is for alternative function select
#define GPIOA_AFSEL (*((volatile unsigned long *)0x40004420))

//address = 0x40025000, offset = 0x52C for port control
#define GPIOA_PCTL (*((volatile unsigned long *)0x4000452C))

//set pos vs neg states
typedef enum { POSITIVE, NEGATIVE } SystemState;
SystemState currentState = POSITIVE;

void UART_Init(void);
void GPIO_Init(void);
void UART_Tx(char data);
char UART_Rx(void);
void UpdateLEDState(void);


int main(void) {
    // Initialize UART and GPIO
    UART_Init();
    GPIO_Init();

    while (1) {
        // Check for UART input to change state
            // Data is available to be read
        if((UARTFR & 0x10) != 0) {
            char input = UART_Rx();
            if (input == 'p') {
                currentState = POSITIVE;
            } else if (input == 'n') {
                currentState = NEGATIVE;
            }
        }
        //currentState = POSITIVE;

        //based on push button and system state
        UpdateLEDState();
    }
}

void UART_Init() {
    //RCGC Register enable UART0 module
    RCGCUART |= (1 << 0);

    //Enable clock for PortA. enable clock -> GPIO module using RCGCGPIO
    RCGCGPIO |= (1 << 0); // Enable clock for Port A

    //GPIO_AFSEL set -> PA0, PA1
    GPIOA_AFSEL |= (1 << 0) | (1 << 1); // Enable alternate function for PA0 and PA1

    //GPIO_PCTL register -> select UART functions to following pins
    //0-7 clear bits
    GPIOA_PCTL &= ~0x000000FF;
    //PA0, PA1 select UART function
    GPIOA_PCTL |= 0x00000011;

    //disable analog function on UART pins
    GPIO_DEN |= (1 << 0) | (1 << 1); // Enable digital function for PA0 and PA1

    //disable UART0
    UARTCLT &= ~(1 << 0);

    //write UART_IBRD register
    //16MHZ system clock
    uint32_t uart_clk = 16000000;
    uint32_t baud_rate = 9600;
    //baud rate divisor
    uint32_t brd = uart_clk / (16 * baud_rate);
    UARTIBRD = brd;

    //write UART_FBRD register
    uint32_t fbrd = (uint32_t)(((float)uart_clk / (16 * baud_rate) - brd) * 64 + 0.5);
    UARTFBRD = fbrd;

    //write serial parameter -> UART_LCRH register
    //8 databits, 1 stop bits, no parity bits
    UARTLCRH = (0x3 << 5);

    //enable UART0 by writing UARTCTL register.
    UARTCLT = (1 << 0) | (1 << 8) | (1 << 9);
}

//void GPIO_Init() {
//    // Enable clock for GPIO Port A and configure pins PA2 and PA4
//    RCGCGPIO |= 0x01; // Enable clock for Port A
//    GPIO_DIR |= PA2; // Set PF2 as output
//    GPIO_DEN |= PA2 | PA4; // Enable digital functions for PA2 and PA4
//    GPIO_DATA &= ~PA2; // Initially turn off the LED
//}

void GPIO_Init() {
    // Enable clock for Port A
    RCGCGPIO |= 0x01;

    // Configure PA2 as output for the LED
    GPIO_DIR |= PA2;

    // Configure PA4 as input for the pushbutton
    GPIO_DIR &= ~PA4;

    // Enable digital functions for PA2 and PA4
    GPIO_DEN |= PA2 | PA4;


    // Initially turn off the LED
    GPIO_DATA &= ~PA2;
}


void UART_Tx(char data) {
    while((UARTFR & 0x20) == 1);
    UARTDR = data;
}

char UART_Rx() {
    char data;
    while((UARTFR & 0x10) == 1);
    data = UARTDR;
    return ((unsigned char) data);
//    return ('p');
}
void UpdateLEDState() {
    // Update the LED state based on the pushbutton and current system state
    // Read pushbutton state (active low)
    int buttonPressed = (GPIO_DATA & PA4);
    if (currentState == POSITIVE) {
        if (buttonPressed) {
            // Turn on the LED
            GPIO_DATA |= PA2;
        } else {
            // Turn off the LED
            GPIO_DATA &= ~PA2;
        }
    } else { // NEGATIVE state
        if (buttonPressed) {
            // Turn off the LED
            GPIO_DATA &= ~PA2;
        } else {
            // Turn on the LED
            GPIO_DATA |= PA2;
        }
    }
}
