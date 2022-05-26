/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"

// #include "V1.h"
// #include "V2_home.h"
// #include "V2_config.h"
// #include "V3_home.h"
// #include "V3_config.h"

LV_FONT_DECLARE(dseg70);
LV_FONT_DECLARE(dseg50);
LV_FONT_DECLARE(dseg30);
LV_FONT_DECLARE(dseg32);
LV_FONT_DECLARE(dseg20);

/************************************************************************/
/* Vars globais                                                         */
/************************************************************************/

lv_obj_t * label_btn_power;
lv_obj_t * label_btn_menu;
lv_obj_t * label_btn_clock;
lv_obj_t * label_btn_up;
lv_obj_t * label_btn_down;

lv_obj_t * label_temp;
lv_obj_t * label_temp_decimal;
lv_obj_t * label_temp_reference;
lv_obj_t * label_time;

lv_obj_t * label_graus1;
lv_obj_t * label_graus2;

volatile char power_state = 0;

SemaphoreHandle_t xMutexLVGL;

/************************************************************************/
/* RTC                                                                  */
/************************************************************************/

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

uint32_t current_hour, current_min, current_sec;
uint32_t current_year, current_month, current_day, current_week;
calendar rtc_initial = {2022, 5, 19, 4, 13, 31 , 0};

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);

/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_RTC_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_RTC_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

SemaphoreHandle_t xSemaphoreRTC;

/************************************************************************/
/* handlers                                                             */
/************************************************************************/

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void up_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	char *c;
	int temp;
	if(code == LV_EVENT_CLICKED) {
		c = lv_label_get_text(label_temp_reference);
		temp = atoi(c);
		lv_label_set_text_fmt(label_temp_reference, "%02d", temp + 1);
	}
}

static void down_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	char *c;
	int temp;
	if(code == LV_EVENT_CLICKED) {
		c = lv_label_get_text(label_temp_reference);
		temp = atoi(c);
		lv_label_set_text_fmt(label_temp_reference, "%02d", temp - 1);
	}
}

void RTC_Handler(void) {
    uint32_t ul_status = rtc_get_status(RTC);
	
    /* seccond tick */
    if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreRTC, &xHigherPriorityTaskWoken);
    }
    
	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

static void power_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		power_state = 1;
	}
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/


void lv_ex_btn_1(void) {
	lv_obj_t * label;

	lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);

	label = lv_label_create(btn1);
	lv_label_set_text(label, "Corsi");
	lv_obj_center(label);

	lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);

	label = lv_label_create(btn2);
	lv_label_set_text(label, "Toggle");
	lv_obj_center(label);
}

void lv_termostato_buttons(void) {
	
	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_color_black());
	lv_style_set_border_color(&style, lv_color_black());
	lv_style_set_border_width(&style, 5);
	
	// Botao power
	lv_obj_t * btn_power = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn_power, power_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn_power, LV_ALIGN_BOTTOM_LEFT, 0, 0);
	lv_obj_add_style(btn_power, &style, 0);
	lv_obj_set_width(btn_power, 60);  
	lv_obj_set_height(btn_power, 60);

	// Label botao power
	label_btn_power = lv_label_create(btn_power);
	lv_label_set_text(label_btn_power, "[  " LV_SYMBOL_POWER);
	lv_obj_center(label_btn_power);
	
	
	// Botao menu
	lv_obj_t * btn_menu = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn_menu, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btn_menu, btn_power, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_obj_add_style(btn_menu, &style, 0);
	lv_obj_set_width(btn_menu, 60);  
	lv_obj_set_height(btn_menu, 60);

	// Label botao menu
	label_btn_menu = lv_label_create(btn_menu);
	lv_label_set_text(label_btn_menu, " |  M  | ");
	lv_obj_center(label_btn_menu);
	
	
	// Botao relogio
	lv_obj_t * btn_clock = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn_clock, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btn_clock, btn_menu, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_obj_add_style(btn_clock, &style, 0);
	lv_obj_set_width(btn_clock, 60);  
	lv_obj_set_height(btn_clock, 60);

	// Label botao relogio
	label_btn_clock = lv_label_create(btn_clock);
	lv_label_set_text(label_btn_clock, LV_SYMBOL_BELL "  ]" );
	lv_obj_center(label_btn_clock);
	
	
	// Botao down
	lv_obj_t * btn_down = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn_down, down_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn_down, LV_ALIGN_BOTTOM_RIGHT, -20, 0);
	lv_obj_add_style(btn_down, &style, 0);
	lv_obj_set_width(btn_down, 60);  
	lv_obj_set_height(btn_down, 60);
	
	// Label botao down
	label_btn_down = lv_label_create(btn_down);
	lv_label_set_text(label_btn_down, " |  " LV_SYMBOL_DOWN "  ]" );
	lv_obj_center(label_btn_down);
	
	// Botao up
	lv_obj_t * btn_up = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn_up, up_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btn_up, btn_down, LV_ALIGN_OUT_LEFT_TOP, -30, 0);
	lv_obj_add_style(btn_up, &style, 0);
	lv_obj_set_width(btn_up, 60);  
	lv_obj_set_height(btn_up, 60);

	// Label botao up
	label_btn_up = lv_label_create(btn_up);
	lv_label_set_text(label_btn_up, "[  " LV_SYMBOL_UP);
	lv_obj_center(label_btn_up);
	
}

