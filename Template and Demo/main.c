volatile unsigned int* gpio;
volatile unsigned int* SET;
volatile unsigned int* CLR;
unsigned int SCLK;
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


	gpio=(volatile unsigned int*)0x20200000;
	SET=gpio+7;
	CLR=gpio+10;

	SCLK=1<<11;
	OUT_GPIO(11); //SCLK

	//Demos ~22MHz signaling. 
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