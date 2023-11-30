#include "setup.h"

int test;
int IP_array[10];

int rbcp(const char* sitcpIpAddr, unsigned int sitcpPort, const char* szVerb, const char* szArg1, const char* szArg2, unsigned char* parameter);


int setup(int BoardNum, 
            const char *trigger, 
            const char *threshold_upp, 
            const char *threshold_low, 
            bool zero_supp_on, 
            bool decision_on){

    int err_board=0;
    int BoardMaxCount = 1;
    unsigned char parameter[256];


//RESET-------------------------------
    for(int boardnum_i = 1; boardnum_i <= BoardNum; boardnum_i++){
        //IP address
        char IP_address_char[256];
        sprintf(IP_address_char, "192.168.10.%d", IP_array[boardnum_i - 1]);

        if(rbcp(IP_address_char,  UDP_port, "wrb", "0x16a"      ,  "0x00", parameter) == -1){err_board=0; return -1;} sleep(1);
        if(rbcp(IP_address_char,  UDP_port, "wrb", "0x11d"      ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char,  UDP_port, "wrb", "0x110"      ,  "0xf0", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char,  UDP_port, "wrb", "0x112"      ,  "0x01", parameter) == -1){err_board=0; return -1;} usleep(50000);
    }

    sleep(1);


//setup------------------------------

    //trigger
    char trigger_send[256];
    if(strcmp(trigger, "off") == 0)sprintf(trigger_send, "0x00");
    else if(strcmp(trigger, "base") == 0)sprintf(trigger_send, "0x02");
    else if(strcmp(trigger, "self") == 0)sprintf(trigger_send, "0x04");
    else if(strcmp(trigger, "normal") == 0)sprintf(trigger_send, "0x08");
    else sprintf(trigger_send, "0x08");


    //ASD threshold
    char threshold_upp_send[256];
    char threshold_low_send[256];
    sprintf(threshold_upp_send, "0x%s", threshold_upp);
    sprintf(threshold_low_send, "0x%s", threshold_low);


    //zero suppress
    char zero_supp_on_send[256];
    if(zero_supp_on){
        sprintf(zero_supp_on_send, "0x01");
    }
    else{
        sprintf(zero_supp_on_send, "0x00");
    }

   
    //Decision Trigger
    char decision_on_send[256];
    if(decision_on){
        sprintf(decision_on_send, "0x01");
    }
    else{
        sprintf(decision_on_send, "0x00");
    }

    char IPaddress_send[256];
    char MAC_address0[256];
    char MAC_address1[256];
    char MAC_address2[256];
    char MAC_address3[256];
    char MAC_address4[256];
    char MAC_address5[256];


    for(int boardnum_i = 1; boardnum_i <= BoardNum; boardnum_i++){

        //IP address
        char IP_address_char[256];
        sprintf(IP_address_char, "192.168.10.%d", IP_array[boardnum_i - 1]);


        //daisy on 
        char daisy_on_send[256];
        if(boardnum_i != BoardNum){
            sprintf(daisy_on_send, "0x01");
        }
        else{
            sprintf(daisy_on_send, "0x00");
        }


        int nextboard_IP = 0;
        if(boardnum_i != BoardNum){


            nextboard_IP = IP_array[boardnum_i];

            //next board IP address
            sprintf(IPaddress_send, "%d", nextboard_IP);

            //next board MAC address;
            sprintf(MAC_address0, "0x7C");
            sprintf(MAC_address1, "0xF0");
            sprintf(MAC_address2, "0x98");
            sprintf(MAC_address3, "0x01");
            sprintf(MAC_address4, "0x16");
                
            sprintf(MAC_address5, "0x%x", nextboard_IP + 128 - 16) ;
            /* 
            if(nextboard_IP >= 16 && nextboard_IP <= 23){
                sprintf(MAC_address5, "0x%x", nextboard_IP + 16);
            }
            else if(nextboard_IP >= 24 && nextboard_IP <= 30){
                sprintf(MAC_address5, "0x%x", nextboard_IP + 96);
            }
            else{
                printf("IP address %d is strage\n", nextboard_IP);
                exit(1);
            }
            */
            printf("TEST IP = %d,  MAC = %s\n", nextboard_IP, MAC_address5);
        }

        //data send
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x110"      ,  "0xf0", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x170"      ,  "30",   parameter) == -1){err_board=0; return -1;} usleep(50000);
        //if(rbcp(IP_address_char, UDP_port, "wrb", "0x170"      ,  "62",   parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00030000" ,  "0x08", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00030002" ,  "0x06", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00010000" ,  "0x3C", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00020000" ,  "0x3C", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00010008" ,  "0x03", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00010008" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00020008" ,  "0x03", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00020008" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00010014" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00020014" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x0001000D" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x0002000D" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x115"      ,  threshold_upp, parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x116"      ,  threshold_low, parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x11d"      ,  trigger_send, parameter) == -1){err_board=0; return -1;} usleep(50000);
        //if(rbcp(IP_address_char, UDP_port, "wrb", "0x141"      ,  "60", parameter) == -1){err_board=0; return -1;} usleep(50000);
        //if(rbcp(IP_address_char, UDP_port, "wrb", "0x142"      ,  "70", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x141"      ,  "180", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x142"      ,  "210", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x140"      ,  zero_supp_on_send, parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x171"      ,  decision_on_send, parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x119"      ,  "0x01", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x111"      ,  "0xf0", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x110"      ,  "0xf0", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x11A"      ,  "0x2F", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x112"      ,  "0x01", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00010016" ,  "0x04", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x00020016" ,  "0x04", parameter) == -1){err_board=0; return -1;} usleep(50000);
        //if(rbcp(IP_address_char, UDP_port, "wrb", "0x00010016" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        //if(rbcp(IP_address_char, UDP_port, "wrb", "0x00020016" ,  "0x00", parameter) == -1){err_board=0; return -1;} usleep(50000);
        if(rbcp(IP_address_char, UDP_port, "wrb", "0x153"      ,  daisy_on_send, parameter) == -1){err_board=0; return -1;} usleep(50000);

        //TCP connect setup
        if(boardnum_i != BoardNum){
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x163"      ,  "192",  parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x162"      ,  "168",  parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x161"      ,  "10",   parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x160"      ,  IPaddress_send,  parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x169"      ,  MAC_address0, parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x168"      ,  MAC_address1, parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x167"      ,  MAC_address2, parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x166"      ,  MAC_address3, parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x165"      ,  MAC_address4, parameter) == -1){err_board=0; return -1;} usleep(50000);
            if(rbcp(IP_address_char, UDP_port, "wrb", "0x164"      ,  MAC_address5, parameter) == -1){err_board=0; return -1;} usleep(50000);

        }
    }


    //TCP connect 
    for(int boardnum_i = BoardNum; boardnum_i > 1; boardnum_i--){

        //IP address
        char IP_address_char[256];
        sprintf(IP_address_char, "192.168.10.%d", IP_array[boardnum_i - 2]);

        if(rbcp(IP_address_char, UDP_port, "wrb", "0x16a"     ,  "0x01", parameter) == -1){err_board=1; return -1;} usleep(50000);

        //TCP connect check
        if(rbcp(IP_address_char, UDP_port, "rd", "0x16D"      ,  "0x01", parameter) == -1){
            fprintf(stderr, "Network Error: TCP connect Err\n");
            exit(3);
        }

        if(parameter[0] != 1){
            fprintf(stderr, "TCP connect Err\n");
            exit(3);
        }
    }

    usleep(50000);

    return 0;
}