void lv_termostato_infos(void){
	
	// Label temperatura
	label_temp = lv_label_create(lv_scr_act());
	lv_obj_align(label_temp, LV_ALIGN_LEFT_MID, 20 , -45);
	lv_obj_set_style_text_font(label_temp, &dseg70, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_temp, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label_temp, "%02d", 23);
	
	// Label temperatura decimal
	label_temp_decimal = lv_label_create(lv_scr_act());
	lv_obj_align_to(label_temp_decimal, label_temp, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0);
	lv_obj_set_style_text_font(label_temp_decimal, &dseg32, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_temp_decimal, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label_temp_decimal, ".%d", 4);
	
	// Label temperatura de referencia
	label_temp_reference = lv_label_create(lv_scr_act());
	lv_obj_align(label_temp_reference, LV_ALIGN_RIGHT_MID, -20 , -35);
	lv_obj_set_style_text_font(label_temp_reference, &dseg50, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_temp_reference, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label_temp_reference, "%02d", 22);
	
	// Label hora
	label_time = lv_label_create(lv_scr_act());
	lv_obj_align(label_time, LV_ALIGN_TOP_RIGHT, -20 , 10);
	lv_obj_set_style_text_font(label_time, &dseg30, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_time, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label_time, "%02d:%02d", 0, 0);
	
	
	
	// Demais labels
	
	// Label graus1
	label_graus1 = lv_label_create(lv_scr_act());
	lv_obj_align_to(label_graus1, label_temp, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	//lv_obj_set_style_text_font(label_graus1, &dseg20, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_graus1, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label_graus1, "ºC");
}

// void lc_image(){
// 	lv_obj_t * img = lv_img_create(lv_scr_act());
// 	lv_img_set_src(img, &V3_config);
// 	lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
// }




/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	int px, py;

	lv_termostato_buttons();
	lv_termostato_infos();
	

	for (;;)  {
		xSemaphoreTake( xMutexLVGL, portMAX_DELAY );
		lv_tick_inc(50);
		lv_task_handler();
		xSemaphoreGive( xMutexLVGL );
		vTaskDelay(50);
	}
}

static void task_rtc(void *pvParameters) {
	// RTC

	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_SECEN);

	for (;;)  {
		if (xSemaphoreTake(xSemaphoreRTC, 1000)){
		
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			
			char *c;
			c = lv_label_get_text(label_time);
					
			if(current_sec % 2 == 0){
				lv_label_set_text_fmt(label_time, "%02d:%02d", current_hour, current_min);
			} else {
				lv_label_set_text_fmt(label_time, "%02d %02d", current_hour, current_min);
			}
			
		
		}
	}
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();
	
	xSemaphoreRTC = xSemaphoreCreateBinary();
	if (xSemaphoreRTC == NULL)
	printf("falha em criar o semaforo do botão da placa \n");

	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	
		/* Create task to control rtc */
	if (xTaskCreate(task_rtc, "LCD", TASK_RTC_STACK_SIZE, NULL, TASK_RTC_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create rtc task\r\n");
	}
	
	/* Start the scheduler. */
	vTaskStartScheduler();
	
	xMutexLVGL = xSemaphoreCreateMutex();

	while(1){ }
}
