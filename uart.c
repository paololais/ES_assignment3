/*
 * File:   uart.c
 * Author: paolo
 *
 * Created on 1 aprile 2025, 9.44
 */


#include "uart.h"

// Inizializzazione UART1
void UART1_Init(void) {
    TRISDbits.TRISD11 = 1; // Imposta RD11 come input (U1RX)
    TRISDbits.TRISD0 = 0;  // Imposta RD0 come output (U1TX)
    
    RPINR18bits.U1RXR = 75; // RD11 è mappato su U1RX
    RPOR0bits.RP64R = 1;    // RD0 è mappato su U1TX
    
    U1BRG = BRGVAL; // Configura il baud rate
    
    U1MODEbits.UARTEN = 1; // Abilita UART1
    U1STAbits.UTXEN = 1; // Abilita la trasmissione
    U1STAbits.URXDA = 1; // Abilita la ricezione
}

// Scrive un carattere sulla UART1
void UART1_WriteChar(char c) {
    while (U1STAbits.UTXBF); // Attende se il buffer di trasmissione è pieno
    U1TXREG = c; // Invia il carattere
}

// Legge un carattere dalla UART1
char UART1_ReadChar(void) {
    while (!U1STAbits.URXDA); // Attende finché non riceve un carattere
    //counter++; // Incrementa il contatore dei caratteri ricevuti
    return U1RXREG;
}

// Funzione di echo (reinvia il carattere ricevuto)
void UART1_Echo(void) {
    char receivedChar = UART1_ReadChar(); // Legge un carattere
    updateWindow(receivedChar); // Aggiorna la finestra di rilevamento comandi
    UART1_WriteChar(receivedChar); // Reinvia il carattere ricevuto
}