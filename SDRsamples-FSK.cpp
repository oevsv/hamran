/******************************************************************************
 * C++ source of RPX-100-TX
 *
 * File:   SDRsamples-TX.cpp
 * Author: Bernhard Isemann
 *
 * Created on 06 Jan 2022, 10:35
 * Updated on 20 Feb 2022, 17:20
 * Version 2.00
 *****************************************************************************/

#include "SDRsamples-FSK.h"

using namespace std;

void *sendBeacon(void *threadID)
{
    uint16_t interval = 1; // time in minutes between beacon frames
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;

    while (txON)
    {
        if (chrono::high_resolution_clock::now() - t2 > chrono::seconds(60 * interval))
        {
            t2 = chrono::high_resolution_clock::now();

            // call SDRinitTX (TX6mPTT)
            if (SDRset(52.8e6, sampleRate, 6, 1.0) != 0)
            {
                msgSDR.str("");
                msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
                Logger(msgSDR.str());
            }

            startSDRTXStream(beaconMessage);
            msgSDR.str("");
            msgSDR << "Send Beacon";
            Logger(msgSDR.str());

            // call SDRiniTX (RX)
            if (SDRset(52.8e6, sampleRate, 0, 1.0) != 0)
            {
                msgSDR.str("");
                msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
                Logger(msgSDR.str());
            }
        }
    }

    // Close device
    if (LMS_Close(device) == 0)
    {
        msgSDR.str("");
        msgSDR << "Closed" << endl;
        Logger(msgSDR.str());
    }

    pthread_exit(NULL);
}

int startSDRTXStream(string message) 
{
    uint16_t interval = 1; // time between beacon frames

    // Initialize stream
    lms_stream_t streamId;                        // stream structure
    streamId.channel = 0;                         // channel number
    streamId.fifoSize = 1024 * 1024;              // fifo size in samples
    streamId.throughputVsLatency = 1.0;           // optimize for max throughput
    streamId.isTx = true;                         // TX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32; // 12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    // Start streaming
    LMS_StartStream(&streamId);

    // create mod/demod objects
    freqmod mod = freqmod_create(kf);   // modulator
    freqdem dem = freqdem_create(kf);   // demodulator
    freqmod_print(mod);

    unsigned int i;

    // generate message signal (sum of sines)
    for (i=0; i<sampleCnt; i++) {
        sig[i] = 0.3f*cosf(2*M_PI*0.013f*i + 0.0f) + 0.2f*cosf(2*M_PI*0.021f*i + 0.4f) + 0.4f*cosf(2*M_PI*0.037f*i + 1.7f);
    }

    // modulate signal
    freqmod_modulate_block(mod, sig, sampleCnt, c_buffer);

    for (int i = 0; i < sampleCnt; i++)
    {
        int k = 0;
        while (k < sampleCnt)
        {
            buffer[2*k] = c_buffer[k].real();
            buffer[2*k + 1] = c_buffer[k].imag();
            k++;
        }
    }

    // transmitting the buffer
    int ret = LMS_SendStream(&streamId, buffer, sampleCnt*2, nullptr, 1000);

    // Stop streaming
    LMS_StopStream(&streamId);            // stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamId); // stream is deallocated and can no longer be used

    return 0;
}

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

int SDRset(double frequency, double sampleRate, int modeSelector, double normalizedGain)
{
    // Set SDR GPIO diretion GPIO0-7 to output
    uint8_t gpio_dir = 0xFF;
    if (LMS_GPIODirWrite(device, &gpio_dir, 1) != 0)
    {
        error();
    }

    // Set GPIOs for RXTX mode (modeselctor)
    if (LMS_GPIOWrite(device, &modeGPIO[modeSelector], 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    uint8_t gpio_val = 0;
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

    // Enable RX or TX channel,Channels are numbered starting at 0
    if (modeSelector == 0)
    {
        if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        {
            error();
        }
        if (LMS_EnableChannel(device, LMS_CH_TX, 0, false) != 0)
        {
            error();
        }
    }
    else
    {
        if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0)
        {
            error();
        }
        if (LMS_EnableChannel(device, LMS_CH_RX, 0, false) != 0)
        {
            error();
        }
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
    if (modeSelector == 0)
    {
        if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, frequency) != 0)
        {
            error();
        }
    }
    else
    {
        if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, frequency) != 0)
        {
            error();
        }
    }

    msgSDR.str("");
    msgSDR << "Center frequency: " << frequency / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // select Low TX path for LimeSDR mini --> TX port 2 (misslabed in MINI, correct in USB)
    if (modeSelector == 0)
    {
        if (LMS_SetAntenna(device, LMS_CH_RX, 0, LMS_PATH_LNAL) != 0)
        {
            error();
        }
    }
    else
    {
        if (LMS_SetAntenna(device, LMS_CH_TX, 0, LMS_PATH_TX2) != 0)
        {
            error();
        }

        // set TX gain
        if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, normalizedGain) != 0)
        {
            error();
        }
    }

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

