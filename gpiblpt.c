/*
 *  gpiblpt.c
 *
 *  Created by Léa Strobino.
 *  Copyright 2017 hepia. All rights reserved.
 *
 */

#include <mex.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "gpiblpt.h"

static SC_HANDLE driver = INVALID_HANDLE_VALUE;
static unsigned long bytesReturned;
static uint16_t timeout;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  
  mexAtExit(mexAtExitFunction);
  
  if (driver == INVALID_HANDLE_VALUE) {
    
    /* Open driver */
    driver = CreateFile("\\\\.\\"DRIVER_ID,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (driver == INVALID_HANDLE_VALUE) mexErrMsgIdAndTxt("gpiblpt:DriverError","Unable to open the driver.");
    
    /* Configure LPT in byte mode (bi-directional) */
    setLPTByteMode();
    setModeTalker();
    
  }
  
  /* Get function name */
  if ((nrhs < 1) || !mxIsChar(prhs[0])) mexErrMsgIdAndTxt("gpiblpt:MissingFunction","Missing function.");
  char *f = mxArrayToString(prhs[0]);
  
  /* Get timeout */
  timeout = *(uint16_t*)mxGetData(prhs[nrhs-1]);
  
  if (strcmp(f,"readString") == 0) {
    
    nargchk(nlhs,1,nrhs,7);
    
    uint8_t *addr = mxGetData(prhs[1]);
    uint32_t *max_length = mxGetData(prhs[2]);
    uint8_t *useEOI = mxGetData(prhs[3]);
    uint8_t *useTerminator = mxGetData(prhs[4]);
    uint8_t *terminator = mxGetData(prhs[5]);
    
    plhs[0] = mxCreateNumericMatrix(*max_length,1,mxUINT8_CLASS,mxREAL);
    uint8_t *data = mxGetData(plhs[0]);
    uint32_t length;
    
    GPIB_readString(*addr,data,*max_length,*useEOI,*useTerminator,*terminator,&length);
    mxSetM(plhs[0],length);
    
  } else if (strcmp(f,"writeString") == 0) {
    
    nargchk(nlhs,0,nrhs,4);
    
    uint8_t *addr = mxGetData(prhs[1]);
    uint8_t n_addr = mxGetM(prhs[1])*mxGetN(prhs[1]);
    uint8_t *data = mxGetData(prhs[2]);
    uint32_t length = mxGetM(prhs[2])*mxGetN(prhs[2]);
    
    GPIB_writeString(addr,n_addr,data,length);
    
  } else if (strcmp(f,"goToLocal") == 0) {
    
    nargchk(nlhs,0,nrhs,3);
    
    uint8_t *addr = mxGetData(prhs[1]);
    uint8_t n_addr = mxGetM(prhs[1])*mxGetN(prhs[1]);
    
    GPIB_goToLocal(addr,n_addr);
    
  } else if (strcmp(f,"selectedDeviceClear") == 0) {
    
    nargchk(nlhs,0,nrhs,3);
    
    uint8_t *addr = mxGetData(prhs[1]);
    uint8_t n_addr = mxGetM(prhs[1])*mxGetN(prhs[1]);
    
    GPIB_selectedDeviceClear(addr,n_addr);
    
  } else if (strcmp(f,"groupExecuteTrigger") == 0) {
    
    nargchk(nlhs,0,nrhs,3);
    
    uint8_t *addr = mxGetData(prhs[1]);
    uint8_t n_addr = mxGetM(prhs[1])*mxGetN(prhs[1]);
    
    GPIB_groupExecuteTrigger(addr,n_addr);
    
  } else if (strcmp(f,"deviceClear") == 0) {
    
    nargchk(nlhs,0,nrhs,2);
    
    GPIB_deviceClear();
    
  } else if (strcmp(f,"serialPollEnable") == 0) {
    
    nargchk(nlhs,0,nrhs,2);
    
    GPIB_serialPollEnable();
    
  } else if (strcmp(f,"serialPoll") == 0) {
    
    nargchk(nlhs,1,nrhs,3);
    
    uint8_t *addr = mxGetData(prhs[1]);
    
    uint8_t d;
    if (GPIB_serialPoll(*addr,&d)) {
      plhs[0] = mxCreateNumericMatrix(1,1,mxUINT8_CLASS,mxREAL);
      *(uint8_t*)mxGetData(plhs[0]) = d;
    } else {
      plhs[0] = mxCreateNumericMatrix(0,0,mxUINT8_CLASS,mxREAL);
    }
    
  } else if (strcmp(f,"serialPollDisable") == 0) {
    
    nargchk(nlhs,0,nrhs,2);
    
    GPIB_serialPollDisable();
    
  } else if (strcmp(f,"readSRQ") == 0) {
    
    nargchk(nlhs,1,nrhs,2);
    
    uint8_t SRQ;
    GPIB_readSRQ(&SRQ);
    
    plhs[0] = mxCreateLogicalScalar(SRQ);
    
  } else mexErrMsgIdAndTxt("gpiblpt:InvalidFunction","Invalid function.");
  
  mxFree(f);
  
}

