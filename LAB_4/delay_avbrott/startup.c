/*
 * 	startup.c
 *
 */
__attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");		/* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");					/* call main */
__asm__ volatile(".L1: B .L1\n");				/* never return */
}

__attribute__( ( naked ) ) void enable_interrupt( void ) {
	__asm volatile (" CPSIE I\n");
	__asm volatile (" BX	LR\n");
}

__attribute__( ( naked ) ) void disable_interrupt( void ) {
	__asm volatile (" CPSID I\n");
	__asm volatile (" BX	LR\n");
		
}

#define GPIO_D 0x40020C00 // Port D
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_D+0x14)) // Minst signifikanta bitarna (0-7) på Output Data Register
#define GPIO_ODR_HIGH ((volatile unsigned char *) (GPIO_D+0x15))
#define GPIO_MODER ((volatile unsigned int *) (GPIO_D))

#define STK_CTRL ((volatile unsigned int *) 0xE000E010   )
#define STK_LOAD ((volatile unsigned int *) 0xE000E014   )
#define STK_VAL ((volatile unsigned int *) 0xE000E018    )

#define SCB_VTOR ((volatile unsigned long *) 0xE000ED08)

#ifdef SIMULATOR
#define	DELAY_COUNT		100
#else
#define DELAY_COUNT		1000
#endif

void	init_app( void )
{
	*GPIO_MODER = 0x55555555;
	/* starta klockor port D och E */
	* ( (unsigned long *) 0x40023830) = 0x18;
	/* starta klockor för SYSCFG */
	* ((unsigned long *)0x40023844) |= 0x4000; 	
	/* Relokera vektortabellen */
	* SCB_VTOR = 0x2001C000; /* Avkommentera ifall detta körs på fysisk hårdvara */
}

void delay_1mikro( void )
{
	*STK_CTRL = 0;
	*STK_LOAD = ( 168 - 1);
	*STK_VAL = 0;
	*STK_CTRL = 7;
}

static volatile int systick_flag;
static volatile int delay_count;

void systick_irq_handler( void )
{
	*STK_CTRL = 0;
	delay_count -- ;
	if( delay_count > 0) delay_1mikro();
	else systick_flag = 1;
}

void delay( unsigned int count )
{
	if( count == 0 ) return;
	delay_count = count;
	systick_flag = 0;
	delay_1mikro();
}

void main(void)
{
	init_app();
	unsigned char c;
	* ((void (**) (void) ) 0x2001C03C) = systick_irq_handler;	/* Initiera avbrottsvektorn */
	*GPIO_ODR_LOW = 0;
	delay( DELAY_COUNT );
	*GPIO_ODR_LOW = 0xFF;
	while(1)
	{
		if( systick_flag ) /* När avbrottet hanterats av systick_irq_handler så sätts systick_flag = 1 och omslutande while-loop bryts */
			break;
			/* Här placeras kod som kan utföras under väntetiden */
		*GPIO_ODR_HIGH = c;
		c ++;
	}
	/* Här finns den kod som "väntar" på time-out */
	*GPIO_ODR_LOW = 0;
}

