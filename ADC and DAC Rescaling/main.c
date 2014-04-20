#define gpio ((volatile unsigned int*)0x20200000)
volatile unsigned int* SET;
volatile unsigned int* CLR;
volatile unsigned int* READ;
unsigned int SCLK;
unsigned int MISO;
unsigned int MOSI1;
unsigned int MOSI2;
unsigned int CS;

volatile short adcInput;
volatile unsigned short dacOutput;
volatile int i;

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
#define ranReg 0b1011000100000001 /*rbf, last 1 should be 0*/


//2.5V reference on ADC and DAC (MAX5312). ADC scales +-2.5V, and DAC scales 0->5V.
//Linear function on data input: output=input*fctnA+fctnB; This should scale +-2V to 3V+-0.5V
#define fctnA 0.25
#define fctnB 3*(0xFFFF/5)

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

	//nop32; //Settling delay

	//Configure ADC registers
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS));
	for(i=15;i>=0;i--) {
		if(conReg&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI1));}
		else              {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI1));}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK)); //Data is clocked into the ADC on falling edges.
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	}
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS));
	nop16; //Delay twixt register writes
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS));
	for(i=15;i>=0;i--) {
		if(ranReg&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI1));}
		else              {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI1));}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK));
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
	}
	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS));


	asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI1)); //Clear data out line to ADC to prevent further register writes.
	dacOutput=0;
	while(1) {
		adcInput=0;

		//Read Value from ADC into adcInput; output dacOutput to DAC.
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(CS));
		for(i=15;i>=0;i--) {
			asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK));
			if(dacOutput&(1<<i)) {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(MOSI2));} //Output to DAC
			else                 {asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(MOSI2));} //Output to DAC
			asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK));
			adcInput|=((((*READ)&MISO)>>10)<<i); //RBF Optimise? //Input from ADC
		}
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(CS));

		//Right and left shift here so the processor will take care of two's complement
		dacOutput=((unsigned short) (((int) ((adcInput&0x3FFE)<<2)*fctnA)+fctnB))>>4; //zero out the ZERO, address, and useless LSB; then format like a signed integer. Apply linear function.
		dacOutput=0x9249;
		nop2; //Delay between samples
	}
}

/*//LED Debug
gpio[1] |= (1 << 18); //Set LED as output
gpio[10] = (1 << 16); //LED On
gpio[7] = (1 << 16); //LED Off*/
