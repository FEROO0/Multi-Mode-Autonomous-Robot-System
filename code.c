/*******************************************************
This program was created by the
CodeWizardAVR V3.14 Advanced
Automatic Program Generator
© Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : 
Version : 
Date    : 4/22/2026
Author  : 
Company : 
Comments: 


Chip type               : ATmega32A
Program type            : Application
AVR Core Clock frequency: 11.059200 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*******************************************************/

#include <mega32a.h>
#include <delay.h>
#include <stdio.h>
#define RIGHT_SENSOR PINC.4
#define CENTER_SENSOR PINC.5
#define LEFT_SENSOR PINC.6
#define speed 250


// PWM initialization
void PWM_init()
{
    DDRB |= (1<<PORTB3);   // ENA
    DDRD |= (1<<PORTD7);   // ENB

    // Timer0 Fast PWM
    TCCR0 = (1<<WGM00)|(1<<WGM01)|(1<<COM01)|(1<<CS01);

    // Timer2 Fast PWM
    TCCR2 = (1<<WGM20)|(1<<WGM21)|(1<<COM21)|(1<<CS21);

    OCR0 = 200;
    OCR2 = 200;
}

volatile unsigned char mode = 0;
volatile unsigned char toggle_request = 0;
volatile unsigned char lock = 0;
unsigned int space_counter = 0;
unsigned char parked = 0;
char detection_enabled = 0;
int reverse_timeout = 0;


interrupt [EXT_INT0] void ext_int0_isr(void)
{
    if(lock == 0)
    {
        toggle_request = 1;
        lock = 1;
    }
}

void UART_init()
{
    // RX enable + TX enable
    UCSRB = (1<<RXEN) | (1<<TXEN);

    // 8-bit
    UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);

    // 9600 baud
    UBRRH = 0;
    UBRRL = 71;
}

char UART_read()
{
    while(!(UCSRA & (1<<RXC)));
    return UDR;
}

void ADC_init()
{
    ADMUX = 0x07;
    ADCSRA = 0x87;
}

unsigned int ADC_read()
{
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC));
    return ADCW;
}


void UART_send(char data)
{
    while(!(UCSRA & (1<<UDRE)));

    UDR = data;
}

float battery_voltage(void)
{
    unsigned int adc_value;
    float adc_voltage;
    float battery_v;

    adc_value = ADC_read();
    adc_voltage = (adc_value * 5.0) / 1023.0;
    battery_v = adc_voltage * 2.14;
    return battery_v;
}

unsigned char battery_percent(float voltage)
{
    unsigned char percent;

    if(voltage >= 12.0)
        percent = 100;
    else if(voltage <= 9.0)
        percent = 0;
    else
        percent =
        (unsigned char)(((voltage - 9.0) / 3.0) * 100.0);

    return percent;  
}

// Forward
void move_forward()
{

    OCR0 = 200;
    OCR2 = 200;
    
    PORTC.0=1;
    PORTC.1=0;
    PORTC.2=1;
    PORTC.3=0;
}


// Backward
void move_backward()
{
    PORTC.0=0;
    PORTC.1=1;
    PORTC.2=0;
    PORTC.3=1;
}


// Right
void move_right()
{
    PORTC.0=0;
    PORTC.1=1;
    PORTC.2=1;
    PORTC.3=0;
}


// Left
void move_left()
{
    PORTC.0=1;
    PORTC.1=0;
    PORTC.2=0;
    PORTC.3=1;
}


// Stop
void stop_motors()
{
    PORTC=0x00;
}

void beep()
{
    PORTB.1 = 0;
    delay_ms(200);
    PORTB.1 = 1;
}


// ================= MOTOR FUNCTIONS =================

void line_forward()
{
    PORTC.0 = 1;
    PORTC.1 = 0;

    PORTC.2 = 1;
    PORTC.3 = 0;
}

void line_left()
{
    PORTC.0 = 0;
    PORTC.1 = 0;

    PORTC.2 = 1;
    PORTC.3 = 0;
}

void line_right()
{
    PORTC.0 = 1;
    PORTC.1 = 0;

    PORTC.2 = 0;
    PORTC.3 = 0;
}

void line_stop()
{
    PORTC.0 = 0;
    PORTC.1 = 0;

    PORTC.2 = 0;
    PORTC.3 = 0;
}

void scan_right()
{
    PORTC.0 = 1;
    PORTC.1 = 0;

    PORTC.2 = 0;
    PORTC.3 = 1;
}

void scan_left()
{
    PORTC.0 = 0;
    PORTC.1 = 1;

    PORTC.2 = 1;
    PORTC.3 = 0;
}
   
