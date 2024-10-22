#include "alprogramok.h"

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
#include<sys/stat.h>

#define PORT_NO 3333

#pragma pack(push, 1)
typedef struct {
    uint16_t signature;     //BM
    uint32_t file_size;     //File size
    uint32_t unused;        //0
    uint32_t offset;        //62
} BMPFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct{
    uint32_t header_size;   //40
    uint32_t width;         //DATA_W
    uint32_t height;        //DATA_H
    uint16_t planes;    //1
    uint16_t bpp;       //1
    uint32_t compression;   //0
    uint32_t image_size;     //0
    uint32_t h_resolution;  //3937
    uint32_t v_resolution;  //3937
    uint32_t colors;        //0
    uint32_t important_colors;  //0
} BMPInfoHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} BMPColorData;
#pragma pack(pop)

int ss; //Server Socket

void SignalHandler(int sig){
    if(sig == SIGINT) {
        printf("Shutting down...");
        exit(0);
    }
    if(sig == SIGUSR1){
        fprintf(stderr, "Error: Data transfer through files is unavailable");
    }
    if(sig == SIGALRM){
        fprintf(stderr, "Error: Server timeout");
        exit(3);
    }
}

void outHelp(){
    printf("Usage: chart [service-mode] [transfer-mode]\n");
    printf("Example: chart -send -socket");
    printf("Options:\n");
    printf("//INFORMATION\n");
    printf("  --version : Print version information\n");
    printf("  --help    : Print this help message\n");
    printf("//SERVICE MODE\n");
    printf("  -send     : Send mode\n");
    printf("  -receive  : Receive mode\n");
    printf("//TRANSFER MODE\n");
    printf("  -file     : File transfer\n");
    printf("  -socket   : Socket transfer\n");
}

int32_t swapEndian(int32_t input){
    return ((input>>24)&0xff) | ((input<<8)&0xff0000) | ((input>>8)&0xff00) | ((input<<24)&0xff000000);
}

int FindPID(){
    DIR *d;
    struct dirent *entry;
    d=opendir("/proc");

    char proc_path[256];

    while((entry=readdir(d))!=NULL){
        if(!isdigit((*entry).d_name[0])) continue;
        snprintf(proc_path, sizeof(proc_path), "/proc/%s/status",(*entry).d_name);
        FILE* status_file = fopen(proc_path, "r");
        if(status_file == NULL){
            fprintf(stderr, "Error: Could not open status file\n");
            exit(2);
        }
        char process_name[256];
        fscanf(status_file,"Name:\t%s\n", process_name);
        if(strcmp(process_name,"chart") != 0) continue;
        char line[256];
        while(fgets(line, sizeof(line),status_file)){
            int pid;

            if(strncmp(line, "Pid:",4)!=0) continue;
            if(sscanf(line, "Pid:\t%d",&pid) != 1) continue;
            if(pid == getpid()) continue;

            fclose(status_file);
            closedir(d);
            return pid;          
        }
        fclose(status_file);
    }
    closedir(d);
    return -1;
}

