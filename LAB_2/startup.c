/*
 *     startup.c
 *
 */
#define STK_CTRL ((volatile unsigned int *)(0xE000E010))  
#define STK_LOAD ((volatile unsigned int *)(0xE000E014))  
#define STK_VAL ((volatile unsigned int *)(0xE000E018))
#define GPIO_E 0x40021000
#define GPIO_OTYPER ((volatile unsigned int *) (GPIO_E+0x04))
#define GPIO_MODER ((volatile unsigned int *) (GPIO_E))
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_E+0x14))
#define GPIO_OSPEEDR ((volatile unsigned int *) (GPIO_E+0x08))

__attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");        /* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");                    /* call main */
__asm__ volatile(".L1: B .L1\n");                /* never return */
}

void delay_250ns( void )
{
    /* SystemCoreClock = 168000000 */
    *STK_CTRL = 0;
    *STK_LOAD = ( (168/4) -1 );
    *STK_VAL = 0;
    *STK_CTRL = 5;
    while( (*STK_CTRL & 0x10000 )== 0 );
	*STK_CTRL = 0;
}

void delay_micro(unsigned int us)
{
    while( us > 0 )
    {
        delay_250ns();
        delay_250ns();
        delay_250ns();
        delay_250ns();
        us--;
    }
}


void app_init(){
	* ( ( unsigned long * ) 0x40023830 ) = 0x18;
    *GPIO_MODER = 0x55005555;
	*GPIO_OTYPER = 0x0;
	*GPIO_OSPEEDR = 0xFF;
}

void delay_milli(unsigned int ms) {
    //#ifdef SIMULATOR
    //    ms = ms / 1000;
    //    ms++;
    //#endif
    delay_micro(ms * 100);
}
void main(void)
{
    app_init();
    while (1) {
        *GPIO_ODR_LOW = 0;
        delay_milli(500);
        *GPIO_ODR_LOW = 0xFF;
        delay_milli(500);
    }
}