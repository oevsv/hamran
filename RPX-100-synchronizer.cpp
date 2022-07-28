/******************************************************************************
 * C++ source of RPX-100-TX
 *
 * File:   RPX-100-TX.cpp
 * Author: Bernhard Isemann
 *         Marek Honek
 *
 * Created on 19 Apr 2022, 18:20
 * Updated on 22 May 2022, 18:00
 * Version 1.00
 * Predecessor  RPX-100-Beacon-reorganized.cpp
 * Goal         Create synchronizer and artificial channel
 *****************************************************************************/

#include "RPX-100-synchronizer.h"
#include <pthread.h>

using namespace std;

int main(void)
{
    int modeSel = 6; //TX6mPTT as default

    LogInit();
    Logger("RPX-100-TX was started succesfully with following settings:");
    msgSDR.str("");
    msgSDR << "Mode: " << mode;
    Logger(msgSDR.str());

    
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&SDRmutex, 0);

    if (pthread_create(&threads[4], NULL, sendBeacon, (void *)4) != 0)
    {
        msgSDR.str("");
        msgSDR << "ERROR starting thread 4";
        Logger(msgSDR.str());
    }

    if (pthread_create(&threads[3], NULL, beaconReception, (void *)3) != 0)
    {
        msgSDR.str("");
        msgSDR << "ERROR starting thread 3";
        Logger(msgSDR.str());
    }

    pthread_mutex_destroy(&SDRmutex);

    pthread_exit(NULL);
}

void *sendBeacon(void *threadID)
{
    uint16_t interval = 30; // time in minutes between beacon frames
    auto t1 = chrono::high_resolution_clock::now();
 
    int frame_symbols = frameSymbols(DEFAULT_CYCL_PREFIX);
    int buffer[2*frame_symbols*(int)(SUBCARRIERS+SUBCARRIERS/DEFAULT_CYCL_PREFIX)];  //buffer for whole frame
    
    int symbolSampleCnt = complexFrameBufferLength(DEFAULT_CYCL_PREFIX);
    BeaconFrameAssemble(buffer);
    
    cout << "before while loop" << endl;

    while (txON)
    {
        if (chrono::high_resolution_clock::now() - t1 > chrono::seconds(/*60 * */ interval))
        {
            t1 = chrono::high_resolution_clock::now();

            setSampleRate(DEFAULT_CYCL_PREFIX);

            cout << "sample rate has been set" << endl;

                       
            //Send buffer to artificial channel buffer
            for (int i = 0; i++; i < 66560)
            {
                   noSDR_buffer[i] = buffer[i];
            }

            cout << "TX buffer was stored into noSDRbuffer" << endl;

            msgSDR.str("");
            msgSDR << "Send Beacon";
            Logger(msgSDR.str());
        }
    }

    pthread_exit(NULL);
}

void BeaconFrameAssemble(int *r_frame_buffer)
{
    string message = "OE1XTU WRAN at 52.8 MHz";

    liquid_float_complex complex_i(0, 1);
    
    unsigned int payload_len = payloadLength(DEFAULT_CYCL_PREFIX, DEFAULT_PHY_MODE); //depends on PHY mode and cyclic prefix
    uint c_buffer_len = complexFrameBufferLength(DEFAULT_CYCL_PREFIX); //depends on cyclic prefix

    // create frame generator
    ofdmflexframegen fg = DefineFrameGenerator(DEFAULT_CYCL_PREFIX, DEFAULT_PHY_MODE);

    // buffers
    liquid_float_complex c_buffer[c_buffer_len]; // time-domain buffer
    unsigned char header[8];                     // header data
    unsigned char payload[payload_len];          // payload data

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
    int i = 0;
    int l = 0;

    while (!last_symbol)
    {
        pthread_mutex_lock(&SDRmutex);
    
        // generate each OFDM symbol
        last_symbol = ofdmflexframegen_write(fg, c_buffer, c_buffer_len);

        for (i = 0; i < c_buffer_len; i++)
        {
            r_frame_buffer[l*2*c_buffer_len+2*i]=c_buffer->real();
            r_frame_buffer[l*2*c_buffer_len+2*i+1]=c_buffer->imag();
        }
        l++;
    
        pthread_mutex_unlock(&SDRmutex);
    }
}

