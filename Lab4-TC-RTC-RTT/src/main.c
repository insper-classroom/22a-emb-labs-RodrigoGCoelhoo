#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/************************************************************************/
/* DEFINES                                                              */
/************************************************************************/

// LED SAME 70
#define LED_PIO PIOC
#define LED_ID ID_PIOC
#define LED_IDX 8
#define LED_IDX_MASK (1 << LED_IDX)

// LED 1
#define LED1_PIO  PIOA
#define LED1_ID  ID_PIOA
#define LED1_IDX  0
#define LED1_IDX_MASK  (1 << LED1_IDX)

// LED 2
#define LED2_PIO  PIOC
#define LED2_ID  ID_PIOC
#define LED2_IDX  30
#define LED2_IDX_MASK  (1 << LED2_IDX)

// LED 3
#define LED3_PIO  PIOB
#define LED3_ID  ID_PIOB
#define LED3_IDX  2
#define LED3_IDX_MASK  (1 << LED3_IDX)

// BUTTON 1 Oled
#define BUT1_PIO  PIOD
#define BUT1_ID  ID_PIOD
#define BUT1_IDX  28
#define BUT1_IDX_MASK  (1u << BUT1_IDX)

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

/************************************************************************/
/* VAR globais                                                          */
/************************************************************************/

uint32_t current_hour, current_min, current_sec;
uint32_t current_year, current_month, current_day, current_week;

volatile char flag_show_hour = 1;

volatile char flag_rtc_alarm = 0;
volatile char flag_alarm = 0;

volatile char flag_blinking = 0;
volatile char flag_stop_blinking = 0;

/************************************************************************/
/* PROTOTYPES                                                           */
/************************************************************************/

void LED_init(int estado);

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);

void but_1_callback(void);
void pin_toggle(Pio *pio, uint32_t mask);

/************************************************************************/
/* Handlers                                                             */
/************************************************************************/

void TC7_Handler(void) {
	volatile uint32_t status = tc_get_status(TC2, 1);

	pin_toggle(LED3_PIO, LED3_IDX_MASK);  
}

void TC1_Handler(void) {
	volatile uint32_t status = tc_get_status(TC0, 1);

	pin_toggle(LED1_PIO, LED1_IDX_MASK);  
}

void TC4_Handler(void) {
	volatile uint32_t status = tc_get_status(TC1, 1);

	pin_toggle(LED_PIO, LED_IDX_MASK);  
}

void RTT_Handler(void) {
	uint32_t ul_status;

	ul_status = rtt_get_status(RTT);

	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		RTT_init(4, 16, RTT_MR_ALMIEN);
	}
	
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		pin_toggle(LED2_PIO, LED2_IDX_MASK);
	}

}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		flag_show_hour = 1;
	}
	
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		flag_rtc_alarm = 1;
		if (flag_blinking) {
			flag_rtc_alarm = 0;
			flag_stop_blinking = 1;
			flag_blinking = 0;
		}
	}
	
	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void but_1_callback(void) {	
	if (pio_get(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK)){
		flag_alarm = 0;
	} else {
		flag_alarm = 1;
	}
}

/************************************************************************/
/* Funções                                                              */
/************************************************************************/

void pin_toggle(Pio *pio, uint32_t mask) {
	if (pio_get_output_data_status(pio, mask)) {
		pio_clear(pio, mask);
	} else {
		pio_set(pio,mask);
	}
}

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);
	
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	pmc_enable_periph_clk(ID_TC);

	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	pmc_enable_periph_clk(ID_RTC);

	rtc_set_hour_mode(rtc, 0);

	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	rtc_enable_interrupt(rtc,  irq_type);
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

void draw (uint32_t current_hour, uint32_t current_min, uint32_t current_sec){
	char tempo[20];
	sprintf(tempo, "%02d:%02d:%02d", current_hour, current_min, current_sec);
	gfx_mono_draw_string(tempo, 24, 8, &sysfont);
}

void init (void) {

	board_init();
	delay_init();
	sysclk_init();
	gfx_mono_ssd1306_init();
	
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	// Leds init
	pmc_enable_periph_clk(LED_ID);
	pmc_enable_periph_clk(LED1_ID);
	pmc_enable_periph_clk(LED2_ID);
	pmc_enable_periph_clk(LED3_ID);

	pio_set_output(LED_PIO, LED_IDX_MASK, 1, 0, 0);
	pio_set_output(LED1_PIO, LED1_IDX_MASK, 1, 0, 0);
	pio_set_output(LED2_PIO, LED2_IDX_MASK, 1, 0, 0);
	pio_set_output(LED3_PIO, LED3_IDX_MASK, 1, 0, 0);
	
	// Button init
	pmc_enable_periph_clk(BUT1_ID);
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);	
	pio_set_debounce_filter(BUT1_PIO, BUT1_IDX_MASK, 60);
	pio_handler_set(BUT1_PIO, BUT1_ID, BUT1_IDX_MASK, PIO_IT_EDGE, but_1_callback);
	
	pio_enable_interrupt(BUT1_PIO, BUT1_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);
	
	NVIC_EnableIRQ(BUT1_ID);
	NVIC_SetPriority(BUT1_ID, 5);
	
	/* Configure timer TC1, canal 1 */
	/* e inicializa a contagem */
	TC_init(TC1, ID_TC4, 1, 5);
	tc_start(TC1, 1);
	
	/* Configure timer TC0, canal 1 */
	/* e inicializa a contagem */
	TC_init(TC0, ID_TC1, 1, 4);
	tc_start(TC0, 1);
	
	/* Configure RTT */
	RTT_init(4, 16, RTT_MR_ALMIEN);
	
	/* Configura RTC */
	calendar rtc_initial = {2022, 3, 18, 3, 22, 20 ,0};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);

}

int main (void) {
	
	init();
	
	while(1) {
		
		rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
		rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
		
		if (flag_alarm) {
			rtc_set_date_alarm(RTC, 1, current_month, 1, current_day);
			rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 20);
		}
		
		if (flag_show_hour) {
			draw(current_hour, current_min, current_sec);
			flag_show_hour = 0;
		}
		
		
		if (flag_stop_blinking) {
			tc_stop(TC2, 1);
			pio_set(LED3_PIO, LED3_IDX_MASK);
			
		} else {
			
			if (flag_rtc_alarm){
				
				/* Configure timer TC2, canal 1 e inicializa a contagem */
				TC_init(TC2, ID_TC7, 1, 5);
				tc_start(TC2, 1);
						
				/* configura alarme do RTC para daqui 20 segundos */
				rtc_set_date_alarm(RTC, 1, current_month, 1, current_day);
				rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 2);
				
				flag_blinking = 1;
				flag_rtc_alarm = 0;
			}
			
			flag_stop_blinking = 0;
		}
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}