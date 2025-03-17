/*
 * File:   main.c
 * Author: paolo
 *
 * Created on March 12, 2025, 3:46 PM
 */


#include "xc.h"
#include "timer.h"

int main(void) {
    
    TRISDbits.TRISD11 = 1;
    RPINR18bits.U1RXR = 75;
    
    TRISDbits.TRISD0 = 0;
    RPOR0bits.RP64R = 1;
    
    U1BRG = 11; // (7372800 / 4) / (16 ? 9600)? 1
    U1MODEbits.UARTEN = 1; // enable UART
    U1STAbits.UTXEN = 1; // enable U1TX (must be after UARTEN)
    U1TXREG = ?C?; // send ?C?

    return 0;
}
