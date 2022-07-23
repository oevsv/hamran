/******************************************************************************
 * C++ source of RPX-100-TX
 *
 * File:   RPX-100-TX.h
 * Author: Bernhard Isemann
 *         Marek Honek
 *
 * Created on 19 Apr 2022, 18:20
 * Updated on 22 May 2022, 18:00
 * Version 1.00
 * Predecessor RPX-100-Beacon-reorganized.h
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sstream>
#include <syslog.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <chrono>
#include <cstring>
#include <bitset>
#include "stuff/ini.h"
#include "stuff/log.h"
#include <chrono>
// #include "alsa/asoundlib.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include "stuff/ServerSocket.h"
#include "stuff/SocketException.h"
#include <iterator>
#include <signal.h>
// #include <pthread.h>
#include "stuff/Util.h"
#include "stuff/WebSocketServer.h"
#include <correct.h>
#pragma once


lms_device_t *device = NULL;


pthread_mutex_t SDRmutex;
#define NUM_THREADS 5 // max number of main threads
#define NUM_CONNECTS 5 // max number of sockets connections
#define PORT_NUMBER 8084
#define TIMEOUT 500
#define SUBCARRIERS 1024
#define DATACARRIERS 480
#define DEFAULT_PHY_MODE 1
#define DEFAULT_CYCL_PREFIX 4

#define TX_6m_MODE 6
#define RX_MODE 0

// print each step for debuggigng
#define PRINT false

string message = "OE1XTU WRAN at 52.8 MHz";

float calculateBER(unsigned int payload_len, string transmitted, unsigned char *received);
bool callback_invoked = false;

// Radio Frontend - Define GPIO settings for CM4 hat module
uint8_t setRX = 0x18;       // GPIO0=LOW - RX, GPIO3=HIGH - PTT off,
uint8_t setTXDirect = 0x0F; // GPIO0=HIGH - TX, GPIO3=HIGH - PTT off, GPIO1=HIGH, GPIO2=HIGH
uint8_t setTX6m = 0x0D;     // GPIO0=HIGH - TX, GPIO3=HIGH - PTT off, GPIO1=LOW, GPIO2=LOW
uint8_t setTX2m = 0x09;     // GPIO0=HIGH - TX, GPIO3=HIGH - PTT off, GPIO1=LOW, GPIO2=HIGH
uint8_t setTX70cm = 0x0B;   // GPIO0=HIGH - TX, GPIO3=HIGH - PTT off, GPIO1=HIGH, GPIO2=LOW

uint8_t setTXDirectPTT = 0x07; // GPIO0=HIGH - TX, GPIO3=LOW - PTT on, GPIO1=HIGH, GPIO2=HIGH
uint8_t setTX6mPTT = 0x05;     // GPIO0=HIGH - TX, GPIO3=LOW - PTT on, GPIO1=LOW, GPIO2=LOW
uint8_t setTX2mPTT = 0x01;     // GPIO0=HIGH - TX, GPIO3=LOW - PTT on, GPIO1=LOW, GPIO2=HIGH
uint8_t setTX70cmPTT = 0x03;   // GPIO0=HIGH - TX, GPIO3=LOW - PTT on, GPIO1=HIGH, GPIO2=LOW

string modeName[9] = {"RX", "TXDirect", "TX6m", "TX2m", "TX70cm", "TXDirectPTT", "TX6mPTT", "TX2mPTT", "TX70cmPTT"};
uint8_t modeGPIO[9] = {setRX, setTXDirect, setTX6m, setTX2m, setTX70cm, setTXDirectPTT, setTX6mPTT, setTX2mPTT, setTX70cmPTT};


void sendFrame(int cyclic_prefix, int phy_mode);
void frameReception(int cyclic_prefix);

int error();
string exec(string command);

// Log facility
void print_gpio(uint8_t gpio_val);
stringstream msgSDR;
stringstream HEXmsg;

// SDR values
double sampleRate = 3328000; //default
string mode = "TX6m";
double normalizedGain = 1;
double frequency = 52.8e6;


// Initialize sdr buffers
liquid_float_complex complex_i(0,1);
int samplesRead = 1048;

bool rxON = true;
bool txON = true;

int startSDRTXStream(int *tx_buffer, int FrameSampleCnt);
void frameAssemble(int *r_frame_buffer, int cyclic_prefix, int phy_mode);
void subcarrierAllocation (unsigned char *array);
ofdmflexframegen DefineFrameGenerator (int dfg_cycl_pref, int dfg_PHYmode);
int frameSymbols(int cyclic_prefix);
uint complexFrameBufferLength(int cyclic_prefix);
uint complexSymbolBufferLength(int cyclic_prefix);
uint payloadLength(int cyclic_prefix, int phy_mode);
void setSampleRate(int cyclic_prefix);

int SDRset(double frequency, int modeSelector, double normalizedGain);
int SDRinit(int modeSelector);
int startSDRTXStream(int *tx_buffer, int FrameSampleCnt);



int callbackWhatsReceived(unsigned char *_header,
               int _header_valid,
               unsigned char *_payload,
               unsigned int _payload_len,
               int _payload_valid,
               framesyncstats_s _stats,
               void *_userdata);

