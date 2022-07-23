/******************************************************************************
 * C++ source of RPX-100-TX
 *
 * File:   RPX-100-TX.cpp
 * Author: Bernhard Isemann
 *         Marek Honek
 *
 * Created on 19 Apr 2022, 18:20
 * Updated on 01 Jun 2022, 08:00
 * Version 1.00
 * Predecessor  RPX-100-Beacon-reorganized.cpp
 * Goal         Create synchronizer and artificial channel
 *****************************************************************************/

#include "RPX-100-BER_simulator.h"
// #include <pthread.h>

using namespace std;

int main(void)
{
    if (PRINT)
        cout << "main - program started" << endl;

    int modeSel = 6; //TX6mPTT as default

    LogInit();
    if (PRINT)
        cout << "main - logger initalized" << endl;

    Logger("RPX-100-synchronizer was started.\n");
    msgSDR << "Mode: " << modeSel << endl;
    Logger(msgSDR.str());
    msgSDR.str("");

        
    if (PRINT)
        cout << "main - first log message saved" << endl;

    for (int repetition = 0; repetition < SIMULATION_REPETITION; repetition++) // repetition for higher resolution
    {
        message = alterMessage(8100); //max payload
    
        for (global_cycl_pref_index = 0; global_cycl_pref_index < 4; global_cycl_pref_index++) // cyclic prefix
        {
            for (global_phy_mode = 1; global_phy_mode <= 14; global_phy_mode++) // PHY mode
            {   
                if (global_phy_mode == 2)
                    global_phy_mode++;

                if (PRINT)
                    cout << "main - global_phy_mode: " << global_phy_mode << "; global_cycl_pref_index: " << global_cycl_pref_index << "; cycl_pref: " << (4 << global_cycl_pref_index) << endl;

                sendFrame(4 << global_cycl_pref_index, global_phy_mode);
    
                if (PRINT)
                    cout <<"main - sendFrame exitted" << endl;
                
                for (artificial_SNR = MIN_SNR; artificial_SNR<=MAX_SNR; artificial_SNR++) // SNR sweep
                {
                    artificialChannel(4 << global_cycl_pref_index);

                    if (PRINT)
                        cout << "main - artificialChannel exitted" << endl;

                    frameReception(4 << global_cycl_pref_index);
                    if (PRINT)
                        cout <<"main - frameReception exitted" << endl;
                }
            }
            cout << "main - repetition: " << repetition << "; cp: " << (4<<global_cycl_pref_index) << endl;
        }
    }
    
    // Up to now, BER_log contains sum of results from individual simulations. Following for loop structure divides
    // the value by number of simulations. Thus calculates the average of all simulations.
    for (global_cycl_pref_index = 0; global_cycl_pref_index<4; global_cycl_pref_index++) // cyclic prefix
    {
        for (global_phy_mode = 1; global_phy_mode <=14; global_phy_mode++) // PHY mode
        {    
            cout << "main - cyclic prefix: " << (4<<global_cycl_pref_index) << "; PHY mode: " << global_phy_mode << endl;           
            for (artificial_SNR = MIN_SNR; artificial_SNR<=MAX_SNR; artificial_SNR++) // SNR sweep
            {    
                BER_log[global_cycl_pref_index][global_phy_mode][artificial_SNR]/=SIMULATION_REPETITION;
            }
        }
    }

    exportBER(); //TBD

    return(0);    
}    
    

