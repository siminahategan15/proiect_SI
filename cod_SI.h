#include "stm32f4xx.h"
#include<string.h>
#include<stdio.h>

//-------------------------------------------------------------------------------------------------------------------------------------
// VARIABILE GLOBALE
//-------------------------------------------------------------------------------------------------------------------------------------


#define RS 1    /* masca pentru register select */
#define EN 2    /* masca for E */

#define NUMAR_MAXIM_CARACTERE_PE_RAND 16
#define MAX_PASSWORD_LENGTH 20
#define PASSWORD "223356" // Parola corecta



char user_password[MAX_PASSWORD_LENGTH + 1]; // Parola introdusa de utilizator

int pozitie_cursor = 0;
int caractere_scrise = 0; // Numarul de caractere scrise
int numar_incercari = 3; // Numarul inițial de încercari
int isLocked=0;

//tabel care mapeaza butoanele de pe tastatura la valorile asociate
const char keypad_values[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Definirea starii sistemului
typedef enum {
    LOCKED,
    UNLOCKED
} SystemState;

SystemState system_state = LOCKED; // Starea inițiala a sistemului este blocata


//-------------------------------------------------------------------------------------------------------------------------------------
//  BUTOANE
//-------------------------------------------------------------------------------------------------------------------------------------

void configure_buttons(void);
int is_SW5_pressed(void);
void wait_for_SW5_press(void);
void configure_PC8(void);

//-------------------------------------------------------------------------------------------------------------------------------------
//  TASTATURA
//-------------------------------------------------------------------------------------------------------------------------------------

char keypad_getkey(void);
void outputEnableCols(char n);
void writeCols(char n);
int readRows(void);
void keypad_init(void);
char get_keypad_value(int row, int col);

//-------------------------------------------------------------------------------------------------------------------------------------
//  LED-URI
//-------------------------------------------------------------------------------------------------------------------------------------

void writeLEDs(char n);
void update_LEDs(void);

//-------------------------------------------------------------------------------------------------------------------------------------
//  LCD
//-------------------------------------------------------------------------------------------------------------------------------------

void LCD_nibble_write(char data, unsigned char control);
void LCD_command(unsigned char command);
void LCD_data(char data);
void LCD_init(void);


//-------------------------------------------------------------------------------------------------------------------------------------
//  SISTEM
//-------------------------------------------------------------------------------------------------------------------------------------

void lock_system(void);
void unlock_system(void);
void check_password(char *input);
void delayMs(int n);
void delay(void);
void SPI1_write(unsigned char data);