float ultrasonic_read()
{
    unsigned int count = 0;
    unsigned int timeout = 0;
    float distance;

    // Trigger pulse
    PORTA.0 = 1;
    delay_us(10);
    PORTA.0 = 0;

    // Echo
    while(PINA.1 == 0)
    {
        timeout++;
        if(timeout > 60000)
            return 999;
    }

    timeout = 0;

    // Echo
    while(PINA.1 == 1)
    {
        count++;
        delay_us(5);

        timeout++;
        if(timeout > 60000)
            break;
    }

    distance = (count / 10.8) * 1.03;

    return distance;
}


float ultrasonic_side_read()
{
    unsigned int count = 0;
    unsigned int timeout = 0;
    float distance;

    PORTA.2 = 1;
    delay_us(10);
    PORTA.2 = 0;

    while(PINA.3 == 0)
    {
        timeout++;
        if(timeout > 60000)
            return 999;
    }

    timeout = 0;

    while(PINA.3 == 1)
    {
        count++;
        delay_us(5);

        timeout++;
        if(timeout > 60000)
            break;
    }

    distance = (count / 10.8) * 1.03;

    return distance;
}

float ultrasonic_back_read()
{
    unsigned int count = 0;
    unsigned int timeout = 0;
    float distance;

    PORTA.4 = 1;
    delay_us(10);
    PORTA.4 = 0;

    while(PINA.5 == 0)
    {
        timeout++;
        if(timeout > 60000)
            return 999;
    }

    timeout = 0;

    while(PINA.5 == 1)
    {
        count++;
        delay_us(5);

        timeout++;
        if(timeout > 60000)
            break;
    }

    distance = (count / 10.8) * 1.03;

    return distance;
}

void perform_parking()
{
    float d_back;
    reverse_timeout = 0;
    
    stop_motors();
    delay_ms(300);

    // Step 1: turn left
    move_left();
    delay_ms(900);

    stop_motors();
    delay_ms(200);

    // Step 2: reverse until obstacle detected
    while(reverse_timeout < 50)
{
    d_back = ultrasonic_back_read();

    if(d_back < 18 && d_back > 2)
    {
        break;
    }

    move_backward();

    delay_ms(30);

    reverse_timeout++;
}

    stop_motors();
    beep();

    delay_ms(300);

    // Step 3: adjust right
     move_right();
    delay_ms(750);

    stop_motors();
    parked = 1;
}

void detect_parking_space()
{
    float d_front;
    float d_side;

    d_front = ultrasonic_read();
    d_side  = ultrasonic_side_read();

    if(parked)
    {
    stop_motors();
    return;
    }

    if(d_front < 20 && d_front > 5)
    {
        stop_motors();
        beep();
        delay_ms(50);
        beep();
        delay_ms(50);
        beep();

        delay_ms(400);
        return;
    }

    if(d_side > 25)
    {
        space_counter++;

        if(space_counter > 40)
    {
     

        perform_parking();
         stop_motors();
        beep(); 
        delay_ms(100);
        beep();     
        delay_ms(400);
    }
        else
        {
            move_forward();
        }
    }
    else
    {
        space_counter = 0;
        move_forward();
    }
}


void line_follow_mode()
{   
        // ================= CENTER =================
        // 0 1 0

        if(!LEFT_SENSOR &&
           CENTER_SENSOR &&
          !RIGHT_SENSOR)
        {
            OCR0 = speed;
            OCR2 = speed;

            line_forward();
        }

        // ================= LEFT =================
        // 1 0 0

        else if(LEFT_SENSOR &&
               !CENTER_SENSOR &&
               !RIGHT_SENSOR)
        {
            OCR0 = speed;
            OCR2 = speed;

            line_left();
        }

        // 1 1 0

        else if(LEFT_SENSOR &&
                CENTER_SENSOR &&
               !RIGHT_SENSOR)
        {
            OCR0 = speed;
            OCR2 = speed;

            line_left();
        }

        // ================= RIGHT =================
        // 0 0 1

        else if(!LEFT_SENSOR &&
                !CENTER_SENSOR &&
                 RIGHT_SENSOR)
        {
            OCR0 = speed;
            OCR2 = speed;

            line_right();
        }

        // 0 1 1

        else if(!LEFT_SENSOR &&
                 CENTER_SENSOR &&
                 RIGHT_SENSOR)
        {
            OCR0 = speed;
            OCR2 = speed;

            line_right();
        }

        // ================= LINE LOST =================

else
{
    unsigned int i;

    for(i=0; i<100; i++)
    {
        // ================= RIGHT SCAN =================

        scan_right();

        delay_ms(10);

        // ?? ??? ????
        if((!LEFT_SENSOR && CENTER_SENSOR && !RIGHT_SENSOR) ||
           (LEFT_SENSOR && !CENTER_SENSOR && !RIGHT_SENSOR) ||
           (LEFT_SENSOR && CENTER_SENSOR && !RIGHT_SENSOR) ||
           (!LEFT_SENSOR && !CENTER_SENSOR && RIGHT_SENSOR) ||
           (!LEFT_SENSOR && CENTER_SENSOR && RIGHT_SENSOR))
        {
            break;
        }

        // ================= LEFT SCAN =================

        scan_left();

        delay_ms(20);

        // ?? ??? ????
        if((!LEFT_SENSOR && CENTER_SENSOR && !RIGHT_SENSOR) ||
           (LEFT_SENSOR && !CENTER_SENSOR && !RIGHT_SENSOR) ||
           (LEFT_SENSOR && CENTER_SENSOR && !RIGHT_SENSOR) ||
           (!LEFT_SENSOR && !CENTER_SENSOR && RIGHT_SENSOR) ||
           (!LEFT_SENSOR && CENTER_SENSOR && RIGHT_SENSOR))
        {
            break;
        }
    }

    // ================= IF LINE FOUND =================

    if((!LEFT_SENSOR && CENTER_SENSOR && !RIGHT_SENSOR) ||
       (LEFT_SENSOR && !CENTER_SENSOR && !RIGHT_SENSOR) ||
       (LEFT_SENSOR && CENTER_SENSOR && !RIGHT_SENSOR) ||
       (!LEFT_SENSOR && !CENTER_SENSOR && RIGHT_SENSOR) ||
       (!LEFT_SENSOR && CENTER_SENSOR && RIGHT_SENSOR))
    {
        return;
    }

    // ================= NO LINE FOUND =================

    line_stop();

}
}

