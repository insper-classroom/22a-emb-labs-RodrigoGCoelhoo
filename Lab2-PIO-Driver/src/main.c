/**
 * 5 semestre - Eng. da Computação - Insper
 * Rafael Corsi - rafael.corsi@insper.edu.br
 *
 * Projeto 0 para a placa SAME70-XPLD
 *
 * Objetivo :
 *  - Introduzir ASF e HAL
 *  - Configuracao de clock
 *  - Configuracao pino In/Out
 *
 * Material :
 *  - Kit: ATMEL SAME70-XPLD - ARM CORTEX M7
 */

/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include "asf.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/

#define DELAY 100;

// Configuracoes do LED1
#define LED1_PIO           PIOA                 // periferico que controla o LED
#define LED1_PIO_ID        ID_PIOA              // ID do periférico PIOC (controla LED)
#define LED1_PIO_IDX       0                    // ID do LED no PIO
#define LED1_PIO_IDX_MASK  (1 << LED1_PIO_IDX)  // Mascara para CONTROLARMOS o LED

// Configuracoes do LED2
#define LED2_PIO           PIOC                 // periferico que controla o LED
#define LED2_PIO_ID        ID_PIOC              // ID do periférico PIOC (controla LED)
#define LED2_PIO_IDX       30                   // ID do LED no PIO
#define LED2_PIO_IDX_MASK  (1 << LED2_PIO_IDX)  // Mascara para CONTROLARMOS o LED

// Configuracoes do LED3
#define LED3_PIO           PIOB                 // periferico que controla o LED
#define LED3_PIO_ID        ID_PIOB              // ID do periférico PIOC (controla LED)
#define LED3_PIO_IDX       2                    // ID do LED no PIO
#define LED3_PIO_IDX_MASK  (1 << LED3_PIO_IDX)  // Mascara para CONTROLARMOS o LED

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

/*  Default pin configuration (no attribute). */
#define _PIO_DEFAULT             (0u << 0)
/*  The internal pin pull-up is active. */
#define _PIO_PULLUP              (1u << 0)
/*  The internal glitch filter is active. */
#define _PIO_DEGLITCH            (1u << 1)
/*  The internal debouncing filter is active. */
#define _PIO_DEBOUNCE            (1u << 3)

/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

void init(void);

/************************************************************************/
/* interrupcoes                                                         */
/************************************************************************/

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

/**
* \brief Set a high output level on all the PIOs defined in ul_mask.
* This has no immediate effects on PIOs that are not output, but the PIO
* controller will save the value if they are changed to outputs.
*
* \param p_pio Pointer to a PIO instance.
* \param ul_mask Bitmask of one or more pin(s) to configure.
*/
void _pio_set(Pio *p_pio, const uint32_t ul_mask) {
	
	p_pio->PIO_SODR = ul_mask;
}

/**
* \brief Set a low output level on all the PIOs defined in ul_mask.
* This has no immediate effects on PIOs that are not output, but the PIO
* controller will save the value if they are changed to outputs.
*
* \param p_pio Pointer to a PIO instance.
* \param ul_mask Bitmask of one or more pin(s) to configure.
*/
void _pio_clear(Pio *p_pio, const uint32_t ul_mask) {
	
	p_pio->PIO_CODR = ul_mask;
}

/**
* \brief Configure PIO internal pull-up.
*
* \param p_pio Pointer to a PIO instance.
* \param ul_mask Bitmask of one or more pin(s) to configure.
* \param ul_pull_up_enable Indicates if the pin(s) internal pull-up shall be
* configured.
*/
void _pio_pull_up(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_pull_up_enable){
	
	if (ul_pull_up_enable) {
		p_pio->PIO_PUER = ul_mask;
	}
	else {
		p_pio->PIO_PUDR = ul_mask;
	}
}

/**
 * \brief Configure one or more pin(s) or a PIO controller as inputs.
 * Optionally, the corresponding internal pull-up(s) and glitch filter(s) can
 * be enabled.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure as input(s).
 * \param ul_attribute PIO attribute(s).
 */
void _pio_set_input(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_attribute) {

	p_pio->PIO_ODR = ul_mask;
	p_pio->PIO_PER = ul_mask;
	
	if(ul_attribute & _PIO_PULLUP){
		pio_pull_up(p_pio, ul_mask, 1);
	}
	
	if(ul_attribute & (_PIO_DEGLITCH | _PIO_DEBOUNCE)){
		p_pio->PIO_IFER = ul_mask;
		
		if (ul_attribute & _PIO_DEBOUNCE){
			p_pio->PIO_IFSCER = ul_mask;
			p_pio->PIO_SCDR = ul_mask;
		}
		
		if (ul_attribute & _PIO_DEGLITCH){
			p_pio->PIO_IFSCDR = ul_mask;
		}
	}
	else{
		p_pio->PIO_IFDR = ul_mask;
	}
	
}

