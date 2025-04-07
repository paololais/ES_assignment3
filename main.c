/*
 * File:   main.c
 * Author: paolo
 *
 * Created on March 12, 2025, 3:46 PM
 */
#include "xc.h"
#include "timer.h"
#include "uart.h"

#define BAUDRATE 9600UL
#define FCY 72000000UL  
#define BRGVAL ((FCY / (16 * BAUDRATE)) - 1)
#define BUFFER_SIZE 32 // da calcolare in base a quanti dati ricevo/trasmetto

typedef struct {
    char buffer[BUFFER_SIZE]; // Array che contiene i dati
    int head; // Indice di scrittura
    int tail; // Indice di lettura
    int count; // Numero di elementi nel buffer
} CircularBuffer;

void cb_init(CircularBuffer *cb) {
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

void cb_push(CircularBuffer *cb, char value) {
    
    cb->buffer[cb->head] = value; // Scrive il valore
    cb->head = (cb->head + 1) % BUFFER_SIZE; // Incremento circolare
    if (cb->count == BUFFER_SIZE){
        cb->tail = (cb->tail + 1) % BUFFER_SIZE;
        return; // Buffer pieno, non incremento count
    }
    cb->count++;
}

void cb_pop(CircularBuffer *cb, char *value) {
    *value = cb->buffer[cb->tail]; // Legge il valore
    cb->tail = (cb->tail + 1) % BUFFER_SIZE; // Incremento circolare
    cb->count--;
}

int cb_is_empty(CircularBuffer *cb) {
    return cb->count == 0;
}

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
int led2 = 1;
int counter_char = 0; // Contatore dei caratteri ricevuti via UART
int missed_deadlines = 0; // variabile per tenere traccia delle deadlines mancate
                              // a seconda del return di wait_period

typedef enum {IDLE, S_L, S_LD} UART_State;
UART_State uartState = IDLE;
CircularBuffer cb;

// ? Interrupt UART RX
void __attribute__((__interrupt__, __auto_psv__)) _U1RXInterrupt() {
    char receivedChar = U1RXREG; // Legge carattere ricevuto
    cb_push(&cb, receivedChar);
    //handle_UART_FSM(receivedChar); // ora gestisco con la pop in processReceivedData())
    counter_char++; // Incrementa contatore caratteri
    IFS0bits.U1RXIF = 0; // Reset flag interrupt
}

// Interrupt per il Timer 3
void __attribute__((__interrupt__, __auto_psv__)) _T3Interrupt(){
  IFS0bits.T3IF = 0; // Reset del flag di interrupt di Timer 3
  IEC0bits.T3IE = 0; // Disabilita l'interrupt di Timer 3
  T3CONbits.TON = 0; // Ferma il Timer 3
  IEC1bits.INT1IE = 1; // Riabilita l'interrupt su INT2

  if (PORTEbits.RE8 == 1) { // Se il pulsante è premuto
      UART1_WriteChar((char) counter_char); // Invia il conteggio dei caratteri ricevuti
  }
}
// Interrupt per il Timer 4
void __attribute__((__interrupt__, __auto_psv__)) _T4Interrupt(){
  IFS1bits.T4IF = 0; // Reset del flag di interrupt di Timer 3
  IEC1bits.T4IE = 0; // Disabilita l'interrupt di Timer 3
  T4CONbits.TON = 0; // Ferma il Timer 3
  IEC1bits.INT2IE = 1; // Riabilita l'interrupt su INT2

  if (PORTEbits.RE9 == 1) { // Se il pulsante è premuto
      UART1_WriteChar((char) missed_deadlines); // Invia il conteggio dei caratteri ricevuti
  }
}
// Interrupt per INT1 (pulsante)
void __attribute__((__interrupt__, __auto_psv__)) _INT1Interrupt(){
  IFS1bits.INT1IF = 0; // Reset del flag di interrupt
  IEC0bits.T3IE = 1; // Abilita l'interrupt del Timer 3
  tmr_setup_period(TIMER3, 10); // Imposta il debounce del pulsante
}

// Interrupt per INT2 (pulsante)
void __attribute__((__interrupt__, __auto_psv__)) _INT2Interrupt(){
  IFS1bits.INT2IF = 0; // Reset del flag di interrupt
  IEC1bits.T4IE = 1; // Abilita l'interrupt del Timer 4
  tmr_setup_period(TIMER4, 10); // Imposta il debounce del pulsante
}

// Funzione che elabora i caratteri dal buffer circolare
void processReceivedData() {
    char receivedChar;
    
    // Se ci sono caratteri nel buffer
    while (!cb_is_empty(&cb)) {
        IEC0bits.U1RXIE = 0;   // disabilita interrupt RX
        // Pop del carattere dal buffer
        cb_pop(&cb, &receivedChar);
        IEC0bits.U1RXIE = 1;   // Abilita interrupt RX
        
        handle_UART_FSM(receivedChar); // Gestisce il carattere in base alla FSM
    }
}

void handle_UART_FSM(char receivedChar) {
    switch (uartState) {
        case IDLE:
            if (receivedChar == 'L') uartState = S_L;
            break;
        case S_L:
            if (receivedChar == 'D') uartState = S_LD;
            else uartState = IDLE;
            break;
        case S_LD:
            if (receivedChar == '1') {
                LATAbits.LATA0 = !LATAbits.LATA0; // Toggle LED1
            } else if (receivedChar == '2') {
                led2 = !led2; // Toggle LED2
            }
            uartState = IDLE; // Reset stato
            break;
    }
}

// Algoritmo con ritardo artificiale
void algorithm() {
    tmr_wait_ms(TIMER2, 7); // Aspetta 7ms
}

int main() {
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000; // Disabilita gli ingressi analogici

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
    
    // pulsante su RE9
    TRISEbits.TRISE9 = 1;        // Configura RE9 come input
    RPINR1bits.INT2R = 0x59;     // Mappatura INT2 su RE9
    IFS1bits.INT2IF = 0;         // Reset flag interrupt INT2
    IEC1bits.INT2IE = 1;         // Abilita interrupt INT2
    
    int ret;
    int i = 0;
    
    cb_init(&cb);
    
    UART1_Init(); // Inizializza UART1
    tmr_setup_period(TIMER1, 10); // Configura Timer 1 con periodo di 10ms
    
    
    while (1) {
        algorithm(); // Esegue un ritardo di 7ms
        
        //blink led2
        i = i + 1;
        // Dopo 20 tick (200ms se ogni tick = 10ms), lampeggia il LED
        if (i == 20) {
            i = 0;
            if (led2 == 1) {
                LATGbits.LATG9 = !LATGbits.LATG9; // Toggle LED2
            }
        }
        
        // circular buffer -> calcolare dimensione corretta in base a quanti dati arrivano e a che frequenza
        // uart read dal circular buffer
        // interrupt per read e write uart
         processReceivedData(); // Elabora i dati ricevuti nel buffer circolare -> pop
        
        ret = tmr_wait_period(TIMER1); // Aspetta il prossimo tick di Timer 1. Questa funzione aspetta che scada il periodo impostato per Timer1.
                                       //Se il timer NON è ancora scaduto, ret sarà 0.
                                       //Se il timer è scaduto, ret sarà 1, segnalando che il ciclo può proseguire.
        if(ret) missed_deadlines++;
    }
}