// void *sendFrame(void *threadID)
void sendFrame(int cyclic_prefix, int phy_mode)
{
    if (PRINT)
        cout << "sendFrame - sendFrame started - cyclic_prefix: " << cyclic_prefix << "; phy_mode: " << phy_mode << endl;
 
    int buffer_len = 2*complexFrameBufferLength(cyclic_prefix);
    if (PRINT)
        cout << "sendFrame - complexFrameBufferLength exitted - buffer_len: " << buffer_len << endl;

    float buffer[buffer_len];  //buffer for whole frame
    if (PRINT)
        cout << "sendFrame - buffer initialized" << endl;

    frameAssemble(buffer, cyclic_prefix, phy_mode);
    if (PRINT)
    {
        cout << "sendFrame - frameAssemble exitted" << endl;
        cout << "sendFrame - buffer[0]: " << buffer[0] << endl;
    }

    setSampleRate(cyclic_prefix);
    if (PRINT)
        cout << "sendFrame - setSampleRate exitted" << endl;

    
    //Send buffer to artificial channel buffer
    for (int i = 0; i < buffer_len; i++)
    {
        before_cahnnel_buffer[i] = buffer[i];
    }

    if (PRINT)
    {
        cout << "sendFrame - buffer[0]: " << buffer[0] << "   before_cahnnel_buffer[0]: " << before_cahnnel_buffer[0] << endl; 
        cout << "sendFrame - TX buffer stored into before_cahnnel_buffer" << endl;
    }


}

void frameAssemble(float *r_frame_buffer, int cyclic_prefix, int phy_mode)
{
    if (PRINT)
        cout << "frameAssemble - frameAssemble started" << endl;
    
    liquid_float_complex complex_i(0, 1);
    
    unsigned int payload_len = payloadLength(cyclic_prefix, phy_mode); //depends on PHY mode and cyclic prefix
    if (PRINT)
    {   
        cout << "frameAssemble - payloadLength exitted" << endl;
        cout << "frameAssemble - payload_len: " << payload_len << endl;
    }

    uint c_buffer_len = complexSymbolBufferLength(cyclic_prefix); //depends on cyclic prefix
    if (PRINT)
        cout << "frameAssemble - complexSymbolBufferLength exitted" << endl;

    // create frame generator
    ofdmflexframegen fg = DefineFrameGenerator(cyclic_prefix, phy_mode);
    if (PRINT)
    {
        cout << "frameAssemble - DefineFrameGenerator exitted, frame generator setted" << endl;
        ofdmflexframegen_print(fg);
    }

    // buffers
    liquid_float_complex c_buffer[c_buffer_len]; // time-domain buffer
    unsigned char header[8];                     // header data
    unsigned char payload[payload_len] = {};          // payload data
    if (PRINT)
        cout << "frameAssemble - header and payload buffers initialized" << endl;

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

    if (PRINT)
        cout << "frameAssemble - header and payload written" << endl;
    
    // cout << "frameAssemble - payload: " << payload << endl;  // prints as text
    if (PRINT)
    {
        cout << "frameAssemble - payload: ";                        // prints as numbers
        for (int i=0; i<payload_len; i++)
        {
            cout << unsigned(payload[i]) << " ";
        } 
        cout << endl << endl;
    }
    

    // assemble frame
    ofdmflexframegen_assemble(fg, header, payload, payload_len);
    if (PRINT)
    {
        cout << "frameAssemble - frame assembled" << endl;
        ofdmflexframegen_print(fg);
    }    

    int last_symbol = 0;
    int i = 0;
    int l = 0;

    while (!last_symbol)
    {
        // pthread_mutex_lock(&SDRmutex);
    
        // generate each OFDM symbol
        last_symbol = ofdmflexframegen_write(fg, c_buffer, c_buffer_len);
        if (PRINT)
        {
            cout << "frameAssemble - symbol " << l+1 << " written" << endl;
            cout << "frameAssemble - c_buffer[0]: " << c_buffer[0] << endl;
            cout << "frameAssemble - last_symbol value: " << last_symbol << endl;
        }
        
        
        if (!last_symbol)
        {
            if (PRINT)
                cout << "frameAssemble - starting complex to real buffer conversion" << endl;
            for (i = 0; i < c_buffer_len; i++)
            {
                r_frame_buffer[l*2*c_buffer_len+2*i]=c_buffer[i].real();
                r_frame_buffer[l*2*c_buffer_len+2*i+1]=c_buffer[i].imag();
            }
        }
        if (PRINT)
        {
            cout << "frameAssemble - r_frame_buffer[0]: " << r_frame_buffer[0] << endl;
            cout << "frameAssemble - exiting complex to real buffer conversion" << endl;
        }
        
        l++;
    
        // pthread_mutex_unlock(&SDRmutex);
    }
    ofdmflexframegen_destroy(fg);
}

