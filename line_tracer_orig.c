#include <avr/io.h>
#include <avr/interrupt.h>

typedef unsigned int WORD;
typedef unsigned char BYTE;

#define SenserNum 3
#define BAUD_38400 25

WORD speed_L;
WORD speed_R;
WORD ADC_Data[SenserNum+1];

int state;
int count_R,count_L,CrossCount,StopState;  
int Left_Sum,Right_Sum,Center_Sum;
//int count_R = 0;
//int count_L = 0;

//unsigned char left_motor[8] = { 0x55, 0x11, 0x99, 0x88, 0xaa, 0x22, 0x66, 0x44};
//unsigned char right_motor[8]
//unsigned char left_motor[8] = { 0x11,0x55,0x44,0x66,0x22,0xaa,0x88,0x99}; //1-2 Phase Control Method
//unsigned char right_motor[8] = { 0x99,0x88,0xaa,0x22,0x66,0x44,0x55,0x11};
unsigned char left_motor[8] = {0x99,0x88,0xcc,0x44,0x66,0x22,0x33,0x11};
unsigned char right_motor[8] = {0x11,0x33,0x22,0x66,0x44,0xcc,0x88,0x99};

// ??? ???????????????? ???????????????
//unsigned char left_motor_count;
//unsigned char right_motor_count;

void PORT_init(void)
{
   DDRE = DDRF = DDRG = 0; //Set port to input direction first.
   PORTA = PORTB = PORTC = PORTD = PORTE = PORTF = PORTG = 0x00; // 
   DDRA = 0XFF;
   DDRB = 0xFF;
   DDRC = 0xFF;
   DDRE = 0xFF;
   DDRD = 0xFF;
   PORTC = 0xFF;
   PORTD = 0xFF;
}

void ADC_init(void)
{
   ADMUX = 0;
   ADCSRA = (1<<ADEN)|(1<<ADFR)|(1<<ADSC)|(1<<ADIE)|(1<<ADPS2)|(ADPS1)|(1<<ADPS0);

   for(int i =0; i<SenserNum; i++)
   {
      ADC_Data[i]=0;
   }
}

void Timer_init(void)
{
   TCCR1B = (1<<CS10);
   TCNT1 = speed_R;

   TCCR3B = (1<<CS30);
   TCNT3 = speed_L;
    
   TIMSK = (1<<TOIE1);
   ETIMSK = (1<<TOIE3);

}

void Delay_us(unsigned char time_us)
{
     register unsigned char i;

     for(i = 0; i < time_us; i++) {
        asm (" PUSH R0 ");
        asm (" POP R0 ");
        asm (" PUSH R0 ");
        asm (" POP R0 ");
        asm (" PUSH R0 ");
        asm (" POP R0 ");
    }
}

void Delay_ms(unsigned int time_ms)
{
    register unsigned int i;

    for(i = 0; i < time_ms; i++)
    {
        Delay_us(250);
        Delay_us(250);
        Delay_us(250);
        Delay_us(250);
    }
}

void Set_Speed(void)
{

    Left_Sum = ADC_Data[1];
    Center_Sum = ADC_Data[2];
    Right_Sum = ADC_Data[3];

    if(Left_Sum > 800 && Center_Sum < 550 && Right_Sum < 500) { //Left Corner
        speed_L = 30000;
        speed_R = 50000;
        state=1;
}
else if(Left_Sum > 800 && Center_Sum > 550 && Right_Sum < 500) { //Left
        speed_L = 35000;
        speed_R = 50000;
        state=2;
}
else if(Left_Sum < 500 && Center_Sum < 550 && Right_Sum > 800 ) { //Right Corner
        speed_L = 50000;
        speed_R = 30000;
        state=3;
}
else if(Left_Sum < 500 && Center_Sum > 550 && Right_Sum > 800) { //Right
        speed_L = 50000;
        speed_R = 35000;
        state=4;
}

else if(Center_Sum > 800) //Straight
    {
        speed_L = 50000;
        speed_R = 50000;
        state=5;
    }
else                     //Stop
    {
        speed_L = 0;
        speed_R = 0;
        state=6;
    }
}
void USART_init(unsigned int baud0,unsigned int baud1)
{
   UBRR0H = (unsigned char)(baud0 >> 8);
   UBRR0L = (unsigned char)baud0;

   UCSR0B = (0<<RXEN0)|(1<<TXEN0)|(0 << RXCIE0)|(0 << TXCIE0);
   UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);

   UBRR1H = (unsigned char)(baud1 >> 8);
   UBRR1L = (unsigned char)baud1;

   UCSR1B = (0<<RXEN1)|(1<<TXEN1)|(0<<RXCIE1)|(0<<TXCIE1);
   UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);
}

