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

#define GPIO_D 0x40020C00 // Port D
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_D+0x14)) // Minst signifikanta bitarna (0-7) på Output Data Register
#define GPIO_ODR_HIGH ((volatile unsigned char *) (GPIO_D+0x15))
#define GPIO_MODER_D ((volatile unsigned int *) (GPIO_D))

#define GPIO_E 0x40021000 // Port E
#define GPIO_E_ODR_LOW ((volatile unsigned char *) (GPIO_E+0x14))
#define GPIO_E_ODR_HIGH ((volatile unsigned char *) (GPIO_E+0x15))
#define GPIO_E_IDR_LOW ((volatile unsigned char *) (GPIO_E+0x10))
#define GPIO_E_MODER ((volatile unsigned int *) (GPIO_E))

#define SYSCFG 0x40013800
#define SYSCFG_EXTICR1 0x40013808
#define EXTI3 ((volatile unsigned int *) (SYSCFG_EXTICR3+0x11))

#define EXTI 0x40013C00
#define EXTI_IMR ((volatile unsigned int *) (EXTI))
#define EXTI_FTSR ((volatile unsigned int *) (EXTI + 0xC))
#define EXTI_RTSR ((volatile unsigned int *) (EXTI + 0x8))
#define EXTI_PR ((volatile unsigned int *) (EXTI + 0x14))

#define NVIC 0xE000E100

#define SCB_VTOR ((volatile unsigned long *) 0xE000ED08)

static int count = 0;

void irq_handler( void ){
	if(* EXTI_PR & 8){
		* EXTI_PR |= 8;
		if(*GPIO_E_IDR_LOW & 1){
			*GPIO_E_ODR_LOW |= 0x10; /* Ettställ motsvarande RTS0-signal */
			*GPIO_E_ODR_LOW = 0x00; /* Nollställ motsvarande RTS0-signal */
			count ++;
		}
		
		 // * Kvittera avbrott EXTI2 *
		 // EXTI_PR |= EXTI2_IRQ_BPOS;		
		 // Skicka RST2 för att kvittera IRQ2
		 // GPIO_E_ODR_LOW |= (1<<6);
		 // GPIO_E_ODR_LOW &= ~(1<<6);
		 // #define EXTI2_IRQ_BPOS (1<<2)

		if(*GPIO_E_IDR_LOW & 2){
			*GPIO_E_ODR_LOW |= 0x20; /* Ettställ motsvarande RTS1-signal */
			*GPIO_E_ODR_LOW = 0x00; /* Nollställ motsvarande RTS1-signal */
			count = 0;
		}
		if(*GPIO_E_IDR_LOW & 4){
			*GPIO_E_ODR_LOW |= 0x40; /* Ettställ motsvarande RTS2-signal */
			*GPIO_E_ODR_LOW = 0x00; /* Nollställ motsvarande RTS2-signal */
			if(*GPIO_ODR_LOW == 0xFF){ 
				count = 0;
			}
			else {
				count = 0xFF;
			}
		}
	}
}

void init_app(){
	* SCB_VTOR = 0x2001C000;
	
	* GPIO_MODER_D = 0x55555555; /* PD0-15 ställs in som utport för visningsenhet */
	
	* GPIO_E_MODER = 0x55555500;
	
	* ((unsigned int *) SYSCFG_EXTICR1) &= ~0xF000; /* Nollställ bitfältet */
	* ((unsigned int *) SYSCFG_EXTICR1) |= 0x4000; /* PE3 -> EXTI3 */
	
	* ((unsigned int *) EXTI) |= 8; /* Aktivera genom att ettställa motsvarande bit i EXTI_IMR */
	
	* ((unsigned int *) EXTI_RTSR) |= 8; /* Aktivera triggvillkor "negativ flank" */
	
	* ((unsigned int *) EXTI_FTSR) &= ~8; /* Maskera triggvillkor positiv flank */
	
	* ((unsigned int *) NVIC) |= (1<<9); /* Konfigurera NVIC */
	
	* ((void (**) (void) ) 0x2001C064) = irq_handler; /* Sätt upp avbrottsvektor */	
}

void main(void)
{
	init_app();
	while(1){
		*GPIO_ODR_LOW = count;
	}
}

