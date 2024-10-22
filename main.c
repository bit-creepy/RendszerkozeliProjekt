#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<libgen.h>
#include<time.h>
#include<stdint.h>
#include<math.h>
#include<dirent.h>
#include<unistd.h>
#include<ctype.h>
#include<signal.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<omp.h>
#include "alprogramok.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    signal(SIGINT, SignalHandler);
    signal(SIGUSR1, SignalHandler);

    int send_flag = 0, receive_flag = 0, file_flag = 0, socket_flag = 0;
    if(strcmp("chart",basename(argv[0])) != 0){
        fprintf(stderr, "Error: Invalid executable name. Executable should be called chart\n");
        exit(9);
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            #pragma omp parallel sections
            {
                #pragma omp section
                {
                    printf("Version: 1.01\n");
                }
                #pragma omp section
                {
                    printf("Made by: Molnár Attila\n");
                }
                #pragma omp section
                {
                    printf("Created on: 5/1/2024");
                }
            }
            
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            outHelp();
            return 0;
        } else if (strcmp(argv[i], "-send") == 0) {
            if (receive_flag || send_flag) {
                fprintf(stderr, "Error: Duplicate argument. -send.\n");
                exit(1);
            }
            send_flag = 1;
        } else if (strcmp(argv[i], "-receive") == 0) {
            if (receive_flag || send_flag) {
                fprintf(stderr, "Error: Duplicate argument. -receive\n");
                exit(1);
            }
            receive_flag = 1;
        } else if (strcmp(argv[i], "-file") == 0) {
            if (file_flag || socket_flag) {
                fprintf(stderr, "Error: Duplicate argument -file.\n");
                exit(1);
            }
            file_flag = 1;
        } else if (strcmp(argv[i], "-socket") == 0) {
            if (socket_flag || file_flag) {
                fprintf(stderr, "Error: Duplicate argument -socket.\n");
                exit(1);
            }
            socket_flag = 1;
        } else {
            outHelp();
            exit(1);
        }
    }
    if(!(receive_flag || send_flag)){
        send_flag = 1;
    }
    if(!(file_flag || socket_flag)){
        file_flag = 1;
    }

    if(receive_flag && file_flag){
        printf("Starting listener...\n");
        while(1){
            fflush(stdout);
            signal(SIGUSR1,ReceiveViaFile);
            pause();
        }
    }
    if(send_flag){
        
        int *Values;
        int N = Measurement(&Values);
        if(file_flag) SendViaFile(Values, N);
        if(socket_flag) SendViaSocket(Values, N);
        free(Values);
    }

    if(receive_flag && socket_flag){
        printf("Starting listener...\n");
        ReceiveViaSocket();
    }
    
    return 0;
}