void *beaconReception(void *threadID)
{
    liquid_float_complex complex_i (0, 1);

    uint c_sync_buffer_len = 32*(int)(SUBCARRIERS+SUBCARRIERS/4); //synchronizer buffer can be  of arbitrary length 
    liquid_float_complex c_sync_buffer[c_sync_buffer_len];
    uint r_sync_buffer[c_sync_buffer_len*2];

    unsigned char allocation_array[SUBCARRIERS];    // subcarrier allocation array (null/pilot/data)
    subcarrierAllocation(allocation_array);

    unsigned int cp_len = (int)SUBCARRIERS / DEFAULT_CYCL_PREFIX; // cyclic prefix length
    unsigned int taper_len = (int)cp_len / 4;          // taper length
  
    ofdmflexframesync fs = ofdmflexframesync_create(SUBCARRIERS, cp_len, taper_len, allocation_array, mycallback, NULL);

    while(1)
    {
        pthread_mutex_lock(&SDRmutex);

        //buffer handover
        for (int i = 0; i++; i < 2*frameSymbols(DEFAULT_CYCL_PREFIX)*(int)(SUBCARRIERS+SUBCARRIERS/DEFAULT_CYCL_PREFIX))
        {
            r_sync_buffer[i] = noSDR_buffer[i];
        }

        pthread_mutex_unlock(&SDRmutex);

        for (int i = 0; i < c_sync_buffer_len; i++)
        {
            c_sync_buffer[i]=r_sync_buffer[2*i]+r_sync_buffer[2*i+1] * complex_i.imag();
        }

        // receive symbol (read samples from buffer)
        ofdmflexframesync_execute(fs, c_sync_buffer, c_sync_buffer_len);
    }

    pthread_exit(NULL);
}


void print_gpio(uint8_t gpio_val)
{
    for (int i = 0; i < 8; i++)
    {
        bool set = gpio_val & (0x01 << i);
        msgSDR << "GPIO" << i << ": " << (set ? "High" : "Low") << std::endl;
    }
}

void setSampleRate(int cyclic_prefix)
{
    switch (cyclic_prefix)
    {
    case 4:
        sampleRate = 3328000;
        break;
    case 8:
        sampleRate = 3225600;
        break;
    case 16:
        sampleRate = 3264000;
        break;
    case 32:
        sampleRate = 3273600;
        break;
    }
}

ofdmflexframegen DefineFrameGenerator (int dfg_cycl_pref, int dfg_PHYmode)
{

    // initialize frame generator properties
    ofdmflexframegenprops_s fgprops;
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check = LIQUID_CRC_NONE;
    fgprops.fec1 = LIQUID_FEC_NONE;

    unsigned int cp_len = (int)SUBCARRIERS / dfg_cycl_pref; // cyclic prefix length
    unsigned int taper_len = (int)cp_len / 4;          // taper length

    switch (dfg_PHYmode)
    {
        
    case 1: 
        fgprops.fec0 = LIQUID_FEC_NONE;
        fgprops.mod_scheme = LIQUID_MODEM_PSK2;
        break;
    case 2:
        // not supported
        break;
    case 3:
        fgprops.fec0 = LIQUID_FEC_CONV_V27;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
        break;
    case 4:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
        break;
    case 5:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
         break;
    case 6:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
        break;
    case 7:
        fgprops.fec0 = LIQUID_FEC_CONV_V27;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        break;
    case 8:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        break;
    case 9:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        break;
    case 10:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
        fgprops.mod_scheme = LIQUID_MODEM_QAM16;
        break;
    case 11:
        fgprops.fec0 = LIQUID_FEC_CONV_V27;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        break;
    case 12:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        break;
    case 13:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        break;
    case 14:
        fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
        fgprops.mod_scheme = LIQUID_MODEM_QAM64;
        break;
    default:
        return NULL;
    }

    unsigned char allocation_array[SUBCARRIERS]; // subcarrier allocation array(null/pilot/data)
    subcarrierAllocation(allocation_array);
    
    return ofdmflexframegen_create(SUBCARRIERS, cp_len, taper_len, allocation_array, &fgprops);
}

