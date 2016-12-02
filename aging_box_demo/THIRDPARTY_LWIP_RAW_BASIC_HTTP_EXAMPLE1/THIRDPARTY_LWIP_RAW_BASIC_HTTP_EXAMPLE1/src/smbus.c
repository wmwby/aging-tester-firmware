/*
 * smbus.c
 *
 * Created: 2016/9/5 11:19:17
 *  Author: mingwei.wu
 */ 

#include <asf.h>
#include "smbus.h"
#include "timer_mgt.h"

struct i2c_master_module i2c_master_instance;
uint8_t smbus_err_code = 0;
uint8_t smbus_port_polling = 0;

uint8_t smbus_log_pingpang = 0;
uint8_t ssd_port_log_pingpang0[ETH_SSD_LOG_LENGTH*8] = {0};
uint8_t ssd_port_log_pingpang1[ETH_SSD_LOG_LENGTH*8] = {0};
uint8_t aging_box_log_pingpang0[ETH_AGING_BOX_INFO_LENGTH] = {0};
uint8_t aging_box_log_pingpang1[ETH_AGING_BOX_INFO_LENGTH] = {0};


uint8_t rx_buff_basic[BASIC_INFO_BUFFER_SIZE] = {0};
uint8_t rx_buff_oob[OOB_INFO_BUFFER_SIZE] = {0};
uint8_t rx_buff_bi[BI_INFO_BUFFER_SIZE] = {0};

/*
 * i2c master module configuration function
 */
