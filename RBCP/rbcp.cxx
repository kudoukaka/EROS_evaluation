#define MAX_LINE_LENGTH 1024
#define MAX_PARAM_LENGTH 20
#define RBCP_VER 0xFF
#define RBCP_CMD_WR 0x80
#define RBCP_CMD_RD 0xC0
#define DEFAULT_IP 192.168.0.16
#define UDP_BUF_SIZE 2048

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "rbcp.h"
#include "rbcp_com.cxx"
#include "myAtoi.cxx"
#include "myScanf.cxx"

int rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sndHeader, const char* sndData, char* readData, char dispMode, unsigned char* parameter);

int DispatchCommand(const char* pszVerb,
		    const char* pszArg1,
		    const char* pszArg2,
		    const char* ipAddr,
		    unsigned int rbcpPort,
		    struct rbcp_header* sndHeader,
		    char dispMode,
            unsigned char* parameter
		    );

int myScanf(char* inBuf, char* argBuf1, char* argBuf2,  char* argBuf3);
int myGetArg(char* inBuf, int i, char* argBuf);

int rbcp(const char* sitcpIpAddr, unsigned int sitcpPort, const char* szVerb, const char* szArg1, const char* szArg2, unsigned char* parameter){
  struct rbcp_header sndHeader;

  sndHeader.type=RBCP_VER;
  sndHeader.id=0;

  return DispatchCommand(szVerb, szArg1, szArg2, sitcpIpAddr, sitcpPort, &sndHeader,1,parameter);
}


int DispatchCommand(const char* pszVerb,
		    const char* pszArg1,
		    const char* pszArg2,
		    const char* ipAddr,
		    unsigned int rbcpPort,
		    struct rbcp_header* sndHeader,
		    char dispMode,
            unsigned char* parameter
		    ){
  //  char sendData[UDP_BUF_SIZE];
  char recvData[UDP_BUF_SIZE];

  unsigned int tempInt;

  if(strcmp(pszVerb, "wrb") == 0){
    tempInt = myAtoi(pszArg2);    

    char pszArg2_2[20];
    pszArg2_2[0] = (char)(0xFF & tempInt);

    sndHeader->command= RBCP_CMD_WR;
    sndHeader->length=1;
    sndHeader->address=htonl(myAtoi(pszArg1));
    return rbcp_com(ipAddr, rbcpPort, sndHeader, pszArg2_2,recvData,dispMode,parameter);
  }
  else if(strcmp(pszVerb, "rd") == 0){
    sndHeader->command= RBCP_CMD_RD;
    sndHeader->length=myAtoi(pszArg2);
    sndHeader->address=htonl(myAtoi(pszArg1));
    
    return rbcp_com(ipAddr, rbcpPort, sndHeader, pszArg2,recvData,dispMode, parameter);
  }
  return 0;
}

