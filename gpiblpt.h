/*
 *  gpiblpt.h
 *
 *  Created by Léa Strobino.
 *  Copyright 2017 hepia. All rights reserved.
 *
 *
 *  Registers:           Bit 7  Bit 6  Bit 5  Bit 4  Bit 3  Bit 2  Bit 1  Bit 0
 *  ----------------------------------------------------------------------------
 *                       _____  _____  _____  _____  _____  _____  _____  _____
 *  LPT_DATA (in/out)    DIO 8  DIO 7  DIO 6  DIO 5  DIO 4  DIO 3  DIO 2  DIO 1
 *                                ___    ___   ____   ____
 *  LPT_STAT (in)          EOI    SRQ    DAV   NDAC   NRFD      -      -      -
 *                                      ____                  ___
 *  LPT_CTRL (out)           -      -   DOUT      -   NDAC    ATN   NRFD    DAV
 *
 *
 *  Cable pinout
 *  ------------
 *
 *  LPT:    1    2    3    4    5    6    7    8    9   10   11   12   13
 *  GPIB:   6    1    2    3    4   13   14   15   16   10    5    6    8
 *
 *  LPT:   14   15   16   17   18   19   20   21   22   23   24   25   shield
 *  GPIB:   7    7   11    8   18   19   20   21   22   23   24   17   12
 *
 */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void mexAtExitFunction();
void nargchk(int nlhs, int required_nlhs, int nrhs, int required_nrhs);
void error(const char *f, uint32_t id);

#define LPT_DATA 0xD010
#define LPT_STAT 0xD011
#define LPT_CTRL 0xD012
#define LPT_ECR  0xD002

#define DRIVER_ID "WinRing0_1_2_0"
#define IOCTL_READ_PORT_UCHAR  CTL_CODE(40000,0x833,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_WRITE_PORT_UCHAR CTL_CODE(40000,0x836,METHOD_BUFFERED,FILE_WRITE_ACCESS)
uint8_t READ_PORT_UCHAR(uint32_t port);
void WRITE_PORT_UCHAR(uint32_t port, uint8_t data);

#define setLPTByteMode() WRITE_PORT_UCHAR(LPT_ECR,0x21)

#define GPIB_CMD_GTL 0x01
#define GPIB_CMD_SDC 0x04
#define GPIB_CMD_GET 0x08
#define GPIB_CMD_DCL 0x14
#define GPIB_CMD_SPE 0x18
#define GPIB_CMD_SPD 0x19
#define GPIB_CMD_LIS 0x20
#define GPIB_CMD_UNL 0x3F
#define GPIB_CMD_TAL 0x40
#define GPIB_CMD_UNT 0x5F

void GPIB_readString(uint8_t addr, uint8_t *data, uint32_t max_length, uint8_t useEOI, uint8_t useTerminator, uint8_t terminator, uint32_t *length);
void GPIB_writeString(uint8_t *addr, uint8_t n_addr, uint8_t *data, uint32_t length);

void GPIB_addressedCommand(uint8_t *addr, uint8_t n_addr, uint8_t cmd);
#define GPIB_goToLocal(addr,n_addr) GPIB_addressedCommand(addr,n_addr,GPIB_CMD_GTL)
#define GPIB_selectedDeviceClear(addr,n_addr) GPIB_addressedCommand(addr,n_addr,GPIB_CMD_SDC)
#define GPIB_groupExecuteTrigger(addr,n_addr) GPIB_addressedCommand(addr,n_addr,GPIB_CMD_GET)
void GPIB_deviceClear();

void GPIB_readDataByte(uint8_t *data, uint8_t *EOI);
void GPIB_writeDataByte(uint8_t data);
void GPIB_writeCmd(uint8_t cmd);
void GPIB_readSRQ(uint8_t *SRQ);
void GPIB_serialPollEnable();
uint8_t GPIB_serialPoll(uint8_t addr, uint8_t *data);
void GPIB_serialPollDisable();

#define read_DIO() ~READ_PORT_UCHAR(LPT_DATA)
#define read_EOI() !!(READ_PORT_UCHAR(LPT_STAT) & 0x80)
#define read_SRQ() !(READ_PORT_UCHAR(LPT_STAT) & 0x40)
#define write_DIO(DIO) WRITE_PORT_UCHAR(LPT_DATA,~DIO)
#define write_DAV_NRFD_NDAC_ATN_DOUT(DAV,NRFD,NDAC,ATN,DOUT) WRITE_PORT_UCHAR(LPT_CTRL,(!DOUT << 5)|(NDAC << 3)|(!ATN << 2)|(NRFD << 1)|(DAV))
#define setModeTalker() write_DAV_NRFD_NDAC_ATN_DOUT(0,0,0,0,0)
#define setModeListener() write_DAV_NRFD_NDAC_ATN_DOUT(0,1,1,0,0)

void waitfor_LPT_STAT(uint8_t mask, uint8_t value, const char *f);
#define waitfor_DAV(value)  waitfor_LPT_STAT(0x20,!value,"waitfor_DAV")
#define waitfor_NRFD(value) waitfor_LPT_STAT(0x08,!value,"waitfor_NRFD")
#define waitfor_NDAC(value) waitfor_LPT_STAT(0x10,!value,"waitfor_NDAC")