// void *frameReception(void *threadID)
void frameReception(int cyclic_prefix)
{
    if (PRINT)
        cout << "frameReception - frameReception started" << endl;

    liquid_float_complex complex_i (0, 1);
    
    int c_sync_buffer_len = complexFrameBufferLength(cyclic_prefix); //synchronizer buffer can be  of arbitrary length 
    liquid_float_complex c_sync_buffer[c_sync_buffer_len];
    float r_sync_buffer[c_sync_buffer_len*2];
    if (PRINT)
        cout << "frameReception - buffers initialized" << endl;

    unsigned char allocation_array[SUBCARRIERS];    // subcarrier allocation array (null/pilot/data)
    subcarrierAllocation(allocation_array);
    if (PRINT)
        cout << "frameReception - subcarrierAllocation exited; allocation_array defined" << endl;

    unsigned int cp_len = (int)SUBCARRIERS / cyclic_prefix; // cyclic prefix length
    unsigned int taper_len = (int)cp_len / 4;          // taper length
  
    //ofdmflexframesync fs = ofdmflexframesync_create(SUBCARRIERS, cp_len, taper_len, allocation_array, callbackWhatsReceived, NULL);
    ofdmflexframesync fs = ofdmflexframesync_create(SUBCARRIERS, cp_len, taper_len, allocation_array, callbackBERCalculation, NULL);
    if (PRINT)
        cout << "frameReception - frame synchronizer created" << endl;

    // while(1)
    {
        //buffer handover
        for (int i = 0; i < c_sync_buffer_len*2; i++)
        {
            r_sync_buffer[i] = after_cahnnel_buffer[i];
        }
        if (PRINT)
        {
            cout << "frameReception - r_sync_buffer filled" << endl;
            cout << "frameReception - r_sync_buffer[0]: " << r_sync_buffer[0] << endl;
        }

        for (int i = 0; i < c_sync_buffer_len; i++)
        {
            c_sync_buffer[i]=r_sync_buffer[2*i]+r_sync_buffer[2*i+1] * complex_i.imag();
        }
        if (PRINT)
            cout << "frameReception - real buffer converted to complex buffer" << endl;

        // receive symbol (read samples from buffer)
        ofdmflexframesync_execute(fs, c_sync_buffer, c_sync_buffer_len);
        if (PRINT)
            cout << "frameReception - synchronization ended" << endl;

        if (!callback_invoked)
        {
            if (PRINT)
                cout << "frameReception - callback was not invoked" << endl;
            BER_log[global_cycl_pref_index][global_phy_mode][artificial_SNR] += 1;
        }
        
        callback_invoked = false;
        
        ofdmflexframesync_destroy(fs);
    }

}


void print_gpio(uint8_t gpio_val)
{
    for (int i = 0; i < 8; i++)
    {
        bool set = gpio_val & (0x01 << i);
        msgSDR << "GPIO" << i << ": " << (set ? "High" : "Low") << endl;
        Logger(msgSDR.str());
        msgSDR.str("");
    }
}

void setSampleRate(int cyclic_prefix)
{
    if (PRINT)
        cout << "setSampleRate - setSampleRate started" << endl;
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
    if (PRINT)
        cout << "setSampleRate - sample rate setted" << endl;
}

