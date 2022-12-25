/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   SDRsamples.h
 * Author: Bernhard Isemann
 *
 * Created on 06 Jan 2022, 12:37
 * Updated on 25 Dec 2022, 15:55
 * Version 1.00
 *****************************************************************************/

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <syslog.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <complex>
#include <ctime>
#include <chrono>
#include <cstring>
#include <bitset>
#include "ini.h"
#include "log.h"
#include "lime/LimeSuite.h"
#include <chrono>
#include "liquid/liquid.h"
#include "sockets/ServerSocket.h"
#include "sockets/SocketException.h"
#include <iterator>
#include <csignal>
#include <pthread.h>
#include "Util.h"
#include "WebSocketServer.h"


#pragma onces

#define NUM_CONNECTS 5 // max number of sockets connections
#define PORT_NUMBER 8085
#define TIMEOUT 500

extern pthread_mutex_t SDRmutex;

// SDR facility
lms_device_t *device = nullptr;
int SDRinit(double frequency, double sampleRate, int modeSelector, double normalizedGain);
int SDRfrequency(lms_device_t *device, double RXfreq, double TXfreq);
int SDRsampleRate(lms_device_t *device, double sampleR);
string exec(string command);
void *startSDRStream(void *threadID);
void *startWebsocketServer(void *threadID);
int error();
extern double frequency;
extern double sampleRate;
extern int modeSelector;
extern double normalizedGain;
double rxFreq = 52e6;
double txFreq = 52e6;
double span = 2e6;


// Radio Frontend - Define GPIO settings for CM4 hat module
uint8_t setRX = 0x00;       // GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
uint8_t setTXDirect = 0x0F; // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=HIGH - no Bandfilter
uint8_t setTX6m = 0x03;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
uint8_t setTX2m = 0x07;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=LOW - 144Mhz Bandfilter
uint8_t setTX70cm = 0x0B;   // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=HIGH - 433Mhz Bandfilter

string modeName[9] = {"RX", "TXDirect", "TX6m", "TX2m", "TX70cm"};
uint8_t modeGPIO[9] = {setRX, setTXDirect, setTX6m, setTX2m, setTX70cm};

// Log facility
void print_gpio(uint8_t gpio_val);
std::stringstream msgSDR;
std::stringstream HEXmsgSDR;


// Initialize sdr buffers
const int sampleCnt = 1048;  // complex samples per buffer --> a "sample" is I + Q values in float or int
float buffer[sampleCnt * 2]; // buffer to hold samples (each I + Q) --> buffer size = 2 * no of samples
liquid_float_complex c_buffer[sampleCnt]; // complex buffer to hold SDR sample in complex domain
liquid_float_complex complex_i(0,1);
int samplesRead = 1048;

// Initialize buffer for spectogram
const int nfft = 512;
liquid_float_complex c_sp_buf[sampleCnt]; // complex buffer to hold spectogram data result
float sp_psd[nfft];
int colormap = 3;
double step = sampleRate / (nfft);

bool rxON = true;

// Socket Server facility
int ConCurSocket;
bool socketsON = true;

// For any real project this should be defined separately in a header file
class rpxServer : public WebSocketServer
{
public: 
    rpxServer( int port );
    ~rpxServer( );
    virtual void onConnect(    int socketID                        );
    virtual void onMessage(    int socketID, const string& data    );
    virtual void onDisconnect( int socketID                        );
    virtual void   onError(    int socketID, const string& message );
};
