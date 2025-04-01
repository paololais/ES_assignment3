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
int counter = 0; // Contatore dei caratteri ricevuti via UART
char char_counter;
char window[3]; // Array per memorizzare gli ultimi tre caratteri ricevuti

// Interrupt per il Timer 1
void __attribute__((__interrupt__, __auto_psv__)) _T1Interrupt() {
    IFS0bits.T1IF = 0; // Reset del flag di interrupt di Timer 1
    i = i + 1;

    // Dopo 20 tick (200ms se ogni tick = 10ms), lampeggia il LED
    if (i == 20) {
        i = 0;
        if (led2 == 1) {
            LATGbits.LATG9 = !LATGbits.LATG9; // Toggle LED2
        }
    }
}

// Interrupt per il Timer 2
void __attribute__((__interrupt__, __auto_psv__)) _T2Interrupt(){
  IFS0bits.T2IF = 0; // Reset del flag di interrupt di Timer 2
  IEC0bits.T2IE = 0; // Disabilita l'interrupt di Timer 2
  T2CONbits.TON = 0; // Ferma il Timer 2
  IEC1bits.INT1IE = 1; // Riabilita l'interrupt su INT1

  if (PORTEbits.RE8 == 1) { // Se il pulsante è premuto
      char_counter = (char) counter;
      UART1_WriteChar(char_counter); // Invia il conteggio dei caratteri ricevuti
  }
}

// Interrupt per INT1 (pulsante)
void __attribute__((__interrupt__, __auto_psv__)) _INT1Interrupt(){
  IFS1bits.INT1IF = 0; // Reset del flag di interrupt
  IEC0bits.T2IE = 1; // Abilita l'interrupt del Timer 2
  tmr_setup_period(TIMER2, 10); // Imposta il debounce del pulsante
}

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
    counter++; // Incrementa il contatore dei caratteri ricevuti
    return U1RXREG;
}

// Funzione di echo (reinvia il carattere ricevuto)
void UART1_Echo(void) {
    char receivedChar = UART1_ReadChar(); // Legge un carattere
    updateWindow(receivedChar); // Aggiorna la finestra di rilevamento comandi
    UART1_WriteChar(receivedChar); // Reinvia il carattere ricevuto
}

// Funzione per aggiornare la finestra e rilevare comandi speciali
void updateWindow(char newChar) {
    window[0] = window[1];
    window[1] = window[2];
    window[2] = newChar;

    // Controllo della sequenza ricevuta
    if (window[0] == 'L' && window[1] == 'D' && window[2] == '1') {
        LATAbits.LATA0 = !LATAbits.LATA0; // Toggle LED1
    }
    else if (window[0] == 'L' && window[1] == 'D' && window[2] == '2') {
        led2 = !led2; // Toggle variabile per LED2
    }
}

// Algoritmo con ritardo artificiale
void algorithm() {
    tmr_wait_ms(TIMER2, 7); // Aspetta 7ms
}

int main() {
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000; // Disabilita gli ingressi analogici

    IEC0bits.T1IE = 1; // Abilita l'interrupt di Timer 1

    TRISAbits.TRISA0 = 0; // Configura LED1 come output
    LATAbits.LATA0 = 0; // Spegne LED1
    TRISGbits.TRISG9 = 0; // Configura LED2 come output
    LATGbits.LATG9 = 0; // Spegne LED2
    
    // Configurazione pulsante su RE8
    TRISEbits.TRISE8 = 1; // Configura RE8 come input
    RPINR0bits.INT1R = 0x58; // Mappatura INT1 su RE8
    INTCON2bits.GIE = 1; // Abilita gli interrupt globali
    IFS1bits.INT1IF = 0; // Reset flag interrupt INT1
    IEC1bits.INT1IE = 1; // Abilita interrupt INT1
    
    int ret;
    UART1_Init(); // Inizializza UART1
    tmr_setup_period(TIMER1, 10); // Configura Timer 1 con periodo di 10ms
    
    while (1) {
        algorithm(); // Esegue un ritardo di 7ms
        UART1_Echo(); // Gestisce la comunicazione UART
        ret = tmr_wait_period(TIMER1); // Aspetta il prossimo tick di Timer 1. Questa funzione aspetta che scada il periodo impostato per Timer1.
                                       //Se il timer NON è ancora scaduto, ret sarà 0.
                                       //Se il timer è scaduto, ret sarà 1, segnalando che il ciclo può proseguire.
        
    }
}
