#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"
#include <string.h>

// LED ext
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      30
#define LED_IDX_MASK (1 << LED_IDX)

// Configuracoes do botao1
#define BUT1_PIO        PIOD
#define BUT1_PIO_ID     ID_PIOD
#define BUT1_PIO_IDX    28                     // Index
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX)  // esse já está pronto.

// Configuracoes do botao2
#define BUT2_PIO        PIOC
#define BUT2_PIO_ID     ID_PIOC
#define BUT2_PIO_IDX    31                     // Index
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX)  // esse já está pronto.

// Configuracoes do botao3
#define BUT3_PIO        PIOA
#define BUT3_PIO_ID     ID_PIOA
#define BUT3_PIO_IDX    19                     // Index
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX)  // esse já está pronto.

volatile char but1_flag_minus;
volatile char but1_flag_plus;

volatile char led_on = 0;
volatile char but2_flag_turn_led_on;
volatile char but2_flag_turn_led_off;

volatile char but3_flag;

void but1_callback(void)
{
	if (pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK)){
		but1_flag_minus = 1;
		but1_flag_plus = 0;
	} else {
		but1_flag_plus = 1;
		but1_flag_minus = 0;
	}
}

void but2_callback(void)
{
	if(led_on){
		but2_flag_turn_led_off = 1;
	}
	else{
		but2_flag_turn_led_on = 1;
	}
}

void but3_callback(void)
{
	but3_flag = 1;
}


void pisca_led(int t){
	led_on = 1;
	gfx_mono_draw_string("pause", 50, 16, &sysfont);
	for (int i=0;i<30;i++){
		
		pio_clear(LED_PIO, LED_IDX_MASK);
		delay_ms(t);
		
		gfx_mono_draw_filled_rect(64, 2, 1+i*2, 8, GFX_PIXEL_SET);
		
		pio_set(LED_PIO, LED_IDX_MASK);
		delay_ms(t);
		
		if (but2_flag_turn_led_off) {
			led_on = 0;
			gfx_mono_draw_string("play ", 50, 16, &sysfont);
			break;
		}
	}
	led_on = 0;
}

void delay_oled(int ms){
	char delay_str[128];
	sprintf(delay_str, "%d", ms);
	gfx_mono_draw_string("      ", 0, 0, &sysfont);
	gfx_mono_draw_filled_rect(64, 2, 60, 8, GFX_PIXEL_CLR);
	gfx_mono_draw_rect(64, 2, 60, 8, GFX_PIXEL_SET);
	strcat(delay_str,"ms");
	gfx_mono_draw_string(delay_str, 0, 0, &sysfont);
	gfx_mono_draw_string("play ", 50, 16, &sysfont);
}

// Inicializa botao SW0 do kit com interrupcao
void io_init(void)
{

	// Configura led
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);

	// Inicializa clock do periférico PIO responsavel pelo botao
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pmc_enable_periph_clk(BUT3_PIO_ID);

	// Configura PIO para lidar com o pino do botão como entrada
	// com pull-up
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT1_PIO, BUT1_PIO_IDX_MASK, 60);
	
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT2_PIO, BUT2_PIO_IDX_MASK, 60);
	
	pio_configure(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT3_PIO, BUT3_PIO_IDX_MASK, 60);


	// Configura interrupção no pino referente ao botao e associa
	// função de callback caso uma interrupção for gerada
	// a função de callback é a: but_callback()
	pio_handler_set(BUT1_PIO,
					BUT1_PIO_ID,
					BUT1_PIO_IDX_MASK,
					PIO_IT_EDGE,
					but1_callback);
					
	pio_handler_set(BUT2_PIO,
					BUT2_PIO_ID,
					BUT2_PIO_IDX_MASK,
					PIO_IT_RISE_EDGE,
					but2_callback);
		
	pio_handler_set(BUT3_PIO,
					BUT3_PIO_ID,
					BUT3_PIO_IDX_MASK,
					PIO_IT_RISE_EDGE,
					but3_callback);

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);
	
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT2_PIO);
		
	pio_enable_interrupt(BUT3_PIO, BUT3_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT3_PIO);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4); // Prioridade 4
	
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 5); // Prioridade 5
		
	NVIC_EnableIRQ(BUT3_PIO_ID);
	NVIC_SetPriority(BUT3_PIO_ID, 6); // Prioridade 6
}

int main (void)
{
	board_init();
	sysclk_init();
	delay_init();
	
	// Desativa watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;

	// configura botao com interrupcao
	io_init();

	// Init OLED
	gfx_mono_ssd1306_init();
	
	int delay_cnt = 100;
  
	gfx_mono_draw_string("100ms", 0, 0, &sysfont);
	gfx_mono_draw_rect(64, 2, 60, 8, GFX_PIXEL_SET);
	gfx_mono_draw_string("-/+", 10, 16, &sysfont);
	gfx_mono_draw_string("-", 118, 16, &sysfont);
	gfx_mono_draw_string("pause", 50, 16, &sysfont);
  
	pisca_led(100);
	
	int flag_summed_one = 0;

	/* Insert application code here, after the board has been initialized. */
	while(1) {

		// Escreve na tela um circulo e um texto
		int clock_count = 0;
		while(but1_flag_plus){
			
			if (clock_count > 25000000){
				flag_summed_one = 1;
				delay_cnt += 100;
				delay_oled(delay_cnt);
				clock_count = 0;
			}
			clock_count ++;
			
		}
			
		if(but1_flag_minus && !flag_summed_one && !but2_flag_turn_led_off && !but2_flag_turn_led_on && !but3_flag){
			delay_cnt -= 100;
			delay_oled(delay_cnt);
			but1_flag_minus = 0;
		}
		
		if(but2_flag_turn_led_off){
			but1_flag_minus = 0;
			but2_flag_turn_led_off = 0;
		}
		
		if(but2_flag_turn_led_on){
			pisca_led(delay_cnt);
			but2_flag_turn_led_on=0;
		}
		
		if(but3_flag){
			delay_cnt -= 100;
			delay_oled(delay_cnt);
			but3_flag = 0;
		}
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
		flag_summed_one = 0;
		
	}
}