void uart_control()
{
    char data;

    if(UCSRA & (1<<RXC))
    {
        data = UART_read();

        // ================= MODES =================

        if(data == 'M')
        {
            mode = 0;

            stop_motors();
        }

        else if(data == 'P')
        {
            mode = 1;

            detection_enabled = 0;
            parked = 0;
            space_counter = 0;
        }

        else if(data == 'T')
        {
            mode = 2;

            stop_motors();
        }

        // ================= MANUAL =================

        else if(mode == 0)
        {
            if(data == 'F')
            {
                move_forward();
            }

            else if(data == 'B')
            {
                move_backward();
            }

            else if(data == 'R')
            {
                move_right();
            }

            else if(data == 'L')
            {
                move_left();
            }

            else if(data == 'S')
            {
                stop_motors();
            }
            
            else if(data == 'O')
            {
                if(parked)
                {
                    move_left();
                    delay_ms(700);
                    move_forward();
                    delay_ms(500);
                    move_right();
                    delay_ms(700);
                    stop_motors();
                    parked = 0;
                    mode = 0;
                }
            }
        }
    }
}


void main()
{
    DDRC = 0x0F;
    
    PORTC.4 = 1;
    PORTC.5 = 1;
    PORTC.6 = 1;
    
    DDRC.4 = 0;
    DDRC.5 = 0;
    DDRC.6 = 0;
   
    PWM_init();
    UART_init();
    ADC_init();
    
    delay_ms(1000);
    toggle_request = 0;
    mode = 0;

DDRA.2 = 1;   // TRIG side sensor
DDRA.3 = 0;   // ECHO side sensor
PORTA.3 = 0;
DDRA.4 = 1;
DDRA.5 = 0;
PORTA.5 = 0;


DDRD.5 = 1;
DDRD.4 = 1;

PORTB.1 = 1;  //pull-up beep
DDRB.1 = 1;   //beep

DDRA.0 = 1;   // TRIG output
DDRA.1 = 0;   // ECHO input
PORTA.1 = 0;  // disable pullup

DDRD &= ~(1<<PORTD2);   // input
PORTD |= (1<<PORTD2);   // pull-up

MCUCR |= (1<<ISC01); // falling edge trigger
MCUCR &= ~(1<<ISC00);

GICR |= (1<<INT0); GIFR |= (1<<INTF0);   // enable INT0

#asm("sei")          // enable global interrupt




while(1)
{
    float battery_v;
    unsigned char battery_p;

    battery_v = battery_voltage();

    battery_p = battery_percent(battery_v);

    UART_send('A');
    UART_send(battery_p);
    uart_control();    

    if(PIND.2 == 1)
    {
        lock = 0;
    }

    if(toggle_request)
    {  
    beep();
    mode++;

        if(mode > 2)
        mode = 0;

    toggle_request = 0;
    }

     
   switch(mode)
    {
        case 0: 
        break;

        case 1:

            if(!detection_enabled)
            {
                move_forward();

                delay_ms(300);

                detection_enabled = 1;
            }
            else
            {
                detect_parking_space();
            }

        break;

        case 2:

            line_follow_mode();

        break;
    }  
}
}
