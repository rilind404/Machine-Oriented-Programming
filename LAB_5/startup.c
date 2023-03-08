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

__attribute__((naked)) void graphic_initialize(void){
	__asm volatile (" .HWORD 0xDFF0\n");
	__asm volatile (" BX LR\n");
}

__attribute__((naked)) void graphic_clear_screen(void){
	__asm volatile (" .HWORD 0xDFF1\n");
	__asm volatile (" BX LR\n");
}

__attribute__((naked)) void graphic_pixel_set(int x, int y){
	__asm volatile (" .HWORD 0xDFF2\n");
	__asm volatile (" BX LR\n");
}

__attribute__((naked)) void graphic_pixel_clear(int x, int y){
	__asm volatile (" .HWORD 0xDFF3\n");
	__asm volatile (" BX LR\n");
}

#define GPIO_D 0x40020C00 // Port D
#define GPIO_D_PUPDR ((volatile unsigned int *) (GPIO_D + 0x0C))
#define GPIO_D_MODER ((volatile unsigned int *) (GPIO_D))
#define GPIO_D_IDR_LOW ((volatile unsigned char *) (GPIO_D + 0x10)) 
#define GPIO_D_IDR_HIGH ((volatile unsigned char *) (GPIO_D + 0x11))
#define GPIO_D_ODR_LOW ((volatile unsigned char *) (GPIO_D + 0x14))
#define GPIO_D_ODR_HIGH ((volatile unsigned char *) (GPIO_D + 0x15))

#define GPIO_E 0x40021000 // Port E
#define GPIO_E_PUPDR ((volatile unsigned int *) (GPIO_E + 0x0C))
#define GPIO_E_MODER ((volatile unsigned int *) (GPIO_E))
#define GPIO_E_IDR_LOW ((volatile unsigned char *) (GPIO_E + 0x10)) 
#define GPIO_E_IDR_HIGH ((volatile unsigned char *) (GPIO_E + 0x11))
#define GPIO_E_ODR_LOW ((volatile unsigned char *) (GPIO_E + 0x14))
#define GPIO_E_ODR_HIGH ((volatile unsigned char *) (GPIO_E + 0x15))

#define STK 0xE000E010 // SysTick
#define STK_CTRL ((volatile unsigned int *) STK)
#define STK_LOAD ((volatile unsigned int *) (STK + 0x4))
#define STK_VAL ((volatile unsigned int *) (STK + 0x8))

#define MAX_POINTS 30

#define LEFT_UP (1<<4)
#define LEFT_DOWN (1<<0)

#define RIGHT_UP (1<<5)
#define RIGHT_DOWN (1<<1)

// ASCII DISPLAY //

#define B_E 0x40
#define B_SELECT 0x4
#define B_RW 0x2
#define B_RS 0x1

int player_1_points = 0;
int player_2_points = 0;
int winner;

// DELAY METODER //

void delay_250ns(void){
	*STK_CTRL = 0;
	*STK_LOAD = ((168/4) - 1);
	*STK_VAL = 0;
	*STK_CTRL = 5;
	
	while((*STK_CTRL & 0x10000) == 0);
	*STK_CTRL = 0;
}

void delay_mikro(unsigned int us){
	#ifdef SIMULATOR
		us = us/1000;
		us++;
	#endif
	
	while(us > 0){
		delay_250ns();
		delay_250ns();
		delay_250ns();
		delay_250ns();
		us--;
	}
}

void delay_milli(unsigned int ms){
	#ifdef SIMULATOR
		ms = ms/10;
		ms++;
	#endif
	while(ms > 0){
		delay_mikro(10);
		ms--;
	}
}

typedef struct
{
	char x,y;
} POINT, *PPOINT;

typedef struct
{
	POINT p0;
	POINT p1;
} LINE, *PLINE;

typedef struct{
	int numpoints;
	int sizex;
	int sizey;
	POINT px[MAX_POINTS];
} GEOMETRY, *PGEOMETRY;

typedef struct tObj{
	PGEOMETRY geo;
	int dirx,diry;
	int posx,posy;
	void (* draw) (struct tObj *);
	void (* clear) (struct tObj *);
	void (* move) (struct tObj *);
	void (* set_speed) (struct tObj *, int, int);
} OBJECT, *POBJECT;

int abs(int i)
{
	if (i < 0) 
		return (i * -1);
	else
		return i;
}

void swap(int *x, int *y)
{
	int temp = *x;
	*x = *y;
	*y = temp;
}