void Txd0Byte(unsigned char datum)
{
   while(!(UCSR0A & (1<<UDRE0)));
   UDR0 = datum;
}
void Txd1Byte(unsigned char datum)
{
   while(!(UCSR1A & (1<<UDRE1)));
   UDR1 = datum;
}
unsigned char Rxd0Byte(void)
{
   while(!(UCSR0A & (1 << RXC0)));
   return UDR0;
}
unsigned char Rxd1Byte(void)
{
   while(!(UCSR1A & (1 << RXC1)));
   return UDR1;
}
void Txd0String(unsigned char *str)
{
   int i;
   for( i=0; str[i]!=0; i++)
      Txd0Byte( str[i]);
}
void Txd1String(unsigned char *str)
{
   int i;
   for( i=0; str[i]!=0; i++)
      Txd1Byte( str[i]);
}

void Txd0Dec(int dec)
{
   char String[5];
   int loop;

   for (loop = 0 ; loop <5 ; loop++)
   {
      String[loop] = 0x30 + (dec % 10);
      dec = dec / 10;
   }
   for(loop = 4; loop >= 0; loop --)
      Txd0Byte(String[loop]);
}
void Txd1Dec(int dec)
{
   char String[5];
   int loop;

   for (loop = 0 ; loop <5 ; loop++)
   {
      String[loop] = 0x30 + (dec % 10);
      dec = dec / 10;
   }
   for(loop = 4; loop >= 0; loop --)
      Txd1Byte(String[loop]);
}
int main (void)
{
   USART_init(BAUD_38400,BAUD_38400);
   PORT_init();
   Timer_init();
   ADC_init();
   
    count_R = 0;
    count_L = 0;
    CrossCount = 0;
    StopState = 0;
    state = 0;

    sei();

   for(;;)   {
      Delay_ms(100);
      Txd1Dec(ADC_Data[3]);
      Txd1Byte(' ');
      Txd1Dec(ADC_Data[2]);
      Txd1Byte(' ');
      Txd1Dec(ADC_Data[1]);
      Txd1Byte(' ');
//      Txd1Dec(ADC_Data[4]);
//      Txd1Byte(' ');
//      Txd1Dec(ADC_Data[5]);
//      Txd1Byte(' ');
      Txd0Dec(ADC_Data[3]);
      Txd0Byte(' ');
        Txd0Dec(ADC_Data[2]);
      Txd0Byte(' ');
        Txd0Dec(ADC_Data[1]);
//      Txd0Byte(' ');
//      Txd0Dec(ADC_Data[4]);
//      Txd0Byte(' ');
//      Txd0Dec(ADC_Data[5]);
      Txd0Byte(' ');
        if(state==1) Txd1String("Left Corner");
        else if (state==2)  Txd1String("Left");
        else if (state==3)  Txd1String("Right Corner");
        else if (state==4)  Txd1String("Right");
		else if (state==5)  Txd1String("Straight");
        else Txd1String("Stop");
        Txd1Byte('\n');
   }

   return 0;
}

SIGNAL(SIG_OVERFLOW1)
{
    PORTB = right_motor[count_R++];
    count_R %=8;

    Set_Speed();
    TCNT1 = speed_R;
}

SIGNAL(SIG_OVERFLOW3)
{
    PORTC= left_motor[count_L++];
    count_L %= 8;

    Set_Speed();
   TCNT3 = speed_L;
}
SIGNAL(SIG_ADC)
{
   ADC_Data[ADMUX++] = ADC ; 
//  ADC_Data[SenserNum-1] = ADC_Data[0];

   ADCSRA = (1<<ADEN)|(1<<ADFR)|(1<<ADSC)|(1<<ADIE)|(1<<ADPS2)|(ADPS1)|(1<<ADPS0);

   ADMUX %= SenserNum+1;
}
////////////////////////////////////////////////
