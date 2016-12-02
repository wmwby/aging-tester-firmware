/*
 * smbus.h
 *
 * Created: 2016/9/5 11:19:02
 *  Author: mingwei.wu
 */ 


#ifndef SMBUS_H_
#define SMBUS_H_

#define SLAVE_ADDRESS	0x6A
#define DATA_LENGTH		10
#define TIMEOUT			100
#define SMBUS_MODULE	SERCOM3//SERCOM0//SERCOM5
#define SMBUS_SCL		PIN_PA23//PIN_PA08//PIN_PB17
#define SMBUS_SDA		PIN_PA22//PIN_PA09//PIN_PB16
#define DELAY_MS		5

#define SMBUS_PINMUX	2

#define BASIC_INFO_BUFFER_SIZE	105
#define OOB_INFO_BUFFER_SIZE	46
#define BI_INFO_BUFFER_SIZE		37
#define BI_FRAME_HEADER			0xffff

#define ETH_SSD_LOG_LENGTH			150
#define ETH_AGING_BOX_INFO_LENGTH	30
#define ETH_AGING_BOX_CMD_LENGTH	10

/* from bi protocol written by fengbiao */
#define SN_LEN			20
#define MN_LEN			40
#define OOB_FW_LEN		12
#define BI_FW_LEN		12
#define TEM_LEN			10
#define OOB_INFO_LEN	18
#define PG_LEN			1
#define BI_LOOP_LEN		2
#define BI_ERR_LEN		4
#define BI_LOG_LEN		6
#define BI_ELA_LEN		4
#define TRACE_LEN		4	// the length of ID and length
#define HEAD_LEN		4


extern uint8_t ssd_port_log_pingpang0[ETH_SSD_LOG_LENGTH*8];
extern uint8_t ssd_port_log_pingpang1[ETH_SSD_LOG_LENGTH*8];
extern uint8_t aging_box_log_pingpang0[ETH_AGING_BOX_INFO_LENGTH];
extern uint8_t aging_box_log_pingpang1[ETH_AGING_BOX_INFO_LENGTH];
/*
 * error code
 * 1: it happens error when write basic-info command to oob
 * 2: it happens error when read basic-info from oob
 * 3: checksum is WRONG after receiving basic-info
 * 11: it happens error when write oob-info command to oob
 * 12: it happens error when read oob-info from oob
 * 13: checksum is WRONG after receiving oob-info
 * 21: it happens error when write bi-info command to oob
 * 22: it happens error when read bi-info from oob
 * 23:  checksum is WRONG after receiving bi-info
 * 31: initialization is NOT completed 
 */
extern uint8_t smbus_err_code;
extern uint8_t smbus_log_pingpang;

void smbus_init(void);
void smbus_read_all_info(void);

#endif /* SMBUS_H_ */