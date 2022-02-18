/******************************************************************************
 * C++ source of RPX-100-TX
 *
 * File:   RPX-100-TX.cpp
 * Author: Bernhard Isemann
 *         Marek Honek
 *
 * Created on 01 Jan 2022, 10:35
 * Updated on 15 Feb 2022, 17:20
 * Version 2.00
 *****************************************************************************/

#include "RPX-100-Beacon-reorganized.h"
#include <pthread.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100-Beacon with default settings:\n";
        cout << "Mode: TX6mPTT" << endl;
        cout << endl;
        cout << "type \033[36m'RF-100-Beacon help'\033[0m to see all options !" << endl;
    }
    else if (argc >= 2)
    {
        for (int c = 0; c < argc; c++)
        {
            switch (c)
            {
            case 1:
                mode = (string)argv[c];
                if (mode == "RX")
                {
                    cout << "Starting RPX-100-Beacon with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 0;
                }           
                else if (mode == "TXDirectPTT")
                {
                    cout << "Starting RPX-100-Beacon with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 5;
                }
                else if (mode == "TX6mPTT")
                {
                    cout << "Starting RPX-100-Beacon with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 6;
                }
                else if (mode == "TX2mPTT")
                {
                    cout << "Starting RPX-100-Beacon with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 7;
                }
                else if (mode == "TX70cmPTT")
                {
                    cout << "Starting RPX-100-Beacon with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 8;
                }
                else if (mode == "help")
                {
                    cout << "Options for starting RPX-100-Beacon: RF-100-Beacon \033[36mMODE\033[0m" << endl;
                    cout << endl;
                    cout << "\033[36mMODE\033[0m:" << endl;
                    cout << "     \033[32mRX\033[0m for receive mode" << endl;
                    cout << endl;
                    cout << "     \033[31mTXDirectPTT\033[0m for transmit mode with PTT without bandpass filter" << endl;
                    cout << "     \033[31mTX6mPTT\033[0m for transmit mode with PTT with bandpass filter for 50-54 MHz" << endl;
                    cout << "     \033[31mTX2mPTT\033[0m for transmit mode with PTT with bandpass filter for 144-146 MHz" << endl;
                    cout << "     \033[31mTX70cmPTT\033[0m for transmit mode with PTT with bandpass filter for 430-440 MHz" << endl;
                    cout << endl;
                    return 0;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'RPX-100-Beacon help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;
            }
        }
    }

    LogInit();
    Logger("RPX-100-TX was started succesfully with following settings:");
    msg.str("");
    msg << "Mode: " << mode;
    Logger(msg.str());

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&SDRmutex, 0);

    if (pthread_create(&threads[4], NULL, sendBeacon, (void *)4) != 0)
    {
        msg.str("");
        msg << "ERROR starting thread 4";
        Logger(msg.str());
    }

    pthread_mutex_destroy(&SDRmutex);
    pthread_exit(NULL);
}

void *sendBeacon(void *threadID)
{
    uint16_t interval = 10; // time in minutes between beacon frames
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;

    while (txON)
    {
        if (chrono::high_resolution_clock::now() - t2 > chrono::seconds(60 * interval))
        {
            t2 = chrono::high_resolution_clock::now();

            BeaconFrameAssemble()

            // call SDRinitTX (TX6mPTT)
            if (SDRinitTX(52.8e6, 6, 1) != 0)
            {
                msgSDR.str("");
                msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
                Logger(msgSDR.str());
            }

            // call startSDRTXStream()

            // call SDRiniTX (RX)
            if (SDRinitTX(52.8e6, 0, 1) != 0)
            {
                msgSDR.str("");
                msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
                Logger(msgSDR.str());
            }

            msgSDR.str("");
            msgSDR << "Send Beacon";
            Logger(msgSDR.str());
        }
    }

    pthread_exit(NULL);
}

int BeaconFrameAssemble() // Marek to complete code !!!
{
    string message = "OE1XTU WRAN at 52.8 MHz";

    liquid_float_complex complex_i(0, 1);
    
    // create frame generator
    ofdmflexframegen fg;
    int DefineFrameGenerator (DEFAULT_CYCL_PREFIX, DEFAULT_PHY_MODE, &fg)

    // buffers
    liquid_float_complex c_buffer[c_buffer_len]; // time-domain buffer
    unsigned char header[8];                     // header data
    unsigned char payload[payload_len];          // payload data
    unsigned int r_buffer[2 * c_buffer_len];

    // ... initialize header/payload ...

    strcpy((char *)payload, message.c_str());

    header[0] = '0';
    header[1] = '0';
    header[2] = '0';
    header[3] = '0';
    header[4] = '0';
    header[5] = '0';
    header[6] = '0';
    header[7] = '0';

    // assemble frame
    ofdmflexframegen_assemble(fg, header, payload, payload_len);

    int last_symbol = 0;
    i = 0;
    l = 0;

    while (!last_symbol)
    {
        pthread_mutex_lock(&SDRmutex);
        // generate each OFDM symbol
        last_symbol = ofdmflexframegen_write(fg, c_buffer, c_buffer_len);

        for (i = 0; i < c_buffer_len; i++)
        {
            // r_buffer[2*i]=c_buffer.real;
            // r_buffer[2*i+1]=c_buffer.imag;
        }
        pthread_mutex_unlock(&SDRmutex);
    }
}

