#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "TGraph.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TAxis.h"
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>


//network setting
#define SERVPORT 24
#define UDP_port 4660

extern int setup(int BoardNum, 
                    const char *trigger, 
                    const char *threshold_upp, 
                    const char *threshold_low, 
                    bool zero_supp_on, 
                    bool decision_on);