void BMPcreator(int *Values, int NumValues){
    int matrix[NumValues][NumValues];
    int center = NumValues/2+1;

    //Initialize and fill array used for display
    for(int x = 0; x < NumValues; x++){
        for(int y = 0; y < NumValues; y++){
            matrix[x][y] = 0;
        }
        int displacement = Values[x];
        int position = displacement+center;
        if(position>=NumValues){
            position = NumValues-1;
        }
        else if(position < 0){
            position = 0;
        }
        matrix[x][position] = 1;
    }

    FILE *bmp_file = fopen("chart.bmp", "wb");
    if(!bmp_file) {
        fprintf(stderr, "Error: Failed to create BMP file. \n");
        exit(2);
    }
    BMPFileHeader file_header = {0x4D42, 0, 0, sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + 2 * sizeof(BMPColorData)};
    file_header.file_size = file_header.offset + 4 * NumValues;
    fwrite(&file_header, sizeof(BMPFileHeader), 1, bmp_file);
    BMPInfoHeader info_header = {40, NumValues, NumValues, 1, 1, 0, 0, 3937, 3937,0,0};
    fwrite(&info_header, sizeof(BMPInfoHeader), 1, bmp_file);
    BMPColorData color_0 = {255, 255, 255, 255};
    BMPColorData color_1 = {0, 0, 255, 255};
    fwrite(&color_0, sizeof(BMPColorData), 1, bmp_file);
    fwrite(&color_1, sizeof(BMPColorData), 1, bmp_file);

    for (int y = 0; y < NumValues; y++) {
        uint32_t outByte = 0;
        for (int x = 0; x < NumValues; x++) {
            uint32_t toAdd = matrix[x][y] << (31 - (x % 32));
            outByte = outByte | toAdd;
            if ((x % 32 == 31) || x == NumValues - 1) {
                outByte = swapEndian(outByte);
                fwrite(&outByte, sizeof(uint32_t), 1, bmp_file);
                outByte = 0;
            }
        }
    }

    fclose(bmp_file);

    if (chmod("chart.bmp", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
        perror("chmod");
        fprintf(stderr, "Error: Failed to set file permissions. \n");
        exit(3);
    }
}

int Measurement(int **Values) {
    int current_time = time(NULL);
    int seconds = current_time % 900;
    int N = seconds > 100 ? seconds : 100;

    *Values = (int *)malloc(N * sizeof(int));
    
    if (*Values == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(4);
    }

    int x = 0;
    (*Values)[0] = x;
    for (int i = 1; i < N; i++) {
        int rand_num = rand() % 1000000;
        if (rand_num < 428571) {
            x++;
        } else if (rand_num > 645161 ) {
            x--;
        }
        (*Values)[i] = x;
    }

    return N;
}

void SendViaFile(int *Values, int NumValues){
    char *home_dir = getenv("HOME");
    char filePath[256];
    sprintf(filePath, "%s/Measurement.txt",home_dir);

    FILE *measurement_file = fopen(filePath, "w");
    if(measurement_file == NULL){
        fprintf(stderr, "Error: Could not open output file\n");
        exit(2);
    }
    for(int i = 0; i < NumValues; i++){
        fprintf(measurement_file, "%d\n",Values[i]);
    }

    fclose(measurement_file);
    int pid = FindPID();
    if(pid == -1){
        fprintf(stderr, "Error: No receiver found running on your machine\n");
        exit(5);
    }

    kill(pid, SIGUSR1);
}

void ReceiveViaFile(int sig){
    if(sig != SIGUSR1) return;
    printf("Working...\n");


    int capacity = 10;
    int *values = malloc(capacity*sizeof(int));
    if(values == NULL){
        fprintf(stderr, "Error: Failed to allocate memory\n");
        exit(4);
    }

    int count = 0;
    int measurement;
    char *home_dir = getenv("HOME");
    char filePath[256];
    sprintf(filePath, "%s/Measurement.txt",home_dir);

    FILE *measurement_file = fopen(filePath, "r");
    while(fscanf(measurement_file, "%d", &measurement) == 1){
        if(count == capacity){
            capacity *= 2;
            int *temp = realloc(values, capacity*sizeof(int));
            if(temp == NULL){
                fprintf(stderr, "Error: Failed to allocate memory\n");
                fclose(measurement_file);
                free(values);
                exit(4);
            }
            values = temp;
        }
        values[count++] = measurement;
    }
    
    fclose(measurement_file);

    BMPcreator(values,count);
    free(values);
    printf("BMP File created!\n");
}

void SendViaSocket(int *Values, int NumValues){
    int bytes;
    int flag = 0;
    char on = 1;
    unsigned int server_size;
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT_NO);
    server_size = sizeof server;

    int s = socket(AF_INET, SOCK_DGRAM, 0);

    if(s < 0){
        fprintf(stderr, "Error: Socket creation error\n");
        exit(3);
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on);

    bytes = sendto(s, &NumValues, sizeof(int), flag, (struct sockaddr *) & server, server_size);
    if(bytes <= 0) {
        fprintf(stderr, "Error: Sending error\n");
        exit(3);
    }
    printf("%i bytes have been sent to server\n",bytes-1);

    signal(SIGALRM, SignalHandler);

    int received_num;
    alarm(1);
    bytes = recvfrom(s, &received_num, sizeof(int), flag, (struct sockaddr *) & server, &server_size);
    signal(SIGALRM, SIG_DFL);
    if(bytes < 0){
        fprintf(stderr, "Error: Receive error\n");
        exit(3);
    }
    if(received_num != NumValues){
        fprintf(stderr, "Error: Server responed with invalid ACK\n");
        exit(3);
    }
    printf("Server responded with valid ACK\n");
    bytes = sendto(s, Values, NumValues*sizeof(int), flag, (struct sockaddr *) & server, server_size);
    if(bytes <= 0) {
        fprintf(stderr, "Error: Sending error\n");
        exit(3);
    }
    printf("%i bytes have been sent to server\n",bytes-1);
    bytes = recvfrom(s, &received_num, sizeof(int), flag, (struct sockaddr *) & server, &server_size);
    if(bytes < 0){
        fprintf(stderr, "Error: Receive error\n");
        exit(3);
    }
    int size = received_num;
    if(size != sizeof(Values)){
        fprintf(stderr, "Error: Server responed with invalid ACK\n");
        exit(3);
    }
    else{
        printf("Server responded with valid ACK");
    }
    close(s);
}