ofdmflexframegen DefineFrameGenerator (int dfg_cycl_pref, int dfg_PHYmode)
{
    if (PRINT)
        cout << "DefineFrameGenerator - DefineFrameGenerator started" << endl;
    
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
    if (PRINT)
        cout << "DefineFrameGenerator - subarrierAllocation exitted" << endl;
    
    return ofdmflexframegen_create(SUBCARRIERS, cp_len, taper_len, allocation_array, &fgprops);
}

void subcarrierAllocation (unsigned char *array)
{
    if (PRINT)
        cout << "subcarrierAllocation - subcarrierAllocation started" << endl;
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
    if (PRINT)
        cout << "frameSymbols - frameSymbols started" << endl;
    int symbolCnt;
    switch (cyclic_prefix)
    {
    case 4:
        symbolCnt = 22+4;
        break;
    case 8:
        symbolCnt = 24+4;
        break;
    case 16:
        symbolCnt = 26+4;
        break;
    case 32:
        symbolCnt = 27+4;
        break;
    default:
        symbolCnt = 0;
    }
    
    if (PRINT)
        cout << "frameSymbols - symbolCnt: " << symbolCnt << endl; 
    return symbolCnt;
}

uint complexFrameBufferLength(int cyclic_prefix)
{
    if (PRINT)
        cout << "complexFrameBufferLength - complexFrameBufferLength started" << endl;
    return complexSymbolBufferLength(cyclic_prefix)*frameSymbols(cyclic_prefix);
}

uint complexSymbolBufferLength(int cyclic_prefix)
{
    if (PRINT)
        cout << "complexSymbolBufferLength - complexSymbolBufferLength started" << endl;
    return (SUBCARRIERS + ((int)SUBCARRIERS / cyclic_prefix));
}

uint payloadLength(int cyclic_prefix, int phy_mode)
{
    if (PRINT)
        cout << "payloadLength - payloadLength started" << endl;
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
            bits_per_symbol = 2.0f * 2.0f / 3.0f;
            break;
        case 5:
            bits_per_symbol = 2.0f * 3.0f / 4.0f;
            break;
        case 6:
            bits_per_symbol = 2.0f * 5.0f / 6.0f;
            break;
        case 7:
            bits_per_symbol = 4.0f / 2.0f;
            break;
        case 8:
            bits_per_symbol = 4.0f * 2.0f / 3.0f;
            break;
        case 9:
            bits_per_symbol = 4.0f * 3.0f / 4.0f;
            break;
        case 10:
            bits_per_symbol = 4.0f * 5.0f / 6.0f;
            break;
        case 11:
            bits_per_symbol = 6.0f / 2.0f;
            break;
        case 12:
            bits_per_symbol = 6.0f * 2.0f / 3.0f;
            break;
        case 13:
            bits_per_symbol = 6.0f * 3.0f / 4.0f;
            break;
        case 14:
            bits_per_symbol = 6.0f * 5.0f / 6.0f;
            break;
        default:
            return 0;
    }

    return (uint)floor(DATACARRIERS * useful_symbols * bits_per_symbol / 8)-1;
}

// callback function
int callbackWhatsReceived(unsigned char *_header,
               int _header_valid,
               unsigned char *_payload,
               unsigned int _payload_len,
               int _payload_valid,
               framesyncstats_s _stats,
               void *_userdata)
{
    cout << endl;
    cout << "***** callback invoked!" << endl << endl;
    if (_header_valid)
    {
        cout << "  header valid" << endl;
    }
    else
    {
        cout << "  header invalid" << endl;
    }

    if (_payload_valid)
    {
        cout << "  payload valid" << endl; 
    }
    else
    {
        cout << "  payload invalid" << endl;
    }
    cout << endl;

    unsigned int i;
    if (_header_valid)
    {
        cout << "Received header: "<< endl;
        for (i = 0; i < 8; i++)
        {
            cout << _header[i];
        }
        cout << endl << endl;
    }

    if (_payload_valid)
    {
        cout << "Received payload: " << endl;
        for (i = 0; i < _payload_len; i++)
        {
            if (_payload[i] == 0)
                break;
            cout << _payload[i];
        }
        cout << endl << endl;
    }

    cout << "payload len: " << _payload_len << endl;
    cout << endl;

    return 0;
}

