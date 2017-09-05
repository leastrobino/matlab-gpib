/*
 *  manage_driver.c
 *
 *  Created by Léa Strobino.
 *  Copyright 2017 hepia. All rights reserved.
 *
 */

#include <mex.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#define DRIVER_ID "WinRing0_1_2_0"
#ifdef _WIN64
#define DRIVER_FILE "WinRing0_64.sys"
#else
#define DRIVER_FILE "WinRing0_32.sys"
#endif

SC_HANDLE manager, service;

void usage() {
  mexErrMsgTxt("Usage: manage_driver install\n    or manage_driver uninstall");
}

void error(const char *f, uint32_t id) {
  
  char msg[256], txt[256];
  
  CloseServiceHandle(service);
  CloseServiceHandle(manager);
  
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,id,0,msg,sizeof(msg),NULL);
  sprintf(txt,"Error %d in '%s'.\n%s",id,f,msg);
  mexErrMsgTxt(txt);
  
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  
  uint32_t e;
  char buffer[MAX_PATH], driverPath[MAX_PATH];
  
  if ((nrhs < 1) || !mxIsChar(prhs[0])) usage();
  char *f = mxArrayToString(prhs[0]);
  
  GetSystemDirectory(buffer,MAX_PATH);
  sprintf(driverPath,"%s\\drivers\\WinRing0.sys",buffer);
  
  manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
  if (!manager) {
    error("OpenSCManager",GetLastError());
  }
  
  if (strcmp(f,"install") == 0) {
    
    sprintf(buffer,"copyfile('"DRIVER_FILE"','%s')",driverPath);
    if (mexEvalString(buffer) == 0) {
      
      service = CreateService(manager,DRIVER_ID,DRIVER_ID,SERVICE_ALL_ACCESS,
              SERVICE_KERNEL_DRIVER,SERVICE_AUTO_START,SERVICE_ERROR_NORMAL,
              driverPath,NULL,NULL,NULL,NULL,NULL);
      if (service) CloseServiceHandle(service);
      else if (e = GetLastError() != ERROR_SERVICE_EXISTS) error("CreateService",e);
      
      service = OpenService(manager,DRIVER_ID,SERVICE_ALL_ACCESS);
      if (!service) error("OpenService",GetLastError());
      
      if (!StartService(service,0,NULL) && (e = GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)) error("StartService",e);
      
      CloseServiceHandle(service);
      
      mexPrintf("Created service '"DRIVER_ID"' (%s).\n",driverPath);
      
    }
    
  } else if (strcmp(f,"uninstall") == 0) {
    
    service = OpenService(manager,DRIVER_ID,SERVICE_ALL_ACCESS);
    if (!service) error("OpenService",GetLastError());
    
    if (!DeleteService(service)) error("DeleteService",e);
    
    sprintf(buffer,"delete('%s')",driverPath);
    if (mexEvalString(buffer) == 0) mexPrintf("Deleted service '"DRIVER_ID"' (%s).\n",driverPath);
    
  } else usage();
  
  CloseServiceHandle(manager);
  
  mxFree(f);
  
}