void draw_line(PLINE line){
	int steep;
	int deltax;
	int deltay;
	int error;
	int y;
	int x;
	int ystep;
	int x0 = line->p0.x;
	int y0 = line->p0.y;
	int x1 = line->p1.x;
	int y1 = line->p1.y;
	
	if(abs(y1-y0) > abs(x1-x0)){
		steep = 1;
	}
	else{
		steep = 0;
	}
	
	if(steep == 1){
		swap(&x0,&y0);
		swap(&x1,&y1);
	}
	
	if(x0 > x1){
		swap(&x0,&x1);
		swap(&y0,&y1);
	}
	
	deltax = x1 - x0;
	deltay = abs(y1 - y0);
	error = 0;
	
	y = y0;
	
	if(y0 < y1){
		ystep = 1;
	}
	else{
		ystep = -1;
	}
	
	for(int x = x0; x < x1; x++){
		if(steep){
			graphic_pixel_set(y,x);
		}
		else{
			graphic_pixel_set(x,y);
		}
		error = error + deltay;
		if((2*error) >= deltax){
			y = y + ystep;
			error = error - deltax;
		}
	}
}

void draw_ballobject(POBJECT object){
	for (int i = 0; i < (object->geo->numpoints); i++){
		graphic_pixel_set(object->posx + object->geo->px[i].x, object->posy + object->geo->px[i].y);
	}
}

void clear_ballobject(POBJECT object){
	for (int i = 0; i < (object->geo->numpoints); i++){
		graphic_pixel_clear(object->posx + object->geo->px[i].x, object->posy + object->geo->px[i].y);
	}
}

void move_ballobject(POBJECT object){
	clear_ballobject(object);
    
	if (object->posx < 1)
	{
		object->dirx = object->dirx * -1;
	}
	
	if (object->posx > 128)
	{
		object->dirx = object->dirx * -1;
	}
	
	if (object->posy < 1)
	{
		object->diry = object->diry * -1;
	}

	if ((object->posy + object->geo->sizey) > 64)
	{
		object->diry = object->diry * -1;
	}
	
	object->posx = object->posx + object->dirx;
	object->posy = object->posy + object->diry;
	
	draw_ballobject(object);
}

void move_paddleobject(POBJECT object)
{
	clear_ballobject(object);
	
	if (object->posy < 1)
	{
		object->posy = 1;
	}

	if ((object->posy + object->geo->sizey) > 64)
	{
		object->posy = 64 - object->geo->sizey;
	}
	
	object->posx = object->posx + object->dirx;
	object->posy = object->posy + object->diry;
	
	draw_ballobject(object);
}

int overlap(POBJECT object, POBJECT paddle){
    if(((object->posx) < (paddle->posx + paddle->geo->sizex)) && (object->posy < (paddle->posy + paddle->geo->sizey)) && ((object->posx + object->geo->sizex) > paddle->posx) && ((object->posy + object->geo->sizey) > paddle->posy)){
        return 1;
    }
    return 0;
}

void set_ballobject_speed(POBJECT o, int speedx, int speedy){
	o->dirx = speedx;
	o->diry = speedy;
}

GEOMETRY ball_geometry = {
	12,
	4,4,
	{
		{0,1},{0,2},{1,0},{1,1},{1,2},{1,3},{2,0},{2,1},{2,2},{2,3},{3,1},{3,2}
	}
};