static void configure_i2c_master(void)
{
	struct i2c_master_config config_i2c_master;
	i2c_master_get_config_defaults(&config_i2c_master);
	config_i2c_master.baud_rate = I2C_MASTER_BAUD_RATE_400KHZ;
	config_i2c_master.buffer_timeout = 10000;
	config_i2c_master.pinmux_pad0 = SMBUS_SDA;
	config_i2c_master.pinmux_pad1 = SMBUS_SCL;
	i2c_master_init(&i2c_master_instance,SMBUS_MODULE,&config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
}

/*
 * i2c master module pinmux function
 */
static void pinmux_smbus(void)
{
	struct system_pinmux_config config_pinmux;
	system_pinmux_get_config_defaults(&config_pinmux);
	
	config_pinmux.mux_position = SMBUS_PINMUX;
	
	system_pinmux_pin_set_config(SMBUS_SCL,&config_pinmux);
	system_pinmux_pin_set_config(SMBUS_SDA,&config_pinmux);
}

static void ssd_log_init(void)
{
	uint8_t i = 0;
	for(i = 0;i < 8;i++)
	{
		ssd_port_log_pingpang0[0+i*ETH_SSD_LOG_LENGTH] = 0;	// means get info command
		ssd_port_log_pingpang0[2+i*ETH_SSD_LOG_LENGTH] = i;	// means ssd port index
		ssd_port_log_pingpang0[3+i*ETH_SSD_LOG_LENGTH] = 1; // means present
		ssd_port_log_pingpang1[0+i*ETH_SSD_LOG_LENGTH] = 0;	// means get info command
		ssd_port_log_pingpang1[2+i*ETH_SSD_LOG_LENGTH] = i;	// means ssd port index
		ssd_port_log_pingpang1[3+i*ETH_SSD_LOG_LENGTH] = 1; // means present
	}
	aging_box_log_pingpang0[0] = 0;
	aging_box_log_pingpang0[2] = 9;
	aging_box_log_pingpang1[0] = 0;
	aging_box_log_pingpang1[2] = 9;
}
/*
 * i2c master module init function
 */
void smbus_init(void)
{
	configure_i2c_master();
	pinmux_smbus();
	ssd_log_init();
}
/* 
 * checksum generator function 
 */
static uint8_t check_sum_generator(uint8_t *buff,uint8_t num)
{
	uint8_t i = 0;
	uint8_t checksum = 0;
	for(i = 0;i < num;i++)
	{
		checksum = checksum + (uint8_t)(*(buff+i));
	}

	return checksum;
}

/*
 * Brief: read basic info function
 *
 */
static void smbus_read_basic_info(void)
{
	enum status_code err = STATUS_OK;
	uint8_t tx_buff_basic[DATA_LENGTH] = {0x0a,0x00,0xff,0xff,0x06,0x00,0x01,0x00,0x00,0x00};
	volatile uint16_t timeout = 0;
	uint8_t check_sum = 0;
	
	tx_buff_basic[DATA_LENGTH-1] = check_sum_generator((uint8_t *)tx_buff_basic,(DATA_LENGTH-1));

	struct i2c_master_packet packet =
	{
		.address = SLAVE_ADDRESS,
		.data_length = DATA_LENGTH,
		.data = tx_buff_basic,
		.ten_bit_address = false,
		.high_speed = false,
		.hs_master_code = 0x0,
	};

	err = i2c_master_write_packet_wait(&i2c_master_instance,&packet);
	while(err != STATUS_OK)
	{
		err = i2c_master_write_packet_wait(&i2c_master_instance,&packet);
		timeout++;
		if(timeout == TIMEOUT)
		{
			smbus_err_code = 1;	//basic info: write error
			return;
		}
	}

	packet.data = (uint8_t *)rx_buff_basic;
	packet.data_length = BASIC_INFO_BUFFER_SIZE;

	/* delay, waiting for i2c ready*/
	sys_delay_ms(DELAY_MS);
	while(sys_delay_ms(0));
	//timeout = 30000;
	//while(timeout--);

	err = i2c_master_read_packet_wait(&i2c_master_instance,&packet);
	while(err != STATUS_OK)
	{
		err = i2c_master_read_packet_wait(&i2c_master_instance,&packet);
		timeout++;
		if(timeout == TIMEOUT)
		{
			smbus_err_code = 2;	//basic info: read error
			return;
		}
	}
	check_sum = check_sum_generator((uint8_t *)rx_buff_basic,(BASIC_INFO_BUFFER_SIZE-1));
	if(check_sum != rx_buff_basic[BASIC_INFO_BUFFER_SIZE-1])
	{
		smbus_err_code = 3;
	}
}

/*
 * Brief: read oob info function
 *
 * if fail, return err;
 * if succeed, write the new oob info to "all_bi_info_buffer", and return OK
 */
static void smbus_read_oob_info(void)
{
	enum status_code err = STATUS_OK;
	uint8_t tx_buff_oob[DATA_LENGTH] = {0x0a,0x00,0xff,0xff,0x06,0x00,0x02,0x00,0x00,0x00};
	volatile uint16_t timeout = 0;
	uint8_t check_sum = 0;
	
	tx_buff_oob[DATA_LENGTH-1] = check_sum_generator((uint8_t *)tx_buff_oob,(DATA_LENGTH-1));

	struct i2c_master_packet packet =
	{
		.address = SLAVE_ADDRESS,
		.data_length = DATA_LENGTH,
		.data = tx_buff_oob,
		.ten_bit_address = false,
		.high_speed = false,
		.hs_master_code = 0x0,
	};

	err = i2c_master_write_packet_wait(&i2c_master_instance,&packet);
	while(err != STATUS_OK)
	{
		err = i2c_master_write_packet_wait(&i2c_master_instance,&packet);
		timeout++;
		if(timeout == TIMEOUT)
		{
			smbus_err_code = 11;	//oob info: write error
			return;
		}
	}

	packet.data = (uint8_t *)rx_buff_oob;
	packet.data_length = OOB_INFO_BUFFER_SIZE;

	/* delay, waiting for i2c ready*/
	sys_delay_ms(DELAY_MS);
	while(sys_delay_ms(0));

	err = i2c_master_read_packet_wait(&i2c_master_instance,&packet);
	while(err != STATUS_OK)
	{
		err = i2c_master_read_packet_wait(&i2c_master_instance,&packet);
		timeout++;
		if(timeout == TIMEOUT)
		{
			smbus_err_code = 12;	//oob info: read error
			return;
		}
	}
	check_sum = check_sum_generator((uint8_t *)rx_buff_oob,(OOB_INFO_BUFFER_SIZE-1));
	if(check_sum != rx_buff_oob[OOB_INFO_BUFFER_SIZE-1])
	{
		smbus_err_code = 13;
	}
}

/*
 * Brief: read bi info function
 *
 * if fail, return err;
 * if succeed, write the new bi info to "all_bi_info_buffer", and return OK
 */
static void smbus_read_bi_info(void)
{
	enum status_code err = STATUS_OK;
	uint8_t tx_buff_bi[DATA_LENGTH] = {0x0a,0x00,0xff,0xff,0x06,0x00,0x03,0x00,0x00,0x00};
	volatile uint16_t timeout = 0;
	uint8_t check_sum = 0;

	tx_buff_bi[DATA_LENGTH-1] = check_sum_generator((uint8_t *)tx_buff_bi,(DATA_LENGTH-1));
	
	struct i2c_master_packet packet =
	{
		.address = SLAVE_ADDRESS,
		.data_length = DATA_LENGTH,
		.data = tx_buff_bi,
		.ten_bit_address = false,
		.high_speed = false,
		.hs_master_code = 0x0,
	};

	err = i2c_master_write_packet_wait(&i2c_master_instance,&packet);
	while(err != STATUS_OK)
	{
		err = i2c_master_write_packet_wait(&i2c_master_instance,&packet);
		timeout++;
		if(timeout == TIMEOUT)
		{
			smbus_err_code = 11;	//bi info: write error
			return;
		}
	}

	packet.data = (uint8_t *)rx_buff_bi;
	packet.data_length = BI_INFO_BUFFER_SIZE;

	/* delay, waiting for i2c ready*/
	sys_delay_ms(DELAY_MS);
	while(sys_delay_ms(0));

	err = i2c_master_read_packet_wait(&i2c_master_instance,&packet);
	while(err != STATUS_OK)
	{
		err = i2c_master_read_packet_wait(&i2c_master_instance,&packet);
		timeout++;
		if(timeout == TIMEOUT)
		{
			smbus_err_code = 22;	//bi info: read error
			return;
		}
	}
	check_sum = check_sum_generator((uint8_t *)rx_buff_bi,(BI_INFO_BUFFER_SIZE-1));
	if(check_sum != rx_buff_bi[BI_INFO_BUFFER_SIZE-1])
	{
		smbus_err_code = 23;
	}
}
/*
 * aging log package function
 * write the aging log coming from oob to associated sram location
 */
static void smbus_log_package(void)
{
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t *p_ssd_log;
	/* select port array and pingpang operation */
	if(smbus_log_pingpang == 0 || smbus_log_pingpang == 2)
	{
		p_ssd_log = (uint8_t *)(ssd_port_log_pingpang0+smbus_port_polling*ETH_SSD_LOG_LENGTH);
	}
	else
	{
		p_ssd_log = (uint8_t *)(ssd_port_log_pingpang1+smbus_port_polling*ETH_SSD_LOG_LENGTH);
	}
	/* error code*/
	*(p_ssd_log+1) = smbus_err_code;
	/* Basic Info */
	/* serial number */
	for(i = 16,j = HEAD_LEN+TRACE_LEN;i < (16+SN_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_basic[j];
	}
	/* model number */
	for(j+=TRACE_LEN;i < (16+SN_LEN+MN_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_basic[j];
	}
	/* oob firmware version */
	for(j+=TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_basic[j];
	}
	/* BI firmware version */
	for(j+=TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_basic[j];
	}
	/* OOB info */
	/* temperature */
	for(j=HEAD_LEN+TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN+TEM_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_oob[j];
	}
	/* oob information*/
	for(j+=TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN+TEM_LEN+OOB_INFO_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_oob[j];
	}
	/* power good */
	for(j=TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN+TEM_LEN+OOB_INFO_LEN+PG_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_oob[j];
	}
	/* BI Info */
	/* BI loop, 2 bytes*/
	for(j = HEAD_LEN+TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN+TEM_LEN+OOB_INFO_LEN+PG_LEN+BI_LOOP_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_bi[j];
	}
	/* BI Error */
	for(j+=TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN+TEM_LEN+OOB_INFO_LEN+PG_LEN+BI_LOOP_LEN+BI_ERR_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_bi[j];
	}
	/* BI log */
	for(j+=TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN+TEM_LEN+OOB_INFO_LEN+PG_LEN+BI_LOOP_LEN+BI_ERR_LEN+BI_LOG_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_bi[j];
	}
	/* BI elapsed */
	for(j+=TRACE_LEN;i < (16+SN_LEN+MN_LEN+OOB_FW_LEN+BI_FW_LEN+TEM_LEN+OOB_INFO_LEN+PG_LEN+BI_LOOP_LEN+BI_ERR_LEN+BI_LOG_LEN+BI_ELA_LEN);i++,j++)
	{
		*(p_ssd_log+i) = rx_buff_bi[j];
	}
}

/*
 * read all aging log function, including basic, oob and bi information
 * get all 8 ports info at once, with pingpang operation
 */
void smbus_read_all_info(void)
{
	for(smbus_port_polling = 0;smbus_port_polling < 8;smbus_port_polling++)
	{
		smbus_err_code = 0;
		smbus_read_basic_info();
		if(smbus_err_code)
		{
			goto package;
		}
		smbus_read_oob_info();
		if(smbus_err_code)
		{
			goto package;
		}
		smbus_read_bi_info();
		if(smbus_err_code)
		{
			goto package;
		}
		package:
		smbus_log_package();
	}
	/* update pingpang flag */
	if(smbus_log_pingpang == 0 || smbus_log_pingpang == 2)
	{
		smbus_log_pingpang = 1;
	}
	else
	{
		smbus_log_pingpang = 2;
	}
}
