/**
 * \file
 *
 * \brief Timer management for the lwIP Raw HTTP basic example.
 *
 * Copyright (c) 2013-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#include <asf.h>
#include "board.h"
#include "tc.h"
#include "timer_mgt.h"
#include "lwip/init.h"
#include "lwip/sys.h"

/* Clock tick count. */
static volatile uint32_t gs_ul_clk_tick;

#if SAMD20
#include "tc_interrupt.h"
struct tc_module tc_instance;
volatile uint8_t sys_timer_30s_tick = 0;
volatile uint16_t sys_delay_ms_counter = 0;

/*
 * timer counter callback function
 * 1. led blinks every 1 second
 * 2. 30s timer tick
 * 3. implement ms delay
 */
static void tc_callback(struct tc_module *const module_inst)
{
	static uint16_t counter = 0;
	static uint16_t counter_led = 0;
	/* Increase tick. */
	gs_ul_clk_tick++;

	if(sys_delay_ms_counter)
	{
		sys_delay_ms_counter--;
	}

	if(counter == 9999)
	{
		counter = 0;
		sys_timer_30s_tick = 1;
	}
	else
	{
		counter++;
	}

	if(counter_led == 499)
	{
		counter_led = 0;
		port_pin_toggle_output_level(MCU_LED1);
	}
	else
	{
		counter_led++;
	}
}
/* led pin configure function */
static void configure_led(void)
{
	struct port_config config_led;
	port_get_config_defaults(&config_led);

	config_led.direction = PORT_PIN_DIR_OUTPUT;

	port_pin_set_config(MCU_LED1,&config_led);
	port_pin_set_output_level(MCU_LED1,true);
}

/* 
 * ms delay function
 */
uint16_t sys_delay_ms(uint16_t ms)
{
	if(ms)
	{
		sys_delay_ms_counter = ms;
	}
	return sys_delay_ms_counter;
}


void sys_init_timing(void)
{
	struct tc_config config_tc;
	tc_get_config_defaults(&config_tc);
	configure_led();

	config_tc.counter_size    = TC_COUNTER_SIZE_16BIT;
	config_tc.wave_generation = TC_WAVE_GENERATION_MATCH_FREQ;
	config_tc.counter_16_bit.compare_capture_channel[0] = 0x5DC0;
	config_tc.clock_source = GCLK_GENERATOR_0;
	config_tc.clock_prescaler = TC_CLOCK_PRESCALER_DIV2;

	tc_init(&tc_instance, TC0, &config_tc);
	tc_enable(&tc_instance);
	tc_register_callback(&tc_instance, tc_callback, TC_CALLBACK_CC_CHANNEL0);
	tc_enable_callback(&tc_instance, TC_CALLBACK_CC_CHANNEL0);

	/* Enable system interrupts. */
	system_interrupt_enable_global();
}
#else
#include "pmc.h"
#include "sysclk.h"

/**
 * TC0 Interrupt handler.
 */
void TC0_Handler(void)
{
	/* Remove warnings. */
	volatile uint32_t ul_dummy;

	/* Clear status bit to acknowledge interrupt. */
	ul_dummy = TC0->TC_CHANNEL[0].TC_SR;

	/* Increase tick. */
	gs_ul_clk_tick++;
}

/**
 * \brief Initialize the timer counter (TC0).
 */
void sys_init_timing(void)
{
	uint32_t ul_div;
	uint32_t ul_tcclks;

	/* Clear tick value. */
	gs_ul_clk_tick = 0;

	/* Configure PMC. */
	pmc_enable_periph_clk(ID_TC0);

	/* Configure TC for a 1kHz frequency and trigger on RC compare. */
	tc_find_mck_divisor(1000,
			sysclk_get_main_hz(), &ul_div, &ul_tcclks,
			sysclk_get_main_hz());
	tc_init(TC0, 0, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 0, (sysclk_get_main_hz() / ul_div) / 1000);

	/* Configure and enable interrupt on RC compare. */
	NVIC_EnableIRQ((IRQn_Type)ID_TC0);
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);

	/* Start timer. */
	tc_start(TC0, 0);
}
#endif

/**
 * \brief Return the number of timer ticks (ms).
 */
uint32_t sys_get_ms(void)
{
	return gs_ul_clk_tick;
}

#if ((LWIP_VERSION) != ((1U << 24) | (3U << 16) | (2U << 8) | (LWIP_VERSION_RC)))
u32_t sys_now(void)
{
	return (sys_get_ms());
}
#endif
