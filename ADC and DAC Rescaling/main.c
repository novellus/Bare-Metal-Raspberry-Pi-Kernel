#define gpio ((volatile unsigned int*)0x20200000)
volatile unsigned int* SET;
volatile unsigned int* CLR;
volatile unsigned int* READ;
unsigned int SCLK;
unsigend int MISO;
unsigned int MOSI;
unsigned int CS1;
unsigned int CS2;
volatile unsigned int i;

#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) INP_GPIO(g);*(gpio+((g)/10)) |=  (1<<(((g)%10)*3))

#define nop asm volatile("nop")
#define nop2 nop; nop
#define nop4 nop2; nop2
#define nop8 nop4; nop4
#define nop16 nop8; nop8
#define nop32 nop16; nop16

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

	//Only templated above, no changes to while(1) yet.
	while(1) {
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(SET), [data]"r"(SCLK)); //(*SET)=SCLK;
		nop;
		asm volatile("str %[data], [%[reg]]" : : [reg]"r"(CLR), [data]"r"(SCLK)); //(*CLR)=SCLK;
		nop;
	}
}

/*//LED Debug
gpio[1] |= (1 << 18); //Set LED as output
gpio[10] = (1 << 16); //LED On
gpio[7] = (1 << 16); //LED Off*/
