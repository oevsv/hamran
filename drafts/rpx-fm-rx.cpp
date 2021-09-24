/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   rpx-fm-rx.cpp
 * Author: Bernhard Isemann
 *
 * Created on 23 Sep 2021, 09:05
 * Updated on 24 Sep 2021, 14:00
 * Version 1.00
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sstream>
#include <syslog.h>
#include <string.h>
#include <iostream>
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
#include <wiringPi.h>
#include <wiringSerial.h>
#include "lime/LimeSuite.h"
#include <chrono>
#include <math.h>
#include "alsa/asoundlib.h"
#include "liquid/liquid.h"

using namespace std;
lms_device_t *device = NULL;
std::stringstream msg;
std::stringstream HEXmsg;
static char audioDev[] = "default";
uint8_t setRX = 0x04;     //all other bit = 0 --> 6m
uint8_t setTXwoBP = 0x0B; //all other bit = 0 --> direct path without BP
uint8_t setTX6m = 0x08;   //all other bit = 0 --> 6m with BP
uint8_t setTX2m = 0x09;   //all other bit = 0 --> 2m with BP
uint8_t setTX70cm = 0x0A; //all other bit = 0 --> 70cm with BP
float centerFrequency = 99.9e6;
string mode = "RX";
float normalizedGain = 0;
float modFactor = 0.8f;
float deviation = 25e3;
int modeSelector = 1;
int duration = 10;
float toneFrequency = 2e3;
float sampleRate = 2e6;
float resampleRate = 240e3f;
float bandwidth = 100e3f;
float fmDeviation = 75e3f;