int callbackBERCalculation(unsigned char *_header,
               int _header_valid,
               unsigned char *_payload,
               unsigned int _payload_len,
               int _payload_valid,
               framesyncstats_s _stats,
               void *_userdata)
{
    if (PRINT)
        cout << "callbackBERCalculation invoked - _payload_len: "<< _payload_len << endl;

    callback_invoked = true;

    if (_payload_len != 0)
    {
        float BER = calculateBER(_payload_len, message, _payload);
        if (PRINT)
            cout << "callbackBERCalculation - calculateBER exitted; BER: "<< BER << endl;
        if (PRINT)
        {
            cout << "calculateBER exitted" << endl;
            cout << "BER: " << BER << endl;
        }
        BER_log[global_cycl_pref_index][global_phy_mode][artificial_SNR] += BER;
    }
    else
        BER_log[global_cycl_pref_index][global_phy_mode][artificial_SNR] += 1;

    return 0;
}

void artificialChannel(int cyclic_prefix)
{
    if (PRINT)
        cout << "artificialChannel - artificialChannel started" << endl;

    int channel_buffer_len = complexFrameBufferLength(cyclic_prefix); //synchronizer buffer can be  of arbitrary length 
    if (PRINT)
        cout << "artificialChannel - complexFrameBufferLength" << endl;
    
    liquid_float_complex complex_i(0, 1);

    // sample buffer
    liquid_float_complex buf_in[channel_buffer_len]; // complex input
    liquid_float_complex buf_out[channel_buffer_len]; // output buffer

    // real to complex buffer conversion
    for (int i = 0; i < channel_buffer_len; i++)
    {
        buf_in[i]=before_cahnnel_buffer[2*i]+before_cahnnel_buffer[2*i+1] * complex_i.imag();
    }
    if (PRINT)
        cout << "artificialChannel - real buffer converted to complex buffer" << endl;

    // create channel object
    channel_cccf channel = channel_cccf_create();

    // additive white Gauss noise impairment
    float noise_floor   = -60.0f;   // noise floor [dB]
    float SNRdB         =  (float)artificial_SNR;   // signal-to-noise ratio [dB]
    channel_cccf_add_awgn(channel, noise_floor, SNRdB);

    // carrier offset impairments
    float dphi          =   0.00f;  // carrier freq offset [radians/sample]
    float phi           =   0.0f;   // carrier phase offset [radians]
    channel_cccf_add_carrier_offset(channel, dphi, phi);

    // multipath channel impairments
    liquid_float_complex* hc   = NULL;     // defaults to random coefficients
    unsigned int hc_len = 4;        // number of channel coefficients
    channel_cccf_add_multipath(channel, hc, hc_len);

    // time-varying shadowing impairments (slow flat fading)
    float sigma         = 1.0f;     // standard deviation for log-normal shadowing
    float fd            = 0.1f;     // relative Doppler frequency
    channel_cccf_add_shadowing(channel, sigma, fd);

    // print channel internals
    if (PRINT)
        channel_cccf_print(channel);

    // fill buffer and repeat as necessary
    // apply channel to input signal
    channel_cccf_execute_block(channel, buf_in, channel_buffer_len, buf_out);
    
    if (PRINT)
        cout << "artificialChannel - channel executed" << endl;
    // destroy channel
    channel_cccf_destroy(channel);

    // complex to real buffer conversion
    for (int i = 0; i < channel_buffer_len; i++)
    {
        after_cahnnel_buffer[2*i]=buf_out[i].real();
        after_cahnnel_buffer[2*i+1]=buf_out[i].imag();
    }
    if (PRINT)
        cout << "artificialChannel - complex buffer converted to real buffer" << endl;
}