void StopSocket(int sig){
    close(ss);
    printf("Shutting down...");
    exit(0);
}

void ReceiveViaSocket(){
    
    int bytes;
    int err;
    int flag = 0;
    char on = 1;
    unsigned int server_size;
    unsigned int client_size;
    struct sockaddr_in server;
    struct sockaddr_in client;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT_NO);
    server_size = sizeof server;
    client_size = sizeof client;

    signal(SIGINT, StopSocket);
    signal(SIGTERM, StopSocket);

    ss = socket(AF_INET, SOCK_DGRAM, 0);

    if(ss < 0){
        fprintf(stderr, "Error: Socket creation error\n");
        exit(3);
    }

    setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    setsockopt(ss, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on);

    err = bind(ss, (struct sockaddr *) & server, server_size);
    if(err<0){
        fprintf(stderr, "Error: Binding error\n");
        exit(3);
    }

    while(1){
        printf("Listening...");
        int NumValues;
        bytes = recvfrom(ss, &NumValues, sizeof(int), flag, (struct sockaddr*) &client, &client_size);
        if(bytes < 0){
            fprintf(stderr, "Error: Receive error\n");
            exit(3);
        }
        printf(" %d bytes received from client, message: %d\n", bytes-1, NumValues);
        printf("Remote peer: %s:%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
        bytes = sendto(ss, &NumValues, sizeof(int), flag, (struct sockaddr*) &client, client_size);
        if(bytes <=0){
            fprintf(stderr, "Error: Sending error\n");
            exit(3);
        }
        printf("ACK sent\n");
        int *Values = (int*)malloc(NumValues*sizeof(int));
        bytes = recvfrom(ss, Values, NumValues*sizeof(int), flag, (struct sockaddr*) &client, &client_size);
        printf(" %d bytes received from client\n", bytes-1);
        if(bytes < 0){
            free(Values);
            fprintf(stderr, "Error: Receive error\n");
            exit(3);
        }
        int size = sizeof(Values);
        bytes = sendto(ss, &size, sizeof(int), flag, (struct sockaddr*) &client, client_size);
        if(bytes <=0){
            free(Values);
            perror("sendto");
            fprintf(stderr, "Error: Sending error");
            exit(3);
        }
        printf("ACK sent\n");
        BMPcreator(Values, NumValues);
        free(Values);
    }
}