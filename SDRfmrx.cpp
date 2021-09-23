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

int duration = 10;

static char audioDev[] = "default";

int error();
void print_gpio(uint8_t gpio_val);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100 with default settings:\n";
        cout << "Frequency: " << centerFrequency << endl;
        cout << "Deviation: " << fmDeviation << endl;
        cout << "Duration: " << duration << endl;
        cout << "Sample Rate: " << sampleRate << endl;
        cout << endl;
        cout << "type \033[36m'SDRfmrx help'\033[0m to see all options !" << endl;
    }
    else if (argc >= 2)
    {
        for (int c = 0; c < argc; c++)
        {
            switch (c)
            {
            case 1:
                cout << "Center Frequency: " << argv[c] << endl;
                centerFrequency = stof(argv[c]);
                break;

            case 2:
                cout << "Deviation: " << argv[c] << endl;
                fmDeviation = atof(argv[c]);
                break;

            case 3:
                cout << "Duration: " << argv[c] << endl;
                duration = stoi(argv[c]);
                break;

            case 4:
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
    msg << "Center Frequency: " << centerFrequency;
    Logger(msg.str());
    msg.str("");
    msg << "Deviation: " << fmDeviation;
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

    if (LMS_GPIOWrite(device, &setRX, 1) != 0)
    {
        error();
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
    msg << "LimeRFE set to RX" << endl;
    Logger(msg.str());

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
    lms_stream_t streamId;                        //stream structure
    streamId.channel = 0;                         //channel number
    streamId.fifoSize = 1024 * 1024;              //fifo size in samples
    streamId.throughputVsLatency = 1.0;           //optimize for max throughput
    streamId.isTx = false;                        //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_I16; //12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    unsigned int i;
    unsigned int k;
    uint16_t sdrSample[INPUT_BUFFER_SIZE];

    // filter options
    unsigned int filter_len = 64;
    float filter_cutoff_freq = bandwidth / sampleRate;
    float filter_attenuation = 70.0f; // stop-band attenuation

    // design filter from prototype and scale to bandwidth
    firfilt_crcf filter = firfilt_crcf_create_kaiser(filter_len, filter_cutoff_freq, filter_attenuation, 0.0f);
    firfilt_crcf_set_scale(filter, 2.0f * filter_cutoff_freq);

    // resampler options
    float resampler_rate = resampleRate / sampleRate;
    msresamp_crcf resampler = msresamp_crcf_create(resampler_rate, filter_attenuation);
    float resampler_delay = msresamp_crcf_get_delay(resampler);

    // number of input samples (zero-padded)
    unsigned int resampler_input_len = NUM_SAMPLES + (int)ceilf(resampler_delay) + 10;
    // output buffer with extra padding
    unsigned int resampler_output_len = (unsigned int)(2 * (float)resampler_input_len * resampler_rate);

    liquid_float_complex c_input[resampler_input_len];
    liquid_float_complex c_output[resampler_output_len];
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
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(duration)) //run for 10 seconds
    {
        //Receive samples - I and Q samples are interleaved in sdrSample: IQIQIQ...
        int samplesRead = LMS_RecvStream(&streamId, sdrSample, INPUT_BUFFER_SIZE, NULL, 1000);

        if (samplesRead)
        {
            // convert int16_t IQ to complex float
            for (k = 0, i = 0; k < samplesRead / 2; k += 2, i++)
            {
                c_input[i] = sdrSample[k] / 32768.0 + sdrSample[k + 1] / 32768.0 * I;

                // run filter
                firfilt_crcf_push(filter, c_input[i]);
                firfilt_crcf_execute(filter, &c_input[i]);
            }

            // resample
            msresamp_crcf_execute(resampler, c_input, NUM_SAMPLES, c_output, &resampler_output_count);

            // demodulate
            freqdem_demodulate_block(fm_demodulator, c_output, resampler_output_count, dem_out);

            // send filtered signal to Audio Codec
            frames = snd_pcm_writei(handle, dem_out, resampler_output_count);
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
    // destroy filter object
	firfilt_crcf_destroy(filter);

	// destroy resampler object
	msresamp_crcf_destroy(resampler);

	// destroy fm demodulator object
	freqdem_destroy(fm_demodulator);

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
