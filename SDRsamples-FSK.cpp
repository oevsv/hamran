/******************************************************************************
 * C++ source of RPX-100-TX
 *
 * File:   SDRsamples-TX.cpp
 * Author: Bernhard Isemann
 *
 * Created on 06 Jan 2022, 10:35
 * Updated on 27 Feb 2022, 17:20
 * Version 2.00
 *****************************************************************************/

#include "SDRsamples-FSK.h"

using namespace std;

void *sendBeacon(void *threadID)
{
    uint16_t interval = 1; // time in 30secs intervals between beacon frames
    auto t1 = chrono::high_resolution_clock::now();

    while (txON)
    {
        if (chrono::high_resolution_clock::now() - t1 > chrono::seconds(30 * interval))
        {
            t1 = chrono::high_resolution_clock::now();

            // call SDRinitTX (TX6mPTT)
            if (SDRset(52.8e6, 4e6, 5, 1) != 0)
            {
                msgSDR.str("");
                msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
                Logger(msgSDR.str());
            }

            sleep(1);

            startSDRTXStream(beaconMessage);
            msgSDR.str("");
            msgSDR << "Send Beacon.\n";
            Logger(msgSDR.str());

            sleep(1);

            // call SDRiniTX (RX)
            if (SDRset(52.8e6, 4e6, 0, 1) != 0)
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

    // Initialize stream
    lms_stream_t streamId;                        // stream structure
    streamId.channel = 0;                         // channel number
    streamId.fifoSize = 1024 * 1024;              // fifo size in samples
    streamId.throughputVsLatency = 1.0;           // optimize for max throughput
    streamId.isTx = true;                         // TX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32; // 12-bit integers

    lms_stream_meta_t meta_tx;   
    meta_tx.waitForTimestamp = false;
    meta_tx.flushPartialPacket = false;
    meta_tx.timestamp = 0;


    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    // Start streaming
    LMS_StartStream(&streamId);

    //modulator
    freqmod mod = freqmod_create(modFactor); // modulator
    float f_ratio = toneFrequency / sampleRate;

    //Initialize data buffers
    liquid_float_complex mod_buffer[sampleCnt]; //TX buffer to hold complex values - liquid library)
    float test_tone[2*sampleCnt];

    msgSDR.str("");
    msgSDR << "Modulation Factor: " << modFactor << endl;
    Logger(msgSDR.str());

    // generate message signal (sum of sines)
    for (int i = 0; i <sampleCnt; i++)
    {
        test_tone[2*i] = cos(2*M_PI*i/16.0);
        test_tone[2*i+1] = sin(2*M_PI*i/16.0);
    }

    freqmod_modulate_block(mod, test_tone, sampleCnt, mod_buffer);

    int i = 0;
    while (i < sampleCnt)
     {
         buffer[2 * i] = mod_buffer[i].real();
         buffer[2*i + 1] = mod_buffer[i].imag();
         i++;
     }

    // transmitting the buffer
    auto t1 = chrono::high_resolution_clock::now();
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(tx_time)) // run for 10 seconds
    {
        int ret = LMS_SendStream(&streamId, buffer, sampleCnt, &meta_tx, 1000);
    }
    // Stop streaming
    LMS_StopStream(&streamId);            // stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamId); // stream is deallocated and can no longer be used

    return 0;
}

int SDRinit(double freq, double sampleR, int modeSel, double normGain)
{
    // Find devices
    int n;
    lms_info_str_t list[8]; // should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0)
    {
        error(); // NULL can be passed to only get number of devices
    }
    msgSDR.str("");
    msgSDR << "Number of devices found: " << n << endl;
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

    // Write and log GPIO values
    uint8_t gpio_val = 0;

    // Set GPIOs to RX mode (initial settings)
    if (LMS_GPIOWrite(device, &modeGPIO[modeSel], 1) != 0)
    {
        error();
    }

    if (LMS_GPIORead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "GPIO Output to High Level:\n";
    print_gpio(gpio_val);
    Logger(msgSDR.str());

    msgSDR.str("");
    msgSDR << "LimeRFE set to " << modeName[modeSel] << endl;
    Logger(msgSDR.str());

    // Enable TX channel,Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0)
    {
        error();
    }

    // Set sample rate
    if (LMS_SetSampleRate(device, sampleR, 0) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Sample rate: " << sampleR / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // Set center frequency
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, freq) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Center frequency: " << freq / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // select Low TX path for LimeSDR mini --> TX port 2 (misslabed in MINI, correct in USB)
    if (LMS_SetAntenna(device, LMS_CH_TX, 0, LMS_PATH_TX2) != 0)
    {
        error();
    }
     if (LMS_SetAntenna(device, LMS_CH_RX, 0, LMS_PATH_LNAW) != 0)
    {
        error();
    }

    // set TX gain
    if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, normGain) != 0)
    {
        error();
    }

    // calibrate Tx, continue on failure
    LMS_Calibrate(device, LMS_CH_TX, 0, sampleR, 0);

    sleep(1);

    return 0;
}

int SDRset(double freq, double sampleR, int modeSel, double normGain)
{
    // Set GPIOs for RXTX mode (modeselctor)
    if (LMS_GPIOWrite(device, &modeGPIO[modeSel], 1) != 0)
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
    msgSDR << "LimeRFE set to " << modeName[modeSel] << endl;
    Logger(msgSDR.str());

    // Enable RX or TX channel,Channels are numbered starting at 0
    if (modeSel == 0)
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
    if (LMS_SetSampleRate(device, sampleR, 0) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Sample rate: " << sampleR / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // Set center frequency
    if (modeSel == 0)
    {
        if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, freq) != 0)
        {
            error();
        }
    }
    else
    {
        if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, freq) != 0)
        {
            error();
        }
    }

    msgSDR.str("");
    msgSDR << "Center frequency: " << freq / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // select Low TX path for LimeSDR mini --> TX port 2 (misslabed in MINI, correct in USB)
    if (modeSel == 0)
    {
        if (LMS_SetAntenna(device, LMS_CH_RX, 0, LMS_PATH_LNAW) != 0)
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
        if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, normGain) != 0)
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