int error();
void print_gpio(uint8_t gpio_val);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100 with default settings:\n";
        cout << "Mode: " << mode << endl;
        cout << "Frequency: " << centerFrequency << endl;
        cout << "Deviation: " << deviation << endl;
        cout << "Modulation Factor: " << modFactor << endl;
        cout << "Duration: " << duration << endl;
        cout << "Sample Rate: " << sampleRate << endl;
        cout << endl;
        cout << "type \033[36m'fm-rx help'\033[0m to see all options !" << endl;
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
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 0;
                }
                else if (mode == "APRS")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 1;
                }
                else if (mode == "OFDM")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 2;
                }
                else if (mode == "help")
                {
                    cout << "Options for starting RPX-100: fm-rx \033[36mMODE CENTER-FREQUENCY DEVIATION MODULATION-FACTOR DURATION SAMPLE-RATE\033[0m" << endl;
                    cout << endl;
                    cout << "\033[36mMODE\033[0m:" << endl;
                    cout << "     \033[32mRX\033[0m for receiving Analog FM" << endl;
                    cout << "     \033[31mAPRS\033[0m for receiving APRS" << endl;
                    cout << "     \033[31mOFDM\033[0m for receiving OFDM frames" << endl;
                    cout << endl;
                    cout << "\033[36mCENTER-FREQUENCY\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mDEVIATION\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mMODULATION-FACTOR\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mDURATION\033[0m:" << endl;
                    cout << "     in sec, number of type int" << endl;
                    cout << endl;
                    cout << "\033[36mSAMPLE-RATE033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    return 0;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'fm-rx help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;

            case 2:
                cout << "Center Frequency: " << argv[c] << endl;
                centerFrequency = stof(argv[c]);
                break;

            case 3:
                cout << "Deviation: " << argv[c] << endl;
                deviation = atof(argv[c]);
                break;

            case 4:
                cout << "Modulation Factor: " << argv[c] << endl;
                modFactor = stof(argv[c]);
                break;

            case 5:
                cout << "Duration: " << argv[c] << endl;
                duration = stoi(argv[c]);
                break;

            case 6:
                cout << "Sample Rate: " << argv[c] << endl;
                sampleRate = stof(argv[c]);
                break;
            }
        }
    }

    pid_t pid, sid;
    pid = fork();
    if (pid < 0)
    {
        return 1;
    }
    if (pid > 0)
    {
        return 1;
    }

    umask(0);

    sid = setsid();
    if (sid < 0)
    {
        return 1;
    }

    if ((chdir("/")) < 0)
    {
        return 1;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    LogInit();
    Logger("RPX-100 was started succesfully with following settings:");
    msg.str("");
    msg << "Mode: " << mode;
    Logger(msg.str());
    msg.str("");
    msg << "Center Frequency: " << centerFrequency;
    Logger(msg.str());
    msg.str("");
    msg << "Deviation: " << deviation;
    Logger(msg.str());
    msg.str("");
    msg << "Modulation factor: " << modFactor;
    Logger(msg.str());
    msg.str("");
    msg << "Duration: " << duration;
    Logger(msg.str());
    msg.str("");
    msg << "Sample Rate: " << sampleRate;
    Logger(msg.str());

    if (wiringPiSetup() == -1) /* initializes wiringPi setup */
    {
        msg.str("");
        msg << "Unable to start wiringPi: " << strerror(errno);
        return 1;
    }

    //Find devices
    int n;
    lms_info_str_t list[8]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0)
    {
        error(); //NULL can be passed to only get number of devices
    }
    msg.str("");
    msg << "Number of devices found: " << n;
    Logger(msg.str()); //print number of devices
    if (n < 1)
    {
        return -1;
    }

    if (LMS_Open(&device, list[0], NULL)) //open the first device
    {
        error();
    }
    sleep(1);
    //Initialize device with default configuration
    if (LMS_Init(device) != 0)
    {
        error();
    }
    sleep(1);

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msg.str("");
    msg << "Read current GPIO state.\n";
    Logger(msg.str());

    uint8_t gpio_dir = 0xFF;
    if (LMS_GPIODirWrite(device, &gpio_dir, 1) != 0)
    {
        error();
    }

    if (LMS_GPIODirRead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msg.str("");
    msg << "Set GPIOs direction to output.\n";
    Logger(msg.str());

    switch (modeSelector)
    {
    case 0:
        if (LMS_GPIOWrite(device, &setRX, 1) != 0)
        {
            error();
        }
        break;

    case 1:
        if (LMS_GPIOWrite(device, &setRX, 1) != 0)
        {
            error();
        }
        break;

    case 2:
        if (LMS_GPIOWrite(device, &setRX, 1) != 0)
        {
            error();
        }
        break;
    }

    if (LMS_GPIORead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msg.str("");
    msg << "GPIO Output to High Level:\n";
    print_gpio(gpio_val);
    Logger(msg.str());

    msg.str("");
    msg << "LimeRFE set to " << mode << endl;
    Logger(msg.str());

    // Send single tone
    const int tx_time = (const int)duration;
    float f_ratio = toneFrequency / sampleRate;

    // open Audio Device
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;

    if (snd_pcm_open(&handle, audioDev, SND_PCM_STREAM_PLAYBACK, 0) != 0)
    {
        error();
    }
    if (snd_pcm_set_params(handle,
                           SND_PCM_FORMAT_FLOAT,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           1,
                           resampleRate,
                           1,
                           500000) != 0)
    { /* 0.5sec */
        error();
    }

    //Enable RX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();

    //Set center frequency to 800 MHz
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, centerFrequency) != 0)
        error();

    //Set sample rate to 8 MHz, ask to use 2x oversampling in RF
    //This set sampling rate for all channels
    if (LMS_SetSampleRate(device, sampleRate, 2) != 0)
        error();

        //To receive data from RF, remove this line or change signal to LMS_TESTSIG_NONE
    if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NONE, 0, 0) != 0)
        error();

    //Streaming Setup

    //Initialize stream
    lms_stream_t streamId;                        //stream structure
    streamId.channel = 0;                         //channel number
    streamId.fifoSize = 1024 * 1024;              //fifo size in samples
    streamId.throughputVsLatency = 1.0;           //optimize for max throughput
    streamId.isTx = false;                        //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32; //12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    //Initialize data buffers
    unsigned int i;
    unsigned int k;
    const int sampleCnt = 512;
    const int IQpairCnt = sampleCnt/4;   //complex samples per buffer
    float buffer[sampleCnt * 2]; //buffer to hold complex values (2*samples))

    // filter options
    unsigned int filter_len = 64;
    float filter_cutoff_freq = bandwidth / sampleRate;
    float filter_attenuation = 70.0f; // stop-band attenuation

    // design filter from prototype and scale to bandwidth
    firfilt_cccf filter = firfilt_cccf_create_kaiser(filter_len, filter_cutoff_freq, filter_attenuation, 0.0f);
    firfilt_cccf_set_scale(filter, 2.0f * filter_cutoff_freq);

    // resampler options
    float resampler_rate = resampleRate / sampleRate;
    msresamp_cccf resampler = msresamp_cccf_create(resampler_rate, filter_attenuation);
    float resampler_delay = msresamp_cccf_get_delay(resampler);

    // number of input samples (zero-padded)
    unsigned int resampler_input_len = IQpairCnt + (int)ceilf(resampler_delay) + 10;
    // output buffer with extra padding
    unsigned int resampler_output_len = (unsigned int)(2 * (float)resampler_input_len * resampler_rate);
    complex<float> c_input[resampler_input_len];
    complex<float> c_output[resampler_output_len];
    complex<float> j(0,1);
    unsigned int resampler_output_count = 0;

    // FM demodulator
    float kf = fmDeviation / resampleRate; // modulation factor
    freqdem fm_demodulator = freqdem_create(kf);
    float dem_out[resampler_output_len];

    //Start streaming
    LMS_StartStream(&streamId);

    //Streaming
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;

    //Start streaming
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(tx_time)) //run for 10 seconds
    {
        //Receive samples - I and Q samples are interleaved in buffer: IQIQIQ..
        int samplesRead = LMS_RecvStream(&streamId, buffer, sampleCnt, NULL, 1000);
        if (samplesRead > 0)
        {
            // convert int16_t IQ to complex float
            for (i = 0; i < samplesRead/2; i++)
            {
                c_input[i] = (buffer[2+i])  + j*(buffer[2*i + 1]);
                firfilt_cccf_push(filter, c_input[i]);
                firfilt_cccf_execute(filter, &c_input[i]);
            }

            // resample
            msresamp_cccf_execute(resampler, c_input, IQpairCnt, c_output, &resampler_output_count);

            // demodulate
            freqdem_demodulate_block(fm_demodulator, c_output, resampler_output_len, dem_out);

            // send filtered signal to Audio Codec
            frames = snd_pcm_writei(handle, dem_out, sizeof(dem_out));

        }

        //Print data rate (once per second)
        if (chrono::high_resolution_clock::now() - t2 > chrono::seconds(5))
        {
            t2 = chrono::high_resolution_clock::now();
            msg.str("");
            msg << "Received: " << samplesRead << " samples" << endl;
            Logger(msg.str());
        }
    }

    sleep(1);

    //Stop streaming
    LMS_StopStream(&streamId);            //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamId); //stream is deallocated and can no longer be used

    //Close device
    if (LMS_Close(device) == 0)
    {
        msg.str("");
        msg << "Closed" << endl;
        Logger(msg.str());
    }

    return 0;
}

int error()
{
    msg.str("");
    msg << "ERROR: " << LMS_GetLastErrorMessage();
    Logger(msg.str());
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

void print_gpio(uint8_t gpio_val)
{
    for (int i = 0; i < 8; i++)
    {
        bool set = gpio_val & (0x01 << i);
        msg << "GPIO" << i << ": " << (set ? "High" : "Low") << std::endl;
    }
}
