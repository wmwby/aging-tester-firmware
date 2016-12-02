

//#ifndef DHCP_USED
//#define DHCP_USED
//#endif

#include "board.h"
#include "system.h"
#include "usart.h"
#include "stdio_serial.h"
#include "delay.h"
#include "ethernet.h"
#include "httpd.h"
#include "conf_eth.h"
#include "smbus.h"
#include "timer_mgt.h"

#define STRING_EOL    "\r"
#define STRING_HEADER "-- Raw HTTP Basic Example -- \r\n" \
		"-- "BOARD_NAME" --\r\n" \
		"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

/** Software instance of the USART upon which to transmit the results. */
static struct usart_module usart_instance;

/** Set up the USART (EDBG) communication for debug purpose. */
//static void setup_usart_channel(void)
//{
	//struct usart_config config_usart;
	//usart_get_config_defaults(&config_usart);
//
	///* Configure the USART settings and initialize the standard I/O library */
	//config_usart.baudrate = 115200;
	//config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	//config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	//config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	//config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	//config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
//
	//stdio_serial_init(&usart_instance, EDBG_CDC_MODULE, &config_usart);
	//usart_enable(&usart_instance);
//}

void configure_pinmux(void);

/**
 * \brief Main program function. Configure the hardware, initialize lwIP
 * TCP/IP stack, and start HTTP service.
 */
int main(void)
{
	uint16_t timer_count = 0;
	/* Initialize the SAM system. */
	system_init();
	delay_init();
	configure_pinmux();
	/* Setup USART module to output results */
	//setup_usart_channel();

	///* Print example information. */
	////puts(STRING_HEADER);
//
	///* Bring up the ethernet interface & initialize timer0, channel0. */
	init_ethernet();
	
	smbus_init();

	/* Bring up the web server. */
	httpd_init();

	/* Program main loop. */
	while (1) 
	{
		/* Check for input packet and process it. */
		ethernet_task();
		if(sys_timer_30s_tick)
		{
			sys_timer_30s_tick = 0;
			smbus_read_all_info();
		}
	}
}


void configure_pinmux(void)
{
	struct system_pinmux_config config_pinmux;
	system_pinmux_get_config_defaults(&config_pinmux);
	
	config_pinmux.mux_position = 3;
	
	system_pinmux_pin_set_config(ETHERNET_SPI_MOSI,&config_pinmux);
	system_pinmux_pin_set_config(ETHERNET_SPI_SCK,&config_pinmux);
	system_pinmux_pin_set_config(ETHERNET_SPI_MISO,&config_pinmux);
}