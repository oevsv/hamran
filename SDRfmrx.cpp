/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   fm-rx.cpp
 * Author: Bernhard Isemann
 *
 * Created on 02 Aug 2021, 09:05
 * Updated on 19 Sep 2021, 14:00
 * Version 1.00
 *****************************************************************************/

#include "RPX-100.h"

using namespace std;
lms_device_t *device = NULL;
std::stringstream msg;
std::stringstream HEXmsg;

float centerFrequency = 99.9e6;
string mode = "RX";
float normalizedGain = 0;
float modFactor = 0.8f;
float deviation = 10e3;
int modeSelector = 1;
int duration = 10;
float toneFrequency = 2e3;
float sampleRate = 2e6;

static char audioDev[] = "default";

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
                           SND_PCM_FORMAT_U8,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           1,
                           48000,
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

    //Streaming Setup

    //Initialize stream
    lms_stream_t streamId;                        //stream structure
    streamId.channel = 0;                         //channel number
    streamId.fifoSize = 1024 * 1024;              //fifo size in samples
    streamId.throughputVsLatency = 1.0;           //optimize for max throughput
    streamId.isTx = false;                        //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    //Initialize data buffers
    const int sampleCnt = 1024;        //complex samples per sdrSample
    float_t sdrSample[sampleCnt * 2];  //sdrSample to hold complex values (2*samples))
    float fc = CUTOFF_HZ / sampleRate; // cutoff frequency
    unsigned int h_len = 64;           // filter length
    float As = 70.0f;                  // stop-band attenuation
    liquid_float_complex receivedSignal[sampleCnt];
    float kf = deviation/sampleRate;// FSK_DEVIATION_HZ / sampleRate; // modulation factor
    freqdem dem = freqdem_create(kf);

    float demodSignal[sampleCnt];                 // filtered signal
    liquid_float_complex filterSignal[sampleCnt]; // filtered signal
    firfilt_crcf q = firfilt_crcf_create_kaiser(h_len, fc, As, 0.0f);
    firfilt_crcf_set_scale(q, 2.0f * fc);

    //Start streaming
    LMS_StartStream(&streamId);

    //Streaming
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    //Start streaming
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(tx_time)) //run for 10 seconds
    {
        //Receive samples - I and Q samples are interleaved in sdrSample: IQIQIQ...
        int samplesRead = LMS_RecvStream(&streamId, sdrSample, sampleCnt * 2, NULL, 1000);

        for (int i = 0; i < sampleCnt; i++)
        {
            receivedSignal[i] = (sdrSample[2 * i]) + _Complex_I * (sdrSample[(2 * i) + 1]);
        }
        
        // FM Demodulation
        freqdem_demodulate_block(dem, receivedSignal, sampleCnt, demodSignal);
        

        // send filtered signal to Audio Codec
        frames = snd_pcm_writei(handle, demodSignal, sizeof(demodSignal));

        //Print data rate (once per second)
        if (chrono::high_resolution_clock::now() - t2 > chrono::seconds(5))
        {
            t2 = chrono::high_resolution_clock::now();
            msg.str("");
            msg << "Received: " << samplesRead << " samples" << endl;
            Logger(msg.str());
        }
    }
    if (snd_pcm_drain(handle) != 0)
    {
        error();
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
    snd_pcm_close(handle);

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