float calculateBER(unsigned int payload_len, string transmitted, unsigned char *received)
{
    unsigned int temp_payload_len = payload_len; //    strcpy((char *)u_ch_transmitted, transmitted.c_str()); makes diffilulties
    if (PRINT)
        cout << "calculateBER - calculateBER started; payload_len: "<< payload_len << endl; 
    
    float BER = 0;
    
    if (PRINT)
        cout << "calculateBER - 1st - payload_len: "<< payload_len<<"; temp_payload_len: "<< temp_payload_len << endl;

    unsigned char u_ch_transmitted[8100] = {};
    if (PRINT)
    {
        cout << "calculateBER - u_ch_transmitted initialized" << endl;
        cout << "calculateBER - 2nd - payload_len: "<< payload_len <<"; temp_payload_len: "<< temp_payload_len << endl;
    }

    strcpy((char *)u_ch_transmitted, transmitted.c_str());

    if (PRINT)
    {
        cout << "calculateBER - message copied" << endl;
        cout << "calculateBER - 3rd - payload_len: "<< payload_len<<"; temp_payload_len: "<< temp_payload_len << endl;
    }

    payload_len = temp_payload_len;
    
    if (PRINT)
    {
        cout << "calculateBER - 4th - payload_len: "<< payload_len <<"; temp_payload_len: "<< temp_payload_len<< endl;
        printByteByByte(payload_len, u_ch_transmitted, received);
        cout << "calculateBER - printByteByByte exitted" << endl;
    }

    for (int i=0; i<payload_len; i++)
    {
        for (int l=0; l<8; l++)
        {
            BER += ((u_ch_transmitted[i]^received[i])>>l)&1;
        }
    }    
    BER /= payload_len*8;
    
    msgSDR << "SNR: " << artificial_SNR << " BER: " << BER << endl;
    Logger(msgSDR.str());

    return BER;
}

void printByteByByte(unsigned int payload_len, unsigned char *transmitted, unsigned char *received)
{
    if (PRINT)
        cout << "printByteByByte - printByteByByte(payload_len = "<<payload_len<<") started" << endl;
        
    cout << "printByteByByte - transmitted: ";
    for (int i=0; i<payload_len; i++)
    {
        cout << unsigned(transmitted[i]) << " ";
    } 
    cout << endl << endl;


    cout << "printByteByByte - received: ";
    for (int i=0; i<payload_len; i++)
    {
        cout << unsigned(received[i]) << " ";
    }
    cout << endl << endl;
}

string alterMessage(int payload_len)
{
    string s = "";
    char random;
    for (int i = 0; i < payload_len; i++)
    {
        random = rand()%255+1;
        s = s + random;
    }
    return s;   
}

int exportBER()
{
    std::ofstream myfile;
    myfile.open ("./BER_simulation_outcome/BER_simulation_" + getCurrentDateTime("now") + ".csv");
    myfile << "Number of simulations: " << SIMULATION_REPETITION << endl;
    for (artificial_SNR = MIN_SNR-1; artificial_SNR<=MAX_SNR; artificial_SNR++) // SNR sweep
    {
        if (artificial_SNR == MIN_SNR-1)
            myfile << "SNR";
        else
            myfile << artificial_SNR;
        for (global_phy_mode = 1; global_phy_mode <=14; global_phy_mode++) // PHY mode
        {
            if (global_phy_mode == 2)
                global_phy_mode++;
            for (global_cycl_pref_index = 0; global_cycl_pref_index<4; global_cycl_pref_index++) // cyclic prefix
            {
                if (artificial_SNR == MIN_SNR-1)
                    myfile << ", PHY mode " << global_phy_mode << "; CP " << (4 << global_cycl_pref_index);
                else
                    myfile << "," << BER_log[global_cycl_pref_index][global_phy_mode][artificial_SNR];
            }
        }
        myfile << endl;
    }
    myfile.close();
    return 0;
}
