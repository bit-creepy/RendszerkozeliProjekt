#include<stdint.h>

#ifndef ALPROGRAMOK_H_
#define ALPROGRAMOK_H_

void SignalHandler(int sig);

void outHelp();

int32_t swapEndian(int32_t input);

int FindPID();

void BMPcreator(int *Values, int NumValues);

int Measurement(int **Values);

void SendViaFile(int *Values, int NumValues);

void ReceiveViaFile(int sig);

void SendViaSocket(int *Values, int NumValues);

void StopSocket(int sig);

void ReceiveViaSocket();

#endif