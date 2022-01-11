/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   SDRsamples.cpp
 * Author: Bernhard Isemann
 *
 * Created on 06 Jan 2022, 10:35
 * Updated on 06 Jan 2022, 18:20
 * Version 1.00
 *****************************************************************************/

#include "SDRsamples.h"
#include <pthread.h>

using namespace std;

int SDRinit(double frequency, double sampleRate, int modeSelector, double normalizedGain)
{
    // Find devices
    int n;
    lms_info_str_t list[8]; // should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0)
    {
        error(); // NULL can be passed to only get number of devices
    }
    msgSDR.str("");
    msgSDR << "Number of devices found: " << n;
    Logger(msgSDR.str()); // print number of devices
    if (n < 1)
    {
        return -1;
    }

    // open the first device
    if (LMS_Open(&device, list[0], NULL))
    {
        error();
    }
    sleep(1);

    // Initialize device with default configuration
    if (LMS_Init(device) != 0)
    {
        error();
    }
    sleep(1);

    // Set SDR GPIO diretion GPIO0-5 to output and GPIO6-7 to input
    uint8_t gpio_dir = 0xFF;
    if (LMS_GPIODirWrite(device, &gpio_dir, 1) != 0)
    {
        error();
    }

    // Read and log GPIO direction settings
    uint8_t gpio_val = 0;
    if (LMS_GPIODirRead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Set GPIOs direction to output.\n";
    Logger(msgSDR.str());

    // Set GPIOs to RX mode (initial settings)
    if (LMS_GPIOWrite(device, &modeGPIO[modeSelector], 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "GPIO Output to High Level:\n";
    print_gpio(gpio_val);
    Logger(msgSDR.str());

    msgSDR.str("");
    msgSDR << "LimeRFE set to " << modeName[modeSelector] << endl;
    Logger(msgSDR.str());

    // Enable TX channel,Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0)
    {
        error();
    }

    // Set sample rate
    if (LMS_SetSampleRate(device, sampleRate, 0) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Sample rate: " << sampleRate / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // Set center frequency
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, frequency) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Center frequency: " << frequency / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // select Low TX path for LimeSDR mini --> TX port 2 (misslabed in MINI, correct in USB)
    if (LMS_SetAntenna(device, LMS_CH_TX, 0, LMS_PATH_TX2) != 0)
    {
        error();
    }

    // set TX gain
    if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, normalizedGain) != 0)
    {
        error();
    }

    // calibrate Tx, continue on failure
    LMS_Calibrate(device, LMS_CH_TX, 0, sampleRate, 0);

    // Wait 12sec and send status LoRa message
    sleep(2);

    return 0;
}

int SDRfrequency(lms_device_t *device, double frequency)
{
    // Set center frequency
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, frequency) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Center frequency: " << frequency / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    return 0;
}

int error()
{
    msgSDR.str("");
    msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
    Logger(msgSDR.str());
    if (device != NULL)
        LMS_Close(device);
    return -1;
}

void print_gpio(uint8_t gpio_val)
{
    for (int i = 0; i < 8; i++)
    {
        bool set = gpio_val & (0x01 << i);
        msgSDR << "GPIO" << i << ": " << (set ? "High" : "Low") << std::endl;
    }
}

void *startSocketServer(void *threadID)
{
    ServerSocket RPX_server(RPX_port);
    msgSDR.str("");
    msgSDR << "Socket server started as thread no: " << threadID << " using port: " << RPX_port << ", rxON=" << rxON;
    Logger(msgSDR.str());
    pthread_t connects[NUM_CONNECTS];
    ConCurSocket = 0;
    while (socketsON)
    {
        // ServerSocket RPX_socket[NUM_CONNECTS];
        if (ConCurSocket < NUM_CONNECTS)
        {
            RPX_server.accept(RPX_socket[ConCurSocket]);
            // Start thread for SocketServer
            if (pthread_create(&connects[ConCurSocket], NULL, startSocketConnect, (void *)ConCurSocket) != 0)
            {
                msgSDR.str("");
                msgSDR << "ERROR starting thread " << ConCurSocket;
                Logger(msgSDR.str());
            }
            ConCurSocket++;
        }
    }
    pthread_exit(NULL);
}

void *startWebSocket(void *threadID)
{
    msgSDR.str("");
    msgSDR << "WebSockets started as thread no: " << threadID << " using port " << WEBSOCKET_port;
    Logger(msgSDR.str());
    EchoServer es = EchoServer(WEBSOCKET_port);
    es.run();
    pthread_exit(NULL);
}

void *startSDRStream(void *threadID)
{
    // Initialize stream
    lms_stream_t streamId;                        // stream structure
    streamId.channel = 0;                         // channel number
    streamId.fifoSize = 1024 * 1024;              // fifo size in samples
    streamId.throughputVsLatency = 1.0;           // optimize for max throughput
    streamId.isTx = false;                        // RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32; // 12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    // Start streaming
    LMS_StartStream(&streamId);

    // Streaming
    auto t1 = chrono::high_resolution_clock::now();
    // Start streaming
    msgSDR.str("");
    msgSDR << "SDR stream started as thread no: " << threadID << " with sampleCnt (I+Q): " << sampleCnt;
    Logger(msgSDR.str());

    while (rxON)
    {
        pthread_mutex_lock(&SDRmutex);
        samplesRead = LMS_RecvStream(&streamId, buffer, sampleCnt, NULL, 1000);
        pthread_mutex_unlock(&SDRmutex);
    }

    // Stop streaming
    LMS_StopStream(&streamId);            // stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamId); // stream is deallocated and can no longer be used

    // Close device
    if (LMS_Close(device) == 0)
    {
        msgSDR.str("");
        msgSDR << "Closed" << endl;
        Logger(msgSDR.str());
    }
    pthread_exit(NULL);
}

void *startSocketConnect(void *threadID)
{
    msgSDR.str("");
    msgSDR << "Socket connection started as connect no: " << (int)threadID << " using port: " << RPX_port << ", rxON=" << rxON;
    Logger(msgSDR.str());

    while (socketsON)
    {
        msgSDR.str("");
        int i = 0;
        while (i < samplesRead)
        {
            msgSDR << buffer[i] << "i" << buffer[i + 1] << ",";
            i = i + 2;
        }
        msgSDR << endl;
        RPX_socket[(int)threadID] << msgSDR.str();
    }

    pthread_exit(NULL);
}

EchoServer::EchoServer(int port) : WebSocketServer(port)
{
}

EchoServer::~EchoServer()
{
}

void EchoServer::onConnect(int socketID)
{
    Util::log("New connection");
    
}

void EchoServer::onMessage(int socketID, const string &data)
{
    // Reply back with the same message
    // Util::log("Received: " + data);
    // this->send(socketID, data);
    while (socketsON)
    {
        msgSDR.str("");
        int i = 0;
        while (i < samplesRead)
        {
            msgSDR << buffer[i] << "i" << buffer[i + 1] << ",";
            i = i + 2;
        }
        msgSDR << endl;
        this->send(socketID, msgSDR.str());
    }
}

void EchoServer::onDisconnect(int socketID)
{
    Util::log("Disconnect");
}

void EchoServer::onError(int socketID, const string &message)
{
    Util::log("Error: " + message);
}