GEOMETRY paddle_geometry = {
	27,
	5,9,
	{
		{0,0},{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},{0,7},{0,8},{1,0},{1,8},{2,0},{2,3},{2,4},{2,5},{2,8},{3,0},{3,8},{4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{4,7},{4,8}
	}
};

static OBJECT ballobject = {
		&ball_geometry,
		0,0,
		30,30,
		draw_ballobject,
		clear_ballobject,
		move_ballobject,
		set_ballobject_speed
	};

static OBJECT paddle_1_object = {
    &paddle_geometry,
    0,0,
    6,26,
    draw_ballobject,
    clear_ballobject,
    move_paddleobject,
    set_ballobject_speed
};

static OBJECT paddle_2_object = {
    &paddle_geometry,
    0,0,
    118,26,
    draw_ballobject,
    clear_ballobject,
    move_paddleobject,
    set_ballobject_speed
};

unsigned char keyb(void){
    
    unsigned char key[]={1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
    int row, col;
    for (row=1; row <=4 ; row++ ) {
        kbdActivate( row );
        if( (col = kbdGetCol () ) ){
            kbdActivate( 0 );
            return key [4*(row-1)+(col-1) ];
        }
    }    
    kbdActivate(0);
    return 0xFF;
}

void kbdActivate( unsigned int row ){ 
    /* Aktivera angiven rad hos tangentbordet, eller
    * deaktivera samtliga */
    switch( row ){
        case 1: * GPIO_D_ODR_HIGH = 0x10; break;
        case 2: *GPIO_D_ODR_HIGH = 0x20; break;
        case 3: *GPIO_D_ODR_HIGH = 0x40; break;
        case 4: *GPIO_D_ODR_HIGH = 0x80; break;
        case 0: *GPIO_D_ODR_HIGH = 0x00; break;
    }
}

int kbdGetCol ( void )
{     /* Om någon tangent (i aktiverad rad)
    * är nedtryckt, returnera dess kolumnnummer,
    * annars, returnera 0 */
    unsigned char c;
    c = * GPIO_D_IDR_HIGH;
    if ( c & 0x8 ) return 4;
    if ( c & 0x4 ) return 3;
    if ( c & 0x2 ) return 2;
    if ( c & 0x1 ) return 1;
    return 0;
}

// Addressera ASCII-Display och ettställ de bitar som är 1 i x
void ascii_ctrl_bit_set( unsigned char x ){
	char c;
	c = * GPIO_E_ODR_LOW;
	* GPIO_E_ODR_LOW = B_SELECT | x | c;
}

// Addressera ASCII-Display och nollställ de bitar som är 1 i x
void ascii_ctrl_bit_clear ( unsigned char x ){
	char c;
	c = * GPIO_E_ODR_LOW;
	c = c & ~x;
    * GPIO_E_ODR_LOW = B_SELECT | c;
}

void ascii_write_controller( unsigned char command) {
	ascii_ctrl_bit_set(B_E);
	* GPIO_E_ODR_HIGH = command;
	delay_250ns();
	ascii_ctrl_bit_clear(B_E);
}

unsigned char ascii_read_controller() {
	unsigned char rv;
	
	ascii_ctrl_bit_set(B_E);
	delay_250ns();
	delay_250ns();
	
	rv = * GPIO_E_IDR_HIGH;
	ascii_ctrl_bit_clear(B_E);
	
	return rv;
}

void ascii_write_cmd( unsigned char command ){
	ascii_ctrl_bit_clear(B_RS);
	ascii_ctrl_bit_clear(B_RW);
	ascii_write_controller(command);
}

void ascii_write_data( unsigned char data){
	ascii_ctrl_bit_set(B_RS);
	ascii_ctrl_bit_clear(B_RW);
	ascii_write_controller(data);
}

void ascii_gotoxy(int row, int col){
	int address = row-1;
	
	if(col==2){
		address = address + 0x40;
	}
	ascii_write_cmd( 0x80 | address );
}

unsigned char ascii_read_status(void){
	unsigned char rv;
	
	* GPIO_E_MODER = 0x00005555;
	ascii_ctrl_bit_clear(B_RS);
	ascii_ctrl_bit_set(B_RW);
	rv = ascii_read_controller();
	* GPIO_E_MODER = 0x55555555;

	return rv;
}

void ascii_write_char(char c) {
	while( (ascii_read_status() & 0x80)==0x80 );
	
	delay_mikro(8);
	ascii_write_data(c);
	delay_mikro(500);

}

unsigned char ascii_read_data(void){
	unsigned char rv;
	
	* GPIO_E_MODER = 0x00005555;
	ascii_ctrl_bit_set(B_RS);
	ascii_ctrl_bit_clear(B_RW);
	
	rv = ascii_read_controller();
	ascii_ctrl_bit_clear(B_E);
	* GPIO_E_MODER = 0x55555555;

	return rv;
}

void clear_display(){
	while( (ascii_read_status() & 0x80)==0x80 );
	
	delay_mikro(8);
	ascii_write_cmd(1);
	delay_milli(2);
}

void functionSetCommand(void){
	 while((ascii_read_status() & 0x80) == 0x80){
		
	}
	delay_mikro(8);
	ascii_write_cmd(0b00111000);
	delay_mikro(40);
 }

 void turnOnDisplayCommand(void){
	 while((ascii_read_status() & 0x80) == 0x80){
		
	}
	delay_mikro(8);
	ascii_write_cmd(0b00001111);
	delay_mikro(40);
 }
 
void modeSetCommand(void){
	while((ascii_read_status() & 0x80) == 0x80){
		
	}
	delay_mikro(8);
	ascii_write_cmd(0b00001110);
	delay_mikro(40);
}



void reset_map(POBJECT paddle_1, POBJECT paddle_2, POBJECT ball){
	graphic_clear_screen();
	
	paddle_1->posy = 32;
	paddle_2->posy = 32;
	ball->posx = 64;
	ball->posy = 32;
	ball->dirx = ball->dirx * -1;
	ball->diry = ball->diry * -1;
	clear_display();
	ascii_scoreboard();
}

void ascii_scoreboard(){
	char *s;
	int symbols[] = {48,49,50,51,52,53,54,56};
	char player_1_score[] = "Score P1: ";
	char player_2_score[] = "Score P2: ";

	init_app();
	ascii_init();

	ascii_gotoxy(1,1);
	s = player_1_score;
	
	while(*s){
		ascii_write_char(*s++);
	}
	ascii_write_char(symbols[player_1_points]);

	ascii_gotoxy(1,2);
	s = player_2_score;
	
	while(*s){
		ascii_write_char(*s++);
	}
	
	ascii_write_char(symbols[player_2_points]);
}

void ascii_init()
{
	// INITIERA ASCII //
	functionSetCommand();
	turnOnDisplayCommand();
	modeSetCommand();	
	clear_display();
}

void init_app()
{
	* GPIO_D_MODER = 0x55005555;
	* GPIO_D_PUPDR = 0xAAAA0000;
	
	/* starta klockor port D och E pull down*/
    *((unsigned long *) 0x40023830) = 0x18;
	
	ascii_init();
}

void ascii_gameover()
{
	clear_display();
	ascii_init();
	char *s;
	char Game_Over[] = "GAME OVER";
	char Player[] = "PLAYER ";
	char Wins[] = " WINS!";
	
	ascii_gotoxy(6,1);
	s=Game_Over;
	while(*s){
		ascii_write_char(*s++);
	}
	
	s=Player;
	ascii_gotoxy(4,2);
	while(*s){
		ascii_write_char(*s++);
	}
	ascii_write_char(winner);
	
	s=Wins;
	while(*s){
		ascii_write_char(*s++);
	}
	
}

void reset_playerpoints()
{
	player_1_points = 0;
	player_2_points = 0;
}

void main(void)
{
	unsigned char c;
	POBJECT ball = &ballobject;
	POBJECT paddle_1 = &paddle_1_object;
	POBJECT paddle_2 = &paddle_2_object;
	
	ball->set_speed(ball, 1, 1);
	
	init_app();
	graphic_initialize();
	graphic_clear_screen();
	
	ascii_scoreboard();
	
	paddle_1->draw(paddle_1);
	
	while(1)
		{
			ball->move(ball);
			paddle_1->move(paddle_1);
			paddle_2->move(paddle_2);
			
			c = keyb();
			switch ( c )
            {
                case 2: paddle_1->set_speed( paddle_1, 0,-2); break;
                case 8: paddle_1->set_speed( paddle_1, 0, 2); break;
				case 5: reset_map(paddle_1, paddle_2, ball); reset_playerpoints(); break;
                default: paddle_1->set_speed( paddle_1, 0, 0); break;
			}
			
			switch( c )
            {
				case 3: paddle_2->set_speed( paddle_2, 0,-2); break;
                case 9: paddle_2->set_speed( paddle_2, 0, 2); break;
                case 5: reset_map(paddle_1, paddle_2, ball); reset_playerpoints(); break;
                default: paddle_2->set_speed( paddle_2, 0, 0); break;
            }
			
			
			if (overlap(ball, paddle_1))
			{
				ball->clear(ball);
				ball->dirx = -ball->dirx;
				ball->draw(ball);
			}
			else if (ball->posx < (1 + ball->geo->sizex))
			{
				player_1_points += 1;
				reset_map(paddle_1, paddle_2, ball);
			}
			
			if (overlap(ball, paddle_2))
			{
				ball->clear(ball);
				ball->dirx = -ball->dirx;
				ball->draw(ball);
			}
			else if (ball->posx > (128 - ball->geo->sizex))
			{
				player_2_points += 1;
				reset_map(paddle_1, paddle_2, ball);
			}
			delay_milli(40);
			if (player_1_points >= 5){
				winner = 49;
				break;
			}
				
			if (player_2_points >= 5)
			{
				winner = 50;
				break;
			}
		}
		ascii_gameover();
}

