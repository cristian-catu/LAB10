/* 
* File:   main.c
 * Author: Cristian Catú
 *
 * Comunicación serial
 * 
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000
/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
char tabla[10] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57};
char mensaje2[7] = {' ', ' ', ' ',  0x0D, 0x0A, 0x0D, 0x0A};
uint8_t indice = 0;             // Variable para saber que posición del mensaje enviar al serial
uint8_t entrada = 0;          // Variable para guardar la entrada
uint8_t bandera = 0;
uint8_t lectura_adc = 0; // variable para lectura ADC
uint8_t centenas = 0; // VARIABLES PARA LAS LECTURAS ADC
uint8_t decenas = 0;
uint8_t unidades = 0;
uint8_t centenas2 = 0;
uint8_t decenas2 = 0;
uint8_t unidades2 = 0;
uint8_t remainder = 0;
uint8_t indice2 = 0;

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);
void cadena(char *str);
void TX_usart(char data);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.RCIF){          // Hay datos recibidos?
        if(bandera == 0){ 
            entrada = RCREG;
            if(entrada != 0x31 & entrada != 0x32){ 
                cadena("ERROR \n\n");
            }
        }
        else if(bandera == 1){ 
            PORTB = RCREG;
            bandera = 0;
            indice = 0;
        }
    }
    if(PIR1bits.ADIF){ //Lectura ADC
        lectura_adc = ADRESH;
        PIR1bits.ADIF = 0;
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){
        if(indice == 0){              // evaluamos bandera indice
            cadena("1. Leer potenciometro\n 2. Enviar ascii \n \n"); // imprimimos cadena
            indice = 1;
        }
        if(entrada == 0x31){  // cuando la entrada es 1
            indice = 0;
            entrada = 0;
            centenas = lectura_adc/100; //Se obtienen las centenas al divir dentro de 100
            remainder = lectura_adc%100;
            decenas = remainder/10; //Se obtienen las decenas al divir el remainder dentro de 10
            unidades = remainder%10;
            centenas2 = tabla[centenas]; //se obtienen sus valores en ascii
            decenas2 = tabla[decenas];
            unidades2 = tabla[unidades];
            mensaje2[0] = centenas2;  //se ingresan a un arreglo
            mensaje2[1] = decenas2;
            mensaje2[2] = unidades2;
            indice2 = 0;
            while(indice2<7){              // Loop para imprimir el mensaje completo
                if (PIR1bits.TXIF){             // Esperamos a que esté libre el TXREG para poder enviar por el serial
                    TXREG = mensaje2[indice2];    // Cargamos caracter a enviar
                    indice2++;                   // Incrementamos indice para enviar sigiente caracter
                }
            }
            
        }
        else if(entrada == 0x32){  //cuando la entrada es 2
            cadena("Ingrese digito \n\n");
            entrada = 0;
            bandera = 1;
        }
        if(ADCON0bits.GO == 0){
            ADCON0bits.GO = 1;              // proceso de conversión  
        }
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    ANSEL = 0b00000001;
    ANSELH = 0;                 // I/O digitales
    TRISA = 0b00000001;
    TRISB = 0;
    PORTB = 0;
    
    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
    
    // Configuraciones de comunicacion serial
    //SYNC = 0, BRGH = 1, BRG16 = 1, SPBRG=25 <- Valores de tabla 12-5
    TXSTAbits.SYNC = 0;         // Comunicación ascincrona (full-duplex)
    TXSTAbits.BRGH = 1;         // Baud rate de alta velocidad 
    BAUDCTLbits.BRG16 = 1;      // 16-bits para generar el baud rate
    
    SPBRG = 25;
    SPBRGH = 0;                 // Baud rate ~9600, error -> 0.16%
    
    RCSTAbits.SPEN = 1;         // Habilitamos comunicación
    TXSTAbits.TX9 = 0;          // Utilizamos solo 8 bits
    TXSTAbits.TXEN = 1;         // Habilitamos transmisor
    RCSTAbits.CREN = 1;         // Habilitamos receptor
    
    // Configuraciones de interrupciones
    PIR1bits.ADIF = 0;          // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;          // Habilitamos interrupcion de ADC
    INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitamos interrupciones de perifericos
    PIE1bits.RCIE = 1;          // Habilitamos Interrupciones de recepción
    
    ADCON0bits.ADCS = 0b01;     // Fosc/8
    ADCON1bits.VCFG0 = 0;       // VDD
    ADCON1bits.VCFG1 = 0;       // VSS
    ADCON0bits.CHS = 0b0000;    // Seleccionamos el AN0
    ADCON1bits.ADFM = 0;        // Justificado a la izquierda
    ADCON0bits.ADON = 1;        // Habilitamos modulo ADC
    __delay_us(40);             // Sample time
}
void cadena(char *str){ // función para imprimir
    while(*str != '\0'){
        TX_usart(*str);
        str++;
    }
}
void TX_usart(char data){
    while(TXSTAbits.TRMT == 0);
    TXREG = data;
}