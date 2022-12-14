/******************************************************************************
 * C++ source of RPX-100-FSK
 *
 * File:   SDR-FSK.h
 * Author: Bernhard Isemann
 *
 * Created on 08 May 2022, 12:37
 * Updated on 08 May 2022, 17:00
 * Version 2.00
 *****************************************************************************/

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>
#include <ctime>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <chrono>
#include <cstring>
#include <bitset>
#include "ini.h"
#include "log.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include "sockets/ServerSocket.h"
#include "sockets/SocketException.h"
#include <iterator>
#include <signal.h>
#include <pthread.h>
#include "Util.h"
#include "WebSocketServer.h"

#pragma once

// external variables
extern pthread_mutex_t SDRmutex;

extern std::stringstream msgSDR;

extern double frequency;
extern double sampleRate;
extern double normalizedGain;
extern string mode;
extern int modeSelector;

lms_device_t *device = NULL;
int SDRinit(double freq, double sampleR, int modeSel, double normGain);
int SDRset(double freq, double sampleR, int modeSel, double normGain);
void *sendBeacon(void *threadID);
int startSDRTXStream(string message);
int error();
void print_gpio(uint8_t gpio_val);


// Radio Frontend - Define GPIO settings for CM4 hat module
uint8_t setRX = 0x01;       // GPIO0=HIGH - RX
uint8_t setTX = 0x02;       // GPIO0=LOW - TX, GPIO1=HIGH - SWITCH Antenna to TX Path
uint8_t setTXPRE = 0x06;    // GPIO0=LOW - TX, GPIO1=HIGH - SWITCH Antenna to TX Path, GPIO2=HIGH - Pre-Amp ON
uint8_t setTXPA = 0x0E;     // GPIO0=LOW - TX, GPIO1=HIGH - SWITCH Antenna to TX Path, GPIO2=HIGH - Pre-Amp ON, GPIO3=HIGH - PA ON

string modeName[9] = {"RX", "TX", "TXPRE", "TXPA"};
uint8_t modeGPIO[9] = {setRX, setTX, setTXPRE, setTXPA};


// Initialize sdr buffers
const int sampleCnt = 1024;               // complex samples per buffer --> a "sample" is I + Q values in float or int
float buffer[sampleCnt * 2];              // buffer to hold samples (each I + Q) --> buffer size = 2 * no of samples
liquid_float_complex c_buffer[sampleCnt]; // complex buffer to hold SDR sample in complex domain
liquid_float_complex complex_i(0, 1);
int samplesRead = 1024;

bool rxON = true;
bool txON = true;

// Beacon frame parameters
float sig[sampleCnt];
float kf = 0.1f;                 // modulation factor
float SNRdB = 30.0f;             // signal-to-noise ratio [dB]
string beaconMessage = "WRAN FSK Beacon at 52.8 MHz";
int tx_time = 20;
float modFactor = 0.4f;
float toneFrequency = 2e3;