void mexAtExitFunction() {
  CloseHandle(driver);
}

void nargchk(int nlhs, int required_nlhs, int nrhs, int required_nrhs) {
  if (nlhs < required_nlhs) mexErrMsgTxt("Not enough output arguments.");
  if (nlhs > required_nlhs) mexErrMsgTxt("Too many output arguments.");
  if (nrhs < required_nrhs) mexErrMsgTxt("Not enough input arguments.");
  if (nrhs > required_nrhs) mexErrMsgTxt("Too many input arguments.");
}

void error(const char *f, uint32_t id) {
  char msgid[32], msg[256], txt[256];
  sprintf(msgid,"gpiblpt:%s:E%d",f,id);
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,id,0,msg,sizeof(msg),NULL);
  sprintf(txt,"Error %d in '%s'.\n%s",id,f,msg);
  mexErrMsgIdAndTxt(msgid,txt);
}

inline void GPIB_readString(uint8_t addr, uint8_t *data, uint32_t max_length, uint8_t useEOI, uint8_t useTerminator, uint8_t terminator, uint32_t *length) {
  
  uint32_t i;
  uint8_t EOI;
  
  GPIB_writeCmd(GPIB_CMD_UNT);
  GPIB_writeCmd(GPIB_CMD_UNL);
  GPIB_writeCmd(GPIB_CMD_LIS);
  GPIB_writeCmd(GPIB_CMD_TAL | (addr & 0x1F));
  
  setModeListener();
  for (i = 0; i < max_length; i++) {
    GPIB_readDataByte(&data[i],&EOI);
    if ((useEOI && EOI) || (useTerminator && (data[i] == terminator))) {
      i++;
      break;
    }
  }
  setModeTalker();
  
  GPIB_writeCmd(GPIB_CMD_UNT);
  GPIB_writeCmd(GPIB_CMD_UNL);
  
  *length = i;
  
}

inline void GPIB_writeString(uint8_t *addr, uint8_t n_addr, uint8_t *data, uint32_t length) {
  
  uint32_t i;
  
  GPIB_writeCmd(GPIB_CMD_UNT);
  GPIB_writeCmd(GPIB_CMD_UNL);
  for (i = 0; i < n_addr; i++) GPIB_writeCmd(GPIB_CMD_LIS | (addr[i] & 0x1F));
  GPIB_writeCmd(GPIB_CMD_TAL);
  
  for (i = 0; i < length; i++) {
    GPIB_writeDataByte(data[i]);
  }
  
  GPIB_writeCmd(GPIB_CMD_UNT);
  GPIB_writeCmd(GPIB_CMD_UNL);
  
}

void GPIB_addressedCommand(uint8_t *addr, uint8_t n_addr, uint8_t cmd) {
  uint8_t i;
  GPIB_writeCmd(GPIB_CMD_UNL);
  for (i = 0; i < n_addr; i++) GPIB_writeCmd(GPIB_CMD_LIS | (addr[i] & 0x1F));
  GPIB_writeCmd(cmd);
  GPIB_writeCmd(GPIB_CMD_UNL);
}

inline void GPIB_deviceClear() {
  GPIB_writeCmd(GPIB_CMD_UNT);
  GPIB_writeCmd(GPIB_CMD_UNL);
  GPIB_writeCmd(GPIB_CMD_DCL);
}

inline void GPIB_readSRQ(uint8_t *SRQ) {
  *SRQ = read_SRQ();
}

