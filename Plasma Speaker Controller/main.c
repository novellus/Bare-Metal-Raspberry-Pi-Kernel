#define gpio ((volatile unsigned int*)0x20200000)
volatile unsigned int* SET;
volatile unsigned int* CLR;
volatile unsigned int* READ;
unsigned int SCLK;
unsigned int MISO;
unsigned int MOSI1;
unsigned int MOSI2;
unsigned int CS;

unsigned short adcInput;
unsigned short dacOutput;
int ZERO;
int i;

#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) INP_GPIO(g);*(gpio+((g)/10)) |=  (1<<(((g)%10)*3))

//Nop's 'are necessary to meet timing requirements.
#define nop asm volatile("nop")
#define nop2 nop; nop
#define nop4 nop2; nop2
#define nop8 nop4; nop4
#define nop16 nop8; nop8
#define nop32 nop16; nop16
#define nop64 nop32; nop32
#define nop128 nop64; nop64
#define nop256 nop128; nop128
#define nop512 nop256; nop256

//control and range registers for AD7322 (ADC)
#define conReg 0b1000001000110000
#define ranReg 0b1011000100000000
//Control bits for DAC
#define conBits 0x4000

//2.5V reference on ADC and DAC (MAX5312). ADC scales +-2.5V, and DAC scales 0->5V.
//Linear function on data input: output=input/fctnA+fctnB; This should scale +-2V to 3V+-0.5V
#define fctnA 4
#define fctnB 0x799

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
	MOSI1=1<<10;
	MOSI2=1<<7;
	CS=1<<8;
	
	OUT_GPIO(11); //SCLK
	INP_GPIO(9); //MISO
	OUT_GPIO(10); //MOSI1
	OUT_GPIO(7); //MOSI2
	OUT_GPIO(8); //CS

	//Initial conditions CS and SCLK high
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS)); //CS is active low.

	nop512; //Settling delay

	//Configure ADC registers
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS));
	for(i=15;i>=0;i--) {
		if(conReg&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI1));}
		else              {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI1));}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK)); //Data is clocked into the ADC on falling edges.
		nop8;
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	}
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS));
	nop512; //Delay twixt register writes
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS));
	for(i=15;i>=0;i--) {
		if(ranReg&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI1));}
		else              {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI1));}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK));
		nop8;
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	}
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS));


	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI1)); //Clear data out line to ADC to prevent further register writes.
	dacOutput=0;
	ZERO=0;
	while(1) {
		adcInput=0;

		//Read Value from ADC into adcInput; output dacOutput to DAC.
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS));
		for(i=15;i>=0;i--) {
			asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK));
			if(dacOutput&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI2));} //Output to DAC
			else                 {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI2));} //Output to DAC
			nop64;
			asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
			nop64;
			adcInput=adcInput|(((*READ)&MISO)?1<<i:0); //Input from ADC
		}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS));

		dacOutput=(adcInput>>2)/fctnA+fctnB+conBits; //Align number & apply linear function.
		if(ZERO) { //Every other output is zero.
			ZERO=0;
			dacOutput=0x4000;
		}
		else {
			ZERO=1;
			nop; //nop to maintain timing.
		}
		nop512; //Delay between samples

	}
}

/*//LED Debug
gpio[1] |= (1 << 18); //Set LED as output
gpio[10] = (1 << 16); //LED On
gpio[7] = (1 << 16); //LED Off*/
