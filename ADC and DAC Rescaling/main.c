#define gpio ((volatile unsigned int*)0x20200000)
volatile unsigned int* SET;
volatile unsigned int* CLR;
volatile unsigned int* READ;
unsigned int SCLK;
unsigned int MISO;
unsigned int MOSI;
unsigned int CS1;
unsigned int CS2;
int adcData;
volatile unsigned int i;

#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) INP_GPIO(g);*(gpio+((g)/10)) |=  (1<<(((g)%10)*3))

#define nop asm volatile("nop")
#define nop2 nop; nop
#define nop4 nop2; nop2
#define nop8 nop4; nop4
#define nop16 nop8; nop8
#define nop32 nop16; nop16

//control and range registers for AD7322 (ADC)
#define conReg 0b1000001000010000
#define ranReg 0b1010000000000000

int main(void) {

	//Enable level 1 cache, and the branch predictor! (This gives a roughly 2x speedup, although I've been told to be careful with this.)
	asm volatile ("MRC p15, 0, %0, c1, c0, 0" : "=r" (i));
	i|=0x1800; 
	asm volatile ("MCR p15, 0, %0, c1, c0, 0" :: "r" (i));


	SET=gpio+7;
	CLR=gpio+10;
	READ=gpio+13;

	SCLK=1<<11;
	MISO=1<<9;
	MOSI=1<<10;
	CS1=1<<8;
	CS2=1<<7;
	
	OUT_GPIO(11); //SCLK
	INP_GPIO(9); //MISO
	OUT_GPIO(10); //MOSI
	OUT_GPIO(8); //CS1
	OUT_GPIO(7); //CS2

	//Initial conditions CS's and SCLK high
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS1)); //CS's are active low.
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS2));

	nop32; //Settling delay

	//Configure ADC registers
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS1));
	for(i=15;i>=0;i--) {
		if(conReg&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI));}
		else                   {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI));}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK)); //Data is clocked into the ADC on falling edges.
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	}
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS1));
	nop16; //Delay twixt register writes
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS1));
	for(i=15;i>=0;i--) {
		if(ranReg&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI));}
		else                   {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI));}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK));
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	}
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS1));


	while(1) {
		adcData=0;

		//Read Value from ADC into adcData
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI)); //Clear data out line to disable accidental register writes on the ADC.
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS1));
		for(i=15;i>=0;i--) {
			asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK));
			asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
			adcData&=((((*READ)&MISO)>>10)<<i); //RBF Optimise?
		}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS1));

		//=============================================BEGIN DEBUG====================================================//
		// Output adcData
		for(i=15;i>=0;i--) {
				asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK));
				if(adcData&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI));}
				else               {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI));}
				asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
		}
		//=============================================End DEBUG======================================================//

		adcData=(adcData&0x3FFE)<<2; //zero out the ZERO, address, and useless LSB; then format like a signed integer.

		//Output data to the DAC (MAX5312)
		

		nop2; //Delay between samples
	}
}

/*//LED Debug
gpio[1] |= (1 << 18); //Set LED as output
gpio[10] = (1 << 16); //LED On
gpio[7] = (1 << 16); //LED Off*/