void subcarrierAllocation (unsigned char *array)
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

int frameSymbols(int cyclic_prefix)
{
    switch (cyclic_prefix)
    {
    case 4:
        return 22+4;
    case 8:
        return 24+4;
    case 16:
        return 26+4;
    case 32:
        return 27+4;
    default:
        return 0;
    }
}

uint complexFrameBufferLength(int cyclic_prefix)
{
    return SUBCARRIERS + ((int)SUBCARRIERS / cyclic_prefix);
}

uint payloadLength(int cyclic_prefix, int phy_mode)
{
    uint8_t useful_symbols;
    float bits_per_symbol;

    switch (cyclic_prefix)
    {
        case 4:
            useful_symbols = 22;
            break;
        case 8:
            useful_symbols = 24;
            break;
        case 16:
            useful_symbols = 26;
            break;
        case 32:
            useful_symbols = 27;
            break;
        default:
            return 0;
    }
    

    switch (phy_mode)
    {        
        case 1: 
            bits_per_symbol = 1;
            break;
        case 2:
            // not supported
            break;
        case 3:
            bits_per_symbol = 2.0f / 2.0f;
            break;
        case 4:
            bits_per_symbol = 2.0f * 3.0f / 2.0f;
            break;
        case 5:
            bits_per_symbol = 2.0f * 4.0f / 3.0f;
            break;
        case 6:
            bits_per_symbol = 2.0f * 6.0f / 5.0f;
            break;
        case 7:
            bits_per_symbol = 4.0f / 2.0f;
            break;
        case 8:
            bits_per_symbol = 4.0f * 3.0f / 2.0f;
            break;
        case 9:
            bits_per_symbol = 4.0f * 4.0f / 3.0f;
            break;
        case 10:
            bits_per_symbol = 4.0f * 6.0f / 5.0f;
            break;
        case 11:
            bits_per_symbol = 6.0f / 2.0f;
            break;
        case 12:
            bits_per_symbol = 6.0f * 3.0f / 2.0f;
            break;
        case 13:
            bits_per_symbol = 6.0f * 4.0f / 3.0f;
            break;
        case 14:
            bits_per_symbol = 6.0f * 6.0f / 5.0f;
            break;
        default:
            return 0;
    }

    return (uint)floor(DATACARRIERS * useful_symbols * bits_per_symbol / 8);
}

// callback function
int mycallback(unsigned char *_header,
               int _header_valid,
               unsigned char *_payload,
               unsigned int _payload_len,
               int _payload_valid,
               framesyncstats_s _stats,
               void *_userdata)
{
    cout << endl;
    cout << "***** callback invoked!\n"
         << endl;
    cout << "  header " << _header_valid << endl;
    cout << "   payload " << _payload_valid << endl;

    unsigned int i;
    if (_header_valid)
    {
        cout << "Received header: \n"
             << endl;
        for (i = 0; i < 8; i++)
        {
            cout << _header[i] << endl;
        }
        cout << endl;
    }

    if (_payload_valid)
    {
        cout << "Received payload: \n"
             << endl;
        for (i = 0; i < _payload_len; i++)
        {
            if (_payload[i] == 0)
                break;
            cout << _payload[i] << endl;
        }
        cout << endl
             << endl;
    }

    return 0;
}