#ifndef __FCP_H__
#define __FCP_H__


#define FCP_COMMAND 0xE0313233
#define READ_I2C_BYTE_MSG        0x10
#define WRITE_I2C_BYTE_MSG       0x11
#define SET_VIDEO_MODE_MSG       0x12
#define GET_FIRM_VERS_MSG        0x13
#define INPUT_SELECT_MSG         0x14
#define CHK_VIDEO_LOCK_MSG       0x15
#define ENA_ISOCH_TX_MSG         0x16
#define AVSYNC_EVERY_HLINE_MSG   0x17
#define READ_LINK_REG_MSG        0x18
#define WRITE_LINK_REG_MSG       0x19
#define SERIAL_NUMBER_MSG        0x1A
#define SET_VIDEO_FREQUENCY_MSG  0x1B

#if EXPERIMENTAL_FW
#define RS232_IO_MSG             0x1D
#define RS232_CONFIG_MSG         0x1E
#define LINK_SPEED_MSG           0x20
#endif 

#define ENTER_BOOTLOAD_MSG       0xB0

#define FCP_ACK_MSG              0xAA
#define FCP_REJECTED_MSG         0xAF
#define FCP_NOT_SUPP_MSG         0xE1

#endif//__FCP_H__