inline void GPIB_serialPollEnable() {
  GPIB_writeCmd(GPIB_CMD_UNT);
  GPIB_writeCmd(GPIB_CMD_UNL);
  GPIB_writeCmd(GPIB_CMD_LIS);
  GPIB_writeCmd(GPIB_CMD_SPE);
}

inline uint8_t GPIB_serialPoll(uint8_t addr, uint8_t *data) {
  static uint8_t EOI;
  GPIB_writeCmd(GPIB_CMD_TAL | (addr & 0x1F));
  setModeListener();
  GPIB_readDataByte(data,&EOI);
  setModeTalker();
  return (!!(*data & 0x40));
}

inline void GPIB_serialPollDisable() {
  GPIB_writeCmd(GPIB_CMD_SPD);
  GPIB_writeCmd(GPIB_CMD_UNT);
  GPIB_writeCmd(GPIB_CMD_UNL);
}

void GPIB_readDataByte(uint8_t *data, uint8_t *EOI) {
  /* Set NRFD = 0 */
  write_DAV_NRFD_NDAC_ATN_DOUT(0,0,1,0,0);
  /* Wait for DAV == 1 */
  waitfor_DAV(1);
  /* Read data & EOI */
  *data = read_DIO();
  *EOI = read_EOI();
  /* Set NRFD = 1, NDAC = 0 */
  write_DAV_NRFD_NDAC_ATN_DOUT(0,1,0,0,0);
  /* Wait for DAV == 0 */
  waitfor_DAV(0);
  /* Set NDAC = 1 */
  setModeListener();
}

void GPIB_writeDataByte(uint8_t data) {
  /* Wait for NRFD == 0 */
  waitfor_NRFD(0);
  /* Write data */
  write_DIO(data);
  /* Set DIO in output mode, DAV = 1 */
  write_DAV_NRFD_NDAC_ATN_DOUT(1,0,0,0,1);
  /* Wait for NRFD == 1 */
  waitfor_NRFD(1);
  /* Wait for NDAC == 0 */
  waitfor_NDAC(0);
  /* Set DAV = 0, DIO in input mode */
  setModeTalker();
}

void GPIB_writeCmd(uint8_t cmd) {
  /* Set ATN = 1 */
  write_DAV_NRFD_NDAC_ATN_DOUT(0,0,0,1,0);
  /* Wait for NRFD == 0 */
  waitfor_NRFD(0);
  /* Write command */
  write_DIO(cmd);
  /* Set DIO in output mode, DAV = 1 */
  write_DAV_NRFD_NDAC_ATN_DOUT(1,0,0,1,1);
  /* Wait for NRFD == 1 */
  waitfor_NRFD(1);
  /* Wait for NDAC == 0 */
  waitfor_NDAC(0);
  /* Set ATN = 0, DAV = 0, DIO in input mode */
  setModeTalker();
}

uint8_t READ_PORT_UCHAR(uint32_t port) {
  uint8_t inBuffer[4], data;
  *((uint32_t*)&inBuffer[0]) = port;
  if (!DeviceIoControl(driver,IOCTL_READ_PORT_UCHAR,&inBuffer,4,&data,1,&bytesReturned,NULL)) {
    error(__func__,GetLastError());
  }
  return data;
}

void WRITE_PORT_UCHAR(uint32_t port, uint8_t data) {
  uint8_t inBuffer[5];
  *((uint32_t*)&inBuffer[0]) = port;
  *((uint8_t*)&inBuffer[4]) = data;
  if (!DeviceIoControl(driver,IOCTL_WRITE_PORT_UCHAR,&inBuffer,5,NULL,0,&bytesReturned,NULL)) {
    error(__func__,GetLastError());
  }
}

void waitfor_LPT_STAT(uint8_t mask, uint8_t value, const char *f) {
  
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME,&t0);
  
  while (!(READ_PORT_UCHAR(LPT_STAT) & mask) == value) {
    clock_gettime(CLOCK_REALTIME,&t1);
    if ((t1.tv_sec-t0.tv_sec)*1000+(t1.tv_nsec-t0.tv_nsec)/1000000 > timeout) {
      setModeTalker();
      error(f,ERROR_TIMEOUT);
    }
  }
  
}