/**
 * \brief Configure one or more pin(s) of a PIO controller as outputs, with
 * the given default value. Optionally, the multi-drive feature can be enabled
 * on the pin(s).
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure.
 * \param ul_default_level Default level on the pin(s).
 * \param ul_multidrive_enable Indicates if the pin(s) shall be configured as
 * open-drain.
 * \param ul_pull_up_enable Indicates if the pin shall have its pull-up
 * activated.
 */
void _pio_set_output(Pio *p_pio, const uint32_t ul_mask,
        const uint32_t ul_default_level,
        const uint32_t ul_multidrive_enable,
        const uint32_t ul_pull_up_enable)
{
	
	p_pio->PIO_PER = ul_mask;
	p_pio->PIO_OER = ul_mask;
	
	if (ul_default_level){
		_pio_set(p_pio, ul_mask);
	}
	else{
		_pio_clear(p_pio, ul_mask);
	}
	
	if (ul_multidrive_enable){
		p_pio->PIO_MDER = ul_mask;
		} else {
		p_pio->PIO_MDDR = ul_mask;
	}
	
	pio_pull_up(p_pio, ul_mask, ul_pull_up_enable);
	
}

// Funcao de ligar o led
void led_cycle(Pio *p_pio, const uint32_t ul_mask, int delay){
	for(int i = 0; i < 5; i++){
		_pio_set(p_pio, ul_mask);
		delay_ms(delay);
		_pio_clear(p_pio, ul_mask);
		delay_ms(delay);
	}
}

// Função de inicialização do uC
void init(void)
{
  // Initialize the board clock
  sysclk_init();

  // Desativa WatchDog Timer
  WDT->WDT_MR = WDT_MR_WDDIS;
  
  // Ativa o PIO na qual o LED foi conectado
  // para que possamos controlar o LED.
  pmc_enable_periph_clk(LED1_PIO_ID);
  pmc_enable_periph_clk(LED2_PIO_ID);
  pmc_enable_periph_clk(LED3_PIO_ID);
  
  //Inicializa pinos dos LEDs como saída
  _pio_set_output(LED1_PIO, LED1_PIO_IDX_MASK, 0, 0, 0);
  _pio_set_output(LED2_PIO, LED2_PIO_IDX_MASK, 0, 0, 0);
  _pio_set_output(LED3_PIO, LED3_PIO_IDX_MASK, 0, 0, 0);
  
  
  // Inicializa PIO do botao
  pmc_enable_periph_clk(BUT1_PIO_ID);
  pmc_enable_periph_clk(BUT2_PIO_ID);
  pmc_enable_periph_clk(BUT3_PIO_ID);
  
  // configura pino ligado ao botão como entrada com um pull-up.
  _pio_set_input(BUT1_PIO, BUT1_PIO_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);
  _pio_set_input(BUT2_PIO, BUT2_PIO_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);
  _pio_set_input(BUT3_PIO, BUT3_PIO_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);
  
  // Configura pull-up do botao
  _pio_pull_up(BUT1_PIO, BUT1_PIO_IDX_MASK, 1);
  _pio_pull_up(BUT2_PIO, BUT2_PIO_IDX_MASK, 1);
  _pio_pull_up(BUT3_PIO, BUT3_PIO_IDX_MASK, 1);

}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/


// Funcao principal chamada na inicalizacao do uC.
int main(void) {
	
  init();
	
  while(1){
	  if(!pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK)){
		  
		  led_cycle(LED1_PIO, LED1_PIO_IDX_MASK, 100);
		  
	  }
	  else if (!pio_get(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK)){
		  
		  led_cycle(LED2_PIO, LED2_PIO_IDX_MASK, 500);
		  
	  }
	  else if (!pio_get(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK)){
		  
		  led_cycle(LED3_PIO, LED3_PIO_IDX_MASK, 1000);
		  
	  } else  {
		  
		  // Ativa o pino LED_IDX (para apagar)
		  _pio_set(LED1_PIO, LED1_PIO_IDX_MASK);
		  _pio_set(LED2_PIO, LED2_PIO_IDX_MASK);
		  _pio_set(LED3_PIO, LED3_PIO_IDX_MASK);
	  }
  }
  
  return 0;
}
