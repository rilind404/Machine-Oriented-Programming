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

#define SCB_VTOR ((volatile unsigned long *) 0xE000ED08)

#define SYSCFG_EXTICR1 *((volatile unsigned int *) 0x40013808)

#define EXTI 0x40013C00
#define EXTI_IMR ((volatile unsigned int *) (EXTI))
#define EXTI_FTSR ((volatile unsigned int *) (EXTI + 0xC))
#define EXTI_RTSR ((volatile unsigned int *) (EXTI + 0x8))
#define EXTI_PR ((volatile unsigned int *) (EXTI + 0x14))

#define EXTI2_IRQVEC ((void (**) (void) ) 0x2001C060)
#define EXTI1_IRQVEC ((void (**) (void) ) 0x2001C05C)
#define EXTI0_IRQVEC ((void (**) (void) ) 0x2001C058)

#define EXTI2_IRQ_BPOS (1<<2)
#define EXTI1_IRQ_BPOS (1<<1)
#define EXTI0_IRQ_BPOS (1<<0)

#define NVIC_ISER0 ((volatile unsigned int *) 0xE000E100)
#define NVIC_EXTI2_IRQ_BPOS (1<<8)
#define NVIC_EXTI1_IRQ_BPOS (1<<7)
#define NVIC_EXTI0_IRQ_BPOS (1<<6)


#define GPIO_D 0x40020C00
#define GPIO_D_MODER ((volatile unsigned int *) (GPIO_D))
#define GPIO_D_ODR_LOW ((volatile unsigned char *) (GPIO_D+0x14))
#define GPIO_D_ODR_HIGH ((volatile unsigned char *) (GPIO_D+0x15))
#define GPIO_D_IDR_LOW ((volatile unsigned char *) (GPIO_D+0x10))
#define GPIO_D_IDR_HIGH ((volatile unsigned char *) (GPIO_D+0x11))

#define GPIO_E 0x40021000
#define GPIO_E_MODER ((volatile unsigned int *) (GPIO_E))
#define GPIO_E_ODR_LOW ((volatile unsigned char *) (GPIO_E+0x14))
#define GPIO_E_ODR_HIGH ((volatile unsigned char *) (GPIO_E+0x15))
#define GPIO_E_IDR_LOW ((volatile unsigned char *) (GPIO_E+0x10))
#define GPIO_E_IDR_HIGH ((volatile unsigned char *) (GPIO_E+0x11))
#define GPIO_E_PUPDR ((volatile unsigned char *) (GPIO_E+0x0C))

static unsigned int count = 0;

void irq2_handler()
{
  /* Om avbrott från EXTI2 */
  if(* EXTI_PR & EXTI2_IRQ_BPOS){
	  /* Kvittera avbrott EXTI2 */
		* EXTI_PR |= EXTI2_IRQ_BPOS;		
	  /* Skicka RST2 för att kvittera IRQ2 */
		* GPIO_E_ODR_LOW |= (1<<6);
		* GPIO_E_ODR_LOW &= ~(1<<6);
	  
	  /* Växla mellan 0 och FF på utport */
	if(* GPIO_D_ODR_LOW == 0xFF){
		count = 0;
		}
	else
		count = 0xFF;
	}
}
void irq1_handler(){
  /* Om avbrott från EXTI1 */
  if(* EXTI_PR & EXTI1_IRQ_BPOS){
	  /* Återställ avbrott från EXTI1 */
		* EXTI_PR |= EXTI1_IRQ_BPOS;
	  /* Skicka RST1 för att kvittera IRQ1 */
		* GPIO_E_ODR_LOW |= 0x20;
		* GPIO_E_ODR_LOW &= ~0x20;
		
		count = 0;
	}
}

void irq0_handler(){
/* Om avbrott från EXTI0 */
  if(* EXTI_PR & EXTI0_IRQ_BPOS){
	  /* Återställ avbrott från EXTI0 */
		* EXTI_PR |= EXTI0_IRQ_BPOS;
		
	  /* Skicka RST0 för att kvittera IRQ0 */
		* GPIO_E_ODR_LOW |= 0x10;
		* GPIO_E_ODR_LOW &= ~0x10;
		
		count++;
	}
}

void init_app()
{
	* SCB_VTOR = 0x2001C000;
	
	* GPIO_D_MODER = 0x00005555;
	
	* GPIO_E_MODER = 0x55555500;
	
	/* Nollställ fält */
	SYSCFG_EXTICR1 &= ~0xFFFF;
	/* PE2 -> EXTI2 */
	SYSCFG_EXTICR1 |= 0x0400;
	/* PE1 -> EXTI1 */
	SYSCFG_EXTICR1 |= 0x0040;
	/* PE0 -> EXTI0 */
	SYSCFG_EXTICR1 |= 0x0004;
	
	/* Avbrottslina EXTI0-3 konfigureras för generering av avbrott vid positiv flank */
	/* Aktivera genom att ettställa motsvarande bit i EXTI_IMR. 7 = 0111 -> EXTI0-3 */
	* EXTI_IMR |= 7;
	/* Aktivera triggvillkor "positiv flank" */
	* EXTI_RTSR |= 7;
	/* Maskera triggvillkor "negativ flank" */
	* EXTI_FTSR &= ~7;
	
	/* Initiera Avbrottsvektor - När avbrott genereras från EXT0-2 så kallas irq_handler() funktionen */
	* EXTI2_IRQVEC = irq2_handler;
	* EXTI1_IRQVEC = irq1_handler;
	* EXTI0_IRQVEC = irq0_handler;
	
	/* Möjliggör avbrott från EXTI0-2 vidare till processorn med ISER (Interrupt Set Enable Register) */
	/* Aktivera NVIC för EXTI2 */
	* NVIC_ISER0 |= NVIC_EXTI2_IRQ_BPOS;
	/* Aktivera NVIC för EXTI1 */
	* NVIC_ISER0 |= NVIC_EXTI1_IRQ_BPOS;
	/* Aktivera NVIC för EXTI0 */
	* NVIC_ISER0 |= NVIC_EXTI0_IRQ_BPOS;
}

void main(void)
{
	init_app();
	while(1){
		* GPIO_D_ODR_LOW = count;
	}
}

