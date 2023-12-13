/*************************************************
*                                                *
* SiTCP Remote Bus Control Protocol              *
* Header file                                    *
*                                                *
* 2010/05/31 Tomohisa Uchida                     *
*                                                *
*************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

struct rbcp_header{
  unsigned char type;
  unsigned char command;
  unsigned char id;
  unsigned char length;
  unsigned int address;
};

