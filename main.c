/*
 * File:   main.c
 * Author: paolo
 *
 * Created on March 12, 2025, 3:46 PM
 */
#include "xc.h"
#include "timer.h"

#define BAUDRATE 9600UL
#define FCY 72000000UL  
#define BRGVAL ((FCY / (16 * BAUDRATE)) - 1)

//ASSIGNMENT3 BASE
/*
void UART1_Init(void) {
    // Configure RD11 as input (U1RX) and RD0 as output (U1TX)
    TRISDbits.TRISD11 = 1; // Set RD11 as input
    TRISDbits.TRISD0 = 0;  // Set RD0 as output
    
    // Perform UART1 pin remapping
    RPINR18bits.U1RXR = 75; // RD11 corresponds to RPI75
    RPOR0bits.RP64R = 1;    // RD0 corresponds to RP64, map it to U1TX (1)
    
    U1BRG = BRGVAL; // Set baud rate
    // Configure UART1
    //U1MODEbits.UARTEN = 0; // Disable UART before configuration
    //U1MODEbits.BRGH = 0;   // Standard speed mode
    //U1MODEbits.PDSEL = 0;  // 8-bit data, no parity
    //U1MODEbits.STSEL = 0;  // 1 Stop bit
    
    // Enable UART -- SET IT BEFORE
    U1MODEbits.UARTEN = 1;
    
    // THEN U CAN -- Enable Transmit and Receive
    U1STAbits.UTXEN = 1;
    U1STAbits.URXDA = 1;

}

void UART1_WriteChar(char c) {
    while (U1STAbits.UTXBF); // Wait if transmit buffer is full
    U1TXREG = c;
}

char UART1_ReadChar(void) {
    while (!U1STAbits.URXDA); // Wait until data is received
    return U1RXREG;
}

void UART1_Echo(void) {
    char receivedChar = UART1_ReadChar();
    UART1_WriteChar(receivedChar); // Echo back the received character
}
int main(void) {
    UART1_Init(); // Initialize UART1
    while (1) {
        UART1_Echo(); // Continuously echo received characters
    }
    return 0;
}
*/

//ASSIGNMENT3 ADVANCED
int i = 0;
int led2 = 1;
int counter = 0; //a counter of how many chars you have received
char window[3];
int missed_deadlines = 0;

void __attribute__((__interrupt__, __auto_psv__)) _T1Interrupt() {
    IFS0bits.T1IF = 0; // reset interrupt flag
    i = i + 1;

    if (i == 20) {
        i = 0;

        if (led2 == 1) {
            LATGbits.LATG9 = !LATGbits.LATG9;
        }
    }
}

void __attribute__((__interrupt__, __auto_psv__)) _T2Interrupt(){
  IFS0bits.T2IF = 0; 
  IEC0bits.T2IE = 0;
  T2CONbits.TON = 0; 
  IEC1bits.INT1IE = 1;

  if (PORTEbits.RE8 == 1) {
      UART1_WriteChar(counter);
  }
}

void __attribute__((__interrupt__, __auto_psv__)) _INT1Interrupt(){
  IFS1bits.INT1IF = 0;
  IEC0bits.T2IE = 1;
  tmr_setup_period(TIMER2, 10); //bouncing
}

void __attribute__((__interrupt__, __auto_psv__)) _INT2Interrupt() {
    IFS1bits.INT2IF = 0;  // Reset flag
    IEC0bits.T2IE = 1;
    tmr_setup_period(TIMER2, 10); //bouncing
}

void UART1_Init(void) {
    // Configure RD11 as input (U1RX) and RD0 as output (U1TX)
    TRISDbits.TRISD11 = 1; // Set RD11 as input
    TRISDbits.TRISD0 = 0;  // Set RD0 as output
    
    // Perform UART1 pin remapping
    RPINR18bits.U1RXR = 75; // RD11 corresponds to RPI75
    RPOR0bits.RP64R = 1;    // RD0 corresponds to RP64, map it to U1TX (1)
    
    U1BRG = BRGVAL; // Set baud rate
    
    // Enable UART -- SET IT BEFORE
    U1MODEbits.UARTEN = 1;
    
    // THEN U CAN -- Enable Transmit and Receive
    U1STAbits.UTXEN = 1;
    U1STAbits.URXDA = 1;

}

void UART1_WriteChar(char c) {
    while (U1STAbits.UTXBF); // Wait if transmit buffer is full
    U1TXREG = c;
}

char UART1_ReadChar(void) {
    while (!U1STAbits.URXDA); // Wait until data is received
    counter++;
    return U1RXREG;
}

void UART1_Echo(void) {
    char receivedChar = UART1_ReadChar();
    updateWindow(receivedChar);
    UART1_WriteChar(receivedChar); // Echo back the received character
}

void updateWindow(char newChar) {
    // Shift a sinistra
    window[0] = window[1];
    window[1] = window[2];
    window[2] = newChar;

    // Controlla se la finestra contiene "LD1"
    if (window[0] == 'L' && window[1] == 'D' && window[2] == '1') {
        LATAbits.LATA0 = !LATAbits.LATA0; //toggle led1
    }
    else if (window[0] == 'L' && window[1] == 'D' && window[2] == '2') {
        led2 = !led2;
    }
}

void algorithm() {
    tmr_wait_ms(TIMER2, 7);
}

int main() {
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;

    //IEC0bits.T1IE = 1;

    TRISAbits.TRISA0 = 0;
    LATAbits.LATA0 = 0;
    TRISGbits.TRISG9 = 0;
    LATGbits.LATG9 = 0;
    
    INTCON2bits.GIE = 1; //abilita interrupt globali
    
    //T2 button
    TRISEbits.TRISE8 = 1;
    RPINR0bits.INT1R = 0x58;
    IFS1bits.INT1IF = 0;
    IEC1bits.INT1IE = 1;
    
    // Configura T3 (RE9) per generare un interrupt su INT2
    TRISEbits.TRISE9 = 1;  // T3 come input
    RPINR1bits.INT2R = 0x59;  // Mappa RE9 su INT2
    IFS1bits.INT2IF = 0;  // Reset flag INT2
    IEC1bits.INT2IE = 1;  // Abilita interrupt INT2
    
    int ret;
    
    UART1_Init(); // Initialize UART1

    tmr_setup_period(TIMER1, 10);
    //int count = 0;
    while (1) {
        algorithm(); 
        // code to handle the assignment
        UART1_Echo();
        i++;
        if (i == 20) {
            i = 0;

            if (led2 == 1) {
                LATGbits.LATG9 = !LATGbits.LATG9;
            }
        }
        
        ret = tmr_wait_period(TIMER1);
        
        if(ret){
            missed_deadlines++;
        }
    }
}
