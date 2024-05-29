#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>
#include "cod_SI"s


//-------------------------------------------------------------------------------------------------------------------------------------
//  BUTOANE
//-------------------------------------------------------------------------------------------------------------------------------------

void configure_buttons(void) {
    // Configurarea pinilor pentru coloane (PB12 - PB15) ca iesiri si initializarea lor la nivel înalt
    GPIOB->MODER &= ~(0xFF000000);  // Resetarea bitilor de mod de operare (MODER) pentru PB12 - PB15
    GPIOB->MODER |= 0x55000000;  // Setarea bitilor MODER corespunzatori PB12 - PB15 ca iesiri (01)

    // Configurarea pinilor pentru rânduri (PC8 - PC11) ca intrari si activarea rezistentelor de pull-down
    GPIOC->MODER &= ~(0xFF00);  // Resetarea bitilor de mod de operare (MODER) pentru PC8 - PC11
    GPIOC->PUPDR &= ~(0xFF00);  // Resetarea bitilor de rezistente de pull-up/pull-down (PUPDR) pentru PC8 - PC11
    GPIOC->PUPDR |= 0x5500;  // Setarea bitilor PUPDR corespunzatori PC8 - PC11 la 01 (pull-down)
}

int is_SW5_pressed(void) {
    return (GPIOC->IDR & (1 << 8)); // Verifica daca pinul PC8 (SW5) este la nivel jos 
}

void wait_for_SW5_press(void) {
    while (is_SW5_pressed()) {} // Asteapta pâna când butonul SW5 este eliberat
    while (!is_SW5_pressed()) {} // Asteapta pâna când butonul SW5 este apasat
    delayMs(10);  // Asteapta 10 msecunde pentru a citi mesajul
}

void configure_PC8(void)
{
    RCC->AHB1ENR |=  2;             
    RCC->AHB1ENR |=  4;

    GPIOB->MODER &= ~0x0000ff00;    
    GPIOB->MODER |=  0x00005500;    
    GPIOC->MODER &= ~0x00FF0000;    
}

//-------------------------------------------------------------------------------------------------------------------------------------
// TASTATURA
//-------------------------------------------------------------------------------------------------------------------------------------

char keypad_getkey(void)
{
    int row, col;
    outputEnableCols(0xF);     
    writeCols(0xF);           
    delay();                  
    row = readRows();        
    writeCols(0x0);             
    outputEnableCols(0x0);      // dezactiveaza coloanele
    if (row == 0) return 0;     // daca nu e nicio tasta apasata

    //daca este apasata o tasta
     // activeaza cate o coloana si citeste randurile
    for (col = 0; col < 4; col++) {
        outputEnableCols(1 << col); // activeaza o coloana
        writeCols(1 << col);        
        delay();                    
        row = readRows();           // citeste randurile
        writeCols(0x0);             
        if (row != 0) break;       
    }

    outputEnableCols(0x0);          // dezactiveaza coloanele
    if (col == 4)
        return 0;                   //nicio tasta nu e apasata

    // daca vreun rand are vreo tasta apasata
    if (row == 0x01) return 0 + col;    // tasta in randul 0
    if (row == 0x02) return 4 + col;    // tasta in randul 1
    if (row == 0x04) return 8 + col;    // tasta in randul 2
    if (row == 0x08) return 12 + col;   // tasta in randul 3 
		
    return 0; 
}

// activeaza coloanele dupa bitii 3-0 ai lui n
void outputEnableCols(char n) {
    GPIOB->MODER &= ~0xFF000000;   

	 if (n & 1)
        GPIOB->MODER |=  0x01000000;
    if (n & 2)
        GPIOB->MODER |=  0x04000000;
    if (n & 4)
        GPIOB->MODER |=  0x10000000;
    if (n & 8)
        GPIOB->MODER |=  0x40000000;
}

// pune coloanele pe HIGH sau LOW dupa bitii 3-0 ai lui n
void writeCols(char n) {
    GPIOB->BSRR = 0xF0000000;   
    GPIOB->BSRR = ((uint32_t)n << 12) | (n & 1); 
}

// citeste randurile
int readRows(void) {
	return (GPIOC->IDR & 0x0F00) >> 8;
}

// initializare pini la tastatura 
void keypad_init(void) {
    RCC->AHB1ENR |=  4;          
    GPIOC->MODER &= ~0x00FF0000;    

    RCC->AHB1ENR |=  2;            
    GPIOB->MODER &= ~0xFF000000;    
}

// Functia care va întoarce valoarea asociata butonului apasat
char get_keypad_value(int row, int col) {
    return keypad_values[row][col];
}


//-------------------------------------------------------------------------------------------------------------------------------------
// LED-URI
//-------------------------------------------------------------------------------------------------------------------------------------


// bitii 3-0 ai lui n pentru stingere/aprindere LED-uri 
void writeLEDs(char n) {
    GPIOB->BSRR = 0x00F00000;   // stingere Led-uri
    GPIOB->BSRR = n << 4;       // aprindere Led-uri
}

// Functia de actualizare a LED-urilor în functie de starea sistemului
void update_LEDs(void) {
    if (system_state == LOCKED) {
        // Daca sistemul este blocat, aprinde LED-urile
        GPIOB->BSRR = 0x000000FF;   
    } else {
        // Daca sistemul este deblocat, stinge LED-urile
        GPIOB->BSRR = 0x00F00000;   
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------
// LCD
//-------------------------------------------------------------------------------------------------------------------------------------

void LCD_nibble_write(char data, unsigned char control) {
    data &= 0xF0;     
    control &= 0x0F;  
    SPI1_write (data | control);         
    SPI1_write (data | control | EN);      
    delayMs(10);        // Asteapta 10 ms între scrierea fiecarui caracter
    // Verifica pozitia cursorului pentru a asigura afisarea pe un singur rând
    if (pozitie_cursor < NUMAR_MAXIM_CARACTERE_PE_RAND) {
        SPI1_write(data);  // Scrie datele pe ecran doar daca cursorul este pe primul rând
        pozitie_cursor++;
    } else if (pozitie_cursor < (NUMAR_MAXIM_CARACTERE_PE_RAND * 2)) {
        // Adauga conditia pentru a permite scrierea pe al doilea rând
        SPI1_write(data | RS | 0x40);  // Seteaza bitul RS pentru a scrie pe al doilea rând
        pozitie_cursor++;
    }
    // Incrementam numarul de caractere scrise pe ecran
    caractere_scrise++;
}

void LCD_command(unsigned char command) {
    LCD_nibble_write(command & 0xF0, 0);    
    LCD_nibble_write(command << 4, 0);      
    if (command < 4)
        delayMs(2);        
    else
        delayMs(1);         
}

void LCD_data(char data) {
    GPIOA->BSRR = RS;
    LCD_nibble_write(data & 0xF0, RS);      
    LCD_nibble_write(data << 4, RS);        
    delayMs(10);
}

void LCD_init(void) {
    RCC->AHB1ENR |= 1;          
    RCC->AHB1ENR |= 4;             
    RCC->APB2ENR |= 0x1000;         

    GPIOA->MODER &= ~0x0000CC00;   
    GPIOA->MODER |=  0x00008800;   
    GPIOA->AFR[0] &= ~0xF0F00000;  
    GPIOA->AFR[0] |=  0x50500000;  

    GPIOA->MODER &= ~0x03000000;    
    GPIOA->MODER |=  0x01000000;   

    SPI1->CR1 = 0x31F;
    SPI1->CR2 = 0;
    SPI1->CR1 |= 0x40;              

    delayMs(20);
    LCD_nibble_write(0x30, 0);
    delayMs(5);
    LCD_nibble_write(0x30, 0);
    delayMs(1);
    LCD_nibble_write(0x30, 0);
    delayMs(1);
    LCD_nibble_write(0x20, 0);  
    delayMs(1);
    LCD_command(0x28);          // seteaza datele pe 4 biti, pe 2 linii, font de 5x7
    LCD_command(0x06);          // muta cursorul in dreapta
    LCD_command(0x01);          // sterge ecranul
    LCD_command(0x0F);          
		
	pozitie_cursor = 0; // initializeaza pozitia cursorului
    caractere_scrise = 0; // initializeaza numarul de caractere scrise
}

//-------------------------------------------------------------------------------------------------------------------------------------
// SISTEM
//-------------------------------------------------------------------------------------------------------------------------------------

// blocare sistem
void lock_system(void) {
    // Afiseaza un mesaj pe LCD pentru a indica ca sistemul este blocat
    LCD_command(0x01);  // sterge ecranul
    LCD_command(0x02);  // repozitioneaza cursorul
    LCD_data('S');
    LCD_data('y');
    LCD_data('s');
    LCD_data('t');
    LCD_data('e');
    LCD_data('m');
    LCD_data(' ');
    LCD_data('b');
    LCD_data('l');
    LCD_data('o');
    LCD_data('c');
    LCD_data('k');
    LCD_data('e');
    LCD_data('d');
    LCD_data('!');
		
    // Aprinde toate LED-urile în ro?u
    GPIOB->BSRR = 0x000000FF;   
    LCD_command(0x01); 
	delayMs(200);	 
}

// deblocarea sistemului 
void unlock_system(void) {
    // Afiseaza un mesaj pe LCD pentru a indica ca sistemul este deblocat
    LCD_command(0x01);  // sterge ecranul
    LCD_command(0x02);  // repozitioneaza cursorul
    LCD_data('S');
    LCD_data('y');
    LCD_data('s');
    LCD_data('t');
    LCD_data('e');
    LCD_data('m');
    LCD_data(' ');
    LCD_data('u');
    LCD_data('n');
    LCD_data('l');
    LCD_data('o');
    LCD_data('c');
    LCD_data('k');
    LCD_data('e');
    LCD_data('d');
    LCD_data('!');
    
    // Stinge toate LED-urile
    GPIOB->BSRR = 0x00F00000;  
}

void check_password(char *input) {
	LCD_command(0x01);
    if (strcmp(input, PASSWORD) == 0) {
        // Daca parola este corecta, deblocheaza sistemul si reseteaza numarul de încercari
        unlock_system();
        numar_incercari = 3;
		LCD_data('c');
		LCD_data('o');
		LCD_data('r');
		LCD_data('r');
		LCD_data('e');
		LCD_data('c');
		LCD_data('t');			
    } else {
        // Daca parola este incorecta, afiseaza mesaj de eroare si decrementeaza numarul de încercari
        // Aprinde LED-urile 
        LCD_data('w');
		LCD_data('r');
		LCD_data('o');
		LCD_data('n');
		LCD_data('g');
			
        numar_incercari--;
		GPIOB->BSRR = 0x000000FF; 
		LCD_command(0x01);
		delayMs(200);
		if (numar_incercari == 0) {
            // Daca numarul de încercari a scazut sub sau egal cu 0, blocheaza sistemul
            lock_system();
			isLocked=1;
        } else {

			LCD_data('t');
			LCD_data('r');
			LCD_data('y');
			LCD_data(' ');
			LCD_data('a');
			LCD_data('g');
			LCD_data('a');
			LCD_data('i');
			LCD_data('n');
			LCD_command(0x01);
		}
    }
}

void delayMs(int n) {
    int i;
    for (; n > 0; n--)
        for (i = 0; i < 3195; i++) ;
}

// 16 MHz, delay de 100 us 
void delay(void) {
	int j;

	for (j = 0; j < 300; j++)
		;      // asteaptă
}

void SPI1_write(unsigned char data) {
    while (!(SPI1->SR & 2)) {}     
    GPIOA->BSRR = 0x10000000;      
    SPI1->DR = data;               
    while (SPI1->SR & 0x80) {}     
    GPIOA->BSRR = 0x00001000;      
}

//-------------------------------------------------------------------------------------------------------------------------------------
//  PROGRAM PRINCIPAL
//-------------------------------------------------------------------------------------------------------------------------------------

int main(void) {

    LCD_init();
    configure_buttons();
	configure_PC8();
    lock_system(); // Blocheaza sistemul initial
	delayMs(100);
	LCD_command(0x01);
	delayMs(100);

    // Afisare mesaj de introducere a parolei
    LCD_data('E');
    LCD_data('n');
    LCD_data('t');
    LCD_data('e');
    LCD_data('r');
    LCD_data(' ');
    LCD_data('p');
    LCD_data('a');
    LCD_data('s');
    LCD_data('s');
    LCD_data('w');
    LCD_data('o');
    LCD_data('r');
    LCD_data('d');
    LCD_data(':');
		
	delayMs(100);
		
	LCD_command(0x01);
	delayMs(200);
    
    char input_password[MAX_PASSWORD_LENGTH + 1]; // +1 pentru caracterul NULL terminator
	int numar_caractere_introduse = 0; // Numarul de caractere introduse de utilizator

    //initializare led-uri pentru display
    RCC->AHB1ENR |=  2;             
    GPIOB->MODER &= ~0x0000ff00;    
    GPIOB->MODER |=  0x00005500;    
		
	char a[20];

    // În bucla principala, asteapta introducerea parolei si verifica parola
    while(1) {
        // Verificam starea sistemului
        if (system_state == LOCKED && isLocked==0) {

            char key = keypad_getkey();  // Citeste butonul apasat de la tastatura matriceala
			delayMs(10);
			if (key >= 1 && key <= 15) {

            switch (key) {
                case 1:
                        user_password[numar_caractere_introduse] = '2'; // Adauga butonul la parola introdusa
						numar_caractere_introduse++;
						LCD_data('2');
                break;
				case 2:
                        user_password[numar_caractere_introduse] = '3'; 
				        numar_caractere_introduse++;
				        LCD_data('3');
                break;
				case 3:
                        user_password[numar_caractere_introduse] = 'A'; 
						numar_caractere_introduse++;
						LCD_data('A');
				break;
        		case 5:
                        user_password[numar_caractere_introduse] = '5'; 
						numar_caractere_introduse++;
						LCD_data('5');
                break;
                case 6:
                        user_password[numar_caractere_introduse] = '6';
						numar_caractere_introduse++;
						LCD_data('6');					
            	break;
				case 7:
                        user_password[numar_caractere_introduse] = 'B'; 
						numar_caractere_introduse++;
						LCD_data('B');							
                break;
				case 9:
                        user_password[numar_caractere_introduse] = '8'; 
						numar_caractere_introduse++;
						LCD_data('8');
                break;
				case 10:
                        user_password[numar_caractere_introduse] = '9'; 
						numar_caractere_introduse++;
						LCD_data('9');
                break;
				case 11:
                        user_password[numar_caractere_introduse] = 'C'; 
						numar_caractere_introduse++;
						LCD_data('C');
                break;
				case 13:
                        user_password[numar_caractere_introduse] = '0'; 
						numar_caractere_introduse++;
						LCD_data('0');
                break;
				case 14: 
                        user_password[numar_caractere_introduse] = '#'; 
						numar_caractere_introduse++;
						LCD_data('#');				
                break;
				case 15:              
                        user_password[numar_caractere_introduse] = 'D'; // Adauga butonul la parola introdusa
						numar_caractere_introduse++;
						LCD_data('D');					
                break;
                default:
                break;
					}					
			}
            if (is_SW5_pressed()) {
                // Daca utilizatorul a apasat butonul SW5, verifica parola introdusa
                check_password(user_password);
                numar_caractere_introduse = 0; // Reseteaza numarul de caractere introduse pentru a incepe din nou
				writeLEDs(key);
            }
		}
	}
}