int startSDRTXStream() // Marek to complete code !!!
{
    uint16_t interval = 10; // time between beacon frames

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

    auto t1 = chrono::high_resolution_clock::now();
    pthread_mutex_lock(&SDRmutex);

    // transmitting the buffer
    // int ret = LMS_SendStream(&streamId, r_buffer, c_buffer_len, nullptr, 1000);

    pthread_mutex_unlock(&SDRmutex);

    //    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(interval)); // wait for another transmission

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
}

int SDRinitTX(double frequency, int modeSelector, double normalizedGain)
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
    if (LMS_SetAntenna(device, LMS_CH_TX, 0, LMS_PATH_LNAL) != 0)
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

int DefineProperties (int dp_cycl_pref; int dp_PHYmode)
{
    // initialize frame generator properties
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check = LIQUID_CRC_NONE;
    fgprops.fec1 = LIQUID_FEC_NONE;

    switch (dp_cycl_pref)
    {
    case 4:
        useful_symbols = 22;
        sampleRate = 3328000;
        break;
    case 8:
        useful_symbols = 24;
        sampleRate = 3225600;
        break;
    case 16:
        useful_symbols = 26;
        sampleRate = 3264000;
        break;
    case 32:
        useful_symbols = 27;
        sampleRate = 3273600;
        break;
    default:
        return -1;
    }

    cp_len = (int)SUBCARRIERS / dp_cycl_pref; // cyclic prefix length
    taper_len = (int)cp_len / 4;          // taper length

    switch (dp_PHYmode)
    {
    case 1: 
        fgprops.fec0 = LIQUID_FEC_NONE;
        fgprops.mod_scheme = LIQUID_MODEM_PSK2;
        bits_per_symbol = 1;
        break;
    case 2:
        // not supported
        break;
    case 3:
        fgprops.fec0 = LIQUID_FEC_CONV_V27;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
        bits_per_symbol = 2.0f / 2.0f;
        break;
    case 4:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
        bits_per_symbol = 2.0f * 3.0f / 2.0f;
        break;
    case 5:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
        bits_per_symbol = 2.0f * 4.0f / 3.0f;
        break;
    case 6:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
        bits_per_symbol = 2.0f * 6.0f / 5.0f;
        break;
    case 7:
        fgprops.fec0 = LIQUID_FEC_CONV_V27;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        bits_per_symbol = 4.0f / 2.0f;
        break;
    case 8:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        bits_per_symbol = 4.0f * 3.0f / 2.0f;
        break;
    case 9:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        bits_per_symbol = 4.0f * 4.0f / 3.0f;
        break;
    case 10:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        bits_per_symbol = 4.0f * 6.0f / 5.0f;
        break;
    case 11:
        fgprops.fec0 = LIQUID_FEC_CONV_V27;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        bits_per_symbol = 6.0f / 2.0f;
        break;
    case 12:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        bits_per_symbol = 6.0f * 3.0f / 2.0f;
        break;
    case 13:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        bits_per_symbol = 6.0f * 4.0f / 3.0f;
        break;
    case 14:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        bits_per_symbol = 6.0f * 6.0f / 5.0f;
        break;
    default:
        return -1;
    }

    // length of payload (bytes)
    unsigned int payload_len = floor(DATACARRIERS * useful_symbols * bits_per_symbol / 8);
    unsigned int c_buffer_len = SUBCARRIERS + cp_len; // length of buffer

    unsigned char allocation_array[SUBCARRIERS];      // subcarrier allocation array(null/pilot/data)
    subcarrier_allocation(allocation_array);
    
    fg = ofdmflexframegen_create(SUBCARRIERS, cp_len, taper_len, allocation_array, &fgprops);
}

int subcarrier_allocation (int &array)
{
    for (int i = 0; i < 1024; i++)
    {
        if (i < 232)
            array[i] = 0; // guard band

        if (231 < i && i < 792)
            if (i % 7 == 0)
                array[i] = 1; // every 7th carrier pilot
            else
                array[i] = 2; // rest data

        if (i > 791)
            array[i] = 0; // guard band
    }
}