#include <16F877A.h>


#FUSES HS, NOWDT, NOBROWNOUT, NOLVP
#use delay(clock=16000000)

//LCD
#define lcd_enable  PIN_E1  // Pino 9
#define lcd_rs      PIN_E2  // Pino 10
#define lcd_rw      PIN_E0  // Pino 8
#define lcd_d4      PIN_D4  // Pino 27
#define lcd_d5      PIN_D5  // Pino 28
#define lcd_d6      PIN_D6  // Pino 29
#define lcd_d7      PIN_D7  // Pino 30

#include "mod_lcd.c"

// BOTÕES E BUZZER
#define BTN_MENU  PIN_B0 
#define BTN_UP    PIN_B1 
#define BTN_DOWN  PIN_B2 
#define BUZZER    PIN_C2 

// globals
int8 hour = 12, min = 0, sec = 0;
int8 alarm_h = 12, alarm_m = 1; //std alarme
int1 alarm_on = 0;
int1 alarm_triggered = 0;
int8 menu_state = 0;
int8 ticks = 0;
int8 i = 0; //nao declarar variaveis dentro do loop, use global


//INTERRUPÇÃO DO TIMER 1 (Base de tempo de 100ms)
#INT_TIMER1
void timer1_isr() {
    // Recarrega o Timer1 para 100ms com cristal de 16MHz e Prescaler 8
    set_timer1(15536); 
    ticks++;
    
    // A cada 10 ticks de 100ms, temos 1 segundo exato
    if (ticks >= 10) { 
        ticks = 0;
        sec++;
        if (sec >= 60) {
            sec = 0;
            min++;
            if (min >= 60) {
                min = 0;
                hour++;
                if (hour >= 24) hour = 0;
            }
        }
    }
}

void main() {
    
    setup_adc_ports(NO_ANALOGS); 
    setup_adc(ADC_OFF); 
    setup_comparator(NC_NC_NC_NC);
    
    
    port_b_pullups(TRUE); 
    output_low(BUZZER);  
    
    // setup Timer1
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    set_timer1(15536);
    enable_interrupts(INT_TIMER1);
    enable_interrupts(GLOBAL);
    
    // Pequeno atraso para a tensão estabilizar antes de comunicar com o LCD
    delay_ms(100); 
    
    // Inicialização do LCD
    lcd_ini(); 
    delay_ms(50); 
    
    // Mensagem inicialização
    printf(lcd_escreve, "\f Relogio PIC \n Init...");
    delay_ms(1500);
    lcd_escreve('\f'); 
    
    //Loop Principal (Máquina de Estados)
    while(TRUE) {
        
        // Verifica Botão MENU
        if (!input(BTN_MENU)) {
            delay_ms(200); // Debounce
            menu_state++;
            if (menu_state > 5) menu_state = 0; 
            lcd_escreve('\f'); 
        }
        
        // Verifica Botão CIMA 
        if (!input(BTN_UP)) {
            delay_ms(200); // Debounce
            if (menu_state == 1) { hour++; if(hour >= 24) hour = 0; }
            if (menu_state == 2) { min++; if(min >= 60) min = 0; }
            if (menu_state == 3) { alarm_h++; if(alarm_h >= 24) alarm_h = 0; }
            if (menu_state == 4) { alarm_m++; if(alarm_m >= 60) alarm_m = 0; }
            if (menu_state == 5) { alarm_on = !alarm_on; }
            
            // desliga alarme tocando
            if (alarm_triggered) { alarm_triggered = 0; output_low(BUZZER); } 
        }
        
        // Verifica Botão BAIXO
        if (!input(BTN_DOWN)) {
            delay_ms(200); // Debounce
            if (menu_state == 1) { if(hour == 0) hour = 23; else hour--; }
            if (menu_state == 2) { if(min == 0) min = 59; else min--; }
            if (menu_state == 3) { if(alarm_h == 0) alarm_h = 23; else alarm_h--; }
            if (menu_state == 4) { if(alarm_m == 0) alarm_m = 59; else alarm_m--; }
            if (menu_state == 5) { alarm_on = !alarm_on; }
            
            if (alarm_triggered) { alarm_triggered = 0; output_low(BUZZER); } 
        }

        // trigger alarme
        if (alarm_on && hour == alarm_h && min == alarm_m && sec == 0) {
            alarm_triggered = 1;
        }

        // Atualização da Tela de acordo com o Menu atual
        if (menu_state == 0) {
            lcd_pos_xy(1, 1);
            printf(lcd_escreve, "Hora: %02u:%02u:%02u ", hour, min, sec);
            lcd_pos_xy(1, 2);
            if(alarm_on)
                printf(lcd_escreve, "Alarme: %02u:%02u  ", alarm_h, alarm_m);
            else
                printf(lcd_escreve, "Alarme: OFF    ");
        } else if (menu_state == 1) {
            lcd_pos_xy(1, 1); printf(lcd_escreve, "Ajustar Hora:  ");
            lcd_pos_xy(1, 2); printf(lcd_escreve, "-> %02u:%02u       ", hour, min);
        } else if (menu_state == 2) {
            lcd_pos_xy(1, 1); printf(lcd_escreve, "Ajustar Minuto:");
            lcd_pos_xy(1, 2); printf(lcd_escreve, "-> %02u:%02u       ", hour, min);
        } else if (menu_state == 3) {
            lcd_pos_xy(1, 1); printf(lcd_escreve, "Alarme Hora:   ");
            lcd_pos_xy(1, 2); printf(lcd_escreve, "-> %02u:%02u       ", alarm_h, alarm_m);
        } else if (menu_state == 4) {
            lcd_pos_xy(1, 1); printf(lcd_escreve, "Alarme Minuto: ");
            lcd_pos_xy(1, 2); printf(lcd_escreve, "-> %02u:%02u       ", alarm_h, alarm_m);
        } else if (menu_state == 5) {
            lcd_pos_xy(1, 1); printf(lcd_escreve, "Status Alarme: ");
            lcd_pos_xy(1, 2); 
            if(alarm_on) printf(lcd_escreve, "-> ON          ");
            else         printf(lcd_escreve, "-> OFF         ");
        }
        
        // logica de som buzzer psv
        if (alarm_triggered) {
            // Usa a variável 'ticks' para criar a intermitência
            if (ticks < 5) {
                for(i = 0; i < 50; i++) {
                    output_high(BUZZER);
                    delay_us(500); 
                    output_low(BUZZER);
                    delay_us(500);
                }
            } else {
                output_low(BUZZER);
                delay_ms(100);
            }
        } else {
            output_low(BUZZER);
            delay_ms(100); 
        }
    }
}
