/******************************************************************************
 * C++ source of RPX-100-transciever
 *
 * File:   RPX-100-TX.cpp
 * Author: Bernhard Isemann
 *         Marek Honek
 *
 * Created on 21 Jul 2022, 16:20
 *
 * Predecessor  RPX-100-synchronizer.cpp
 *****************************************************************************/

#include "RPX-100-transciever.h"

using namespace std;


int main(int argc, char *argv[])
{
    if (PRINT)
        cout << "main - program started" << endl;

    int modeSel = RX_MODE; //RX as default
    int cycl_prefix = 4;
    int phy_mode = 1;

    if (argc == 1)
    {
        cout << "Starting RPX-100-transciever with default settings:\n";
        cout << "Mode: RX" << endl;
        cout << "Cyclic prefix: 1/4" << endl;
        cout << endl;
        cout << "type \033[36m'RPX-100-transciever help'\033[0m to see all options!" << endl;
    }
    else if (argc >= 2)
    {
        for (int c = 1; c < argc; c++)
        {
            switch (c)
            {
            case 1:
                mode = (string)argv[c];
                if (mode == "RX")
                {
                    cout << "Starting RPX-100-transciever with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 0;
                }
                else if (mode == "TX6mPTT")
                {
                    cout << "Starting RPX-100-transciever with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 6;
                }
                else if (mode == "help")
                {
                    cout << "Options for starting RPX-100-transciever \033[36mMODE\033[0m" << endl;
                    cout << endl;
                    cout << "\033[36mMODE\033[0m:" << endl;
                    cout << "     \033[32mRX\033[0m for receive mode" << endl;
                    cout << endl;
                    cout << "     \033[31mTX6mPTT\033[0m for transmit mode with PTT with bandpass filter for 50-54 MHz" << endl;
                    cout << endl;
                    cout << "\033[36mCYCLIC PREFIX\033[0m:" << endl;
                    cout << "     \033[32m4, 8, 16 or 32\033[0m for 1/n cyclic prefix" << endl;
                    cout << endl;
                    cout << "\033[36mPHY MODE\033[0m:" << endl;
                    cout << "     \033[32mNumber 1 to 14\033[0m for PHY mode (applies only for TX mode)" << endl;
                    cout << endl;
                    cout << "\033[36mMESSAGE\033[0m:" << endl;
                    cout << "     \033[32mString to be transmitted\033[0m (applies only for TX mode)" << endl;
                    cout << endl;
                    return 0;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'RPX-100-transciever help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;
            case 2:
                cycl_prefix = stoi((string)argv[c]);
                if ((cycl_prefix == 4) || (cycl_prefix == 8) || (cycl_prefix == 16) || (cycl_prefix == 32))
                {
                    cout << "     cyclic prefix: " << cycl_prefix << endl;
                    cout << endl;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'RPX-100-transciever help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;
            case 3:
                phy_mode  = stoi((string)argv[c]);
                if (phy_mode == 1 || (phy_mode > 2 && phy_mode < 15))
                {
                    cout << "     PHY mode: " << phy_mode << endl;
                    cout << endl;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'RPX-100-transciever help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;
            case 4:
                message = (string)argv[c];
                break;
            }
        }
    }

    LogInit();
    if (PRINT)
        cout << "main - logger initalized" << endl;

    Logger("RPX-100-synchronizer was started.\n");
    msgSDR << "Mode: " << modeSel << endl;
    Logger(msgSDR.str());
    msgSDR.str("");

        
    if (PRINT)
        cout << "main - first log message saved" << endl;

    setSampleRate(cycl_prefix);

    SDRinit(TX_6m_MODE);

    if (modeSel == TX_6m_MODE)
    {
        sendFrame(cycl_prefix, phy_mode);
        if (PRINT)
            cout <<"main - sendFrame exitted" << endl;
    }

    SDRset(frequency, RX_MODE, normalizedGain);

    if (modeSel == RX_MODE)
    {
        frameReception(cycl_prefix);
        if (PRINT)
            cout <<"main - frameReception exitted" << endl;
    }
    
    return(0);    
}    
    

void sendFrame(int cyclic_prefix, int phy_mode)
{
    if (PRINT)
        cout << "sendFrame - sendFrame started - cyclic_prefix: " << cyclic_prefix << "; phy_mode: " << phy_mode << endl;
 
    int tx_buffer_len = 2*complexFrameBufferLength(cyclic_prefix);
    if (PRINT)
        cout << "sendFrame - complexFrameBufferLength exitted - buffer_len: " << tx_buffer_len << endl;

    int tx_buffer[tx_buffer_len];  //buffer for whole frame
    if (PRINT)
        cout << "sendFrame - buffer initialized" << endl;

    frameAssemble(tx_buffer, cyclic_prefix, phy_mode);
    if (PRINT)
    {
        cout << "sendFrame - frameAssemble exitted" << endl;
        cout << "sendFrame - tx_buffer[0]: " << tx_buffer[0] << endl;
    }

    if (PRINT)
        cout << "sendFrame - setSampleRate exitted" << endl;

    // call SDRinit (TX6mPTT)
    if (SDRset(frequency, TX_6m_MODE, normalizedGain) != 0)
    {
        msgSDR.str("");
        msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
        Logger(msgSDR.str());
    }
    
    if (PRINT)
        cout << "sendFrame - SDR has been set" << endl;


    startSDRTXStream(tx_buffer, complexFrameBufferLength(cyclic_prefix));


    if (PRINT)
    {
        cout << "sendFrame - frame in tx_buffer has been transmitted" << endl;
    }

    // call SDRini (RX)
    if (SDRset(frequency, RX_MODE, normalizedGain) != 0)
    {
        msgSDR.str("");
        msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
        Logger(msgSDR.str());
    }
}

void frameAssemble(int *r_frame_buffer, int cyclic_prefix, int phy_mode)
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
    {    
        cout << "frameAssemble - header and payload written" << endl;
        cout << "frameAssemble - payload: " << payload << endl;  // prints as text
        cout << "frameAssemble - payload: ";                     // prints as numbers
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

    // Initialize stream
    lms_stream_t streamId;                        //stream structure
    streamId.channel = 0;                         //channel number
    streamId.fifoSize = 1024 * 1024;              //fifo size in samples
    streamId.throughputVsLatency = 1.0;           //optimize for max throughput
    streamId.isTx = false;                        //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32; //12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    
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
  
    ofdmflexframesync fs = ofdmflexframesync_create(SUBCARRIERS, cp_len, taper_len, allocation_array, callbackWhatsReceived, NULL);
    //ofdmflexframesync fs = ofdmflexframesync_create(SUBCARRIERS, cp_len, taper_len, allocation_array, callbackBERCalculation, NULL);
    if (PRINT)
        cout << "frameReception - frame synchronizer created" << endl;

    // Start streaming
    LMS_StartStream(&streamId);

    while(rxON)
    {
        //Receive samples
        LMS_RecvStream(&streamId, r_sync_buffer, c_sync_buffer_len, NULL, 1000); //should work, for now replaced by tx_buffer
        //I and Q samples are interleaved in r_sync_buffer: IQIQIQ...

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
    }
    ofdmflexframesync_destroy(fs);

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
        fgprops.fec1 = LIQUID_FEC_REP3;
        fgprops.fec0 = LIQUID_FEC_CONV_V27;
        fgprops.mod_scheme = LIQUID_MODEM_QPSK;
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
    uint8_t useful_symbols; // number of OFDM symbols carrying payload
    float coding_rate;  // uncoded to coded ratio

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
            coding_rate = 1;
            break;
        case 2:
            coding_rate = 2.0 / (2.0 * 3.0f);   // ****** Je bits per sybmlol 
                                                    // ******lokální proměnná? Je ten výpočet správně?
            break;
        case 3:
            coding_rate = 2.0f / 2.0f;
            break;
        case 4:
            coding_rate = 2.0f * 2.0f / 3.0f;
            break;
        case 5:
            coding_rate = 2.0f * 3.0f / 4.0f;
            break;
        case 6:
            coding_rate = 2.0f * 5.0f / 6.0f;
            break;
        case 7:
            coding_rate = 4.0f / 2.0f;
            break;
        case 8:
            coding_rate = 4.0f * 2.0f / 3.0f;
            break;
        case 9:
            coding_rate = 4.0f * 3.0f / 4.0f;
            break;
        case 10:
            coding_rate = 4.0f * 5.0f / 6.0f;
            break;
        case 11:
            coding_rate = 6.0f / 2.0f;
            break;
        case 12:
            coding_rate = 6.0f * 2.0f / 3.0f;
            break;
        case 13:
            coding_rate = 6.0f * 3.0f / 4.0f;
            break;
        case 14:
            coding_rate = 6.0f * 5.0f / 6.0f;
            break;
        default:
            return 0;
    }

    return (uint)floor(DATACARRIERS * useful_symbols * coding_rate / 8)-1; // Without the -1, frame generator produces excess symbol
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


int startSDRTXStream(int *tx_buffer, int FrameSampleCnt)
{
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

    
    int ret = LMS_SendStream(&streamId, tx_buffer, FrameSampleCnt, nullptr, 1000);


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

    sleep(1);

    return 0;
}

int SDRinit(int modeSelector)
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


    // Enable RX or TX channel,Channels are numbered starting at 0
    if (modeSelector == RX_MODE)
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
    if (modeSelector == RX_MODE)
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
    if (modeSelector == RX_MODE)
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


    // calibrate Tx, continue on failure
    if (modeSelector == RX_MODE)
    {
        LMS_Calibrate(device, LMS_CH_TX, 0, sampleRate, 0);
    }
    else
    {
        LMS_Calibrate(device, LMS_CH_RX, 0, sampleRate, 0);
    }

    sleep(2);

    return 0;
}

int SDRset(double frequency, int modeSelector, double normalizedGain)
{
    // Set SDR GPIO diretion GPIO0-5 to output and GPIO6-7 to input
    uint8_t gpio_dir = 0xFF;
    if (LMS_GPIODirWrite(device, &gpio_dir, 1) != 0)
    {
        error();
    }

    // Set GPIOs to RX/TX mode (initial settings)
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


    // Set sample rate
    if (LMS_SetSampleRate(device, sampleRate, 0) != 0)
    {
        error();
    }
    msgSDR.str("");
    msgSDR << "Sample rate: " << sampleRate / 1e6 << " MHz" << endl;
    Logger(msgSDR.str());

    // Set center frequency
    if (modeSelector == RX_MODE)
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
    if (modeSelector == RX_MODE)
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

    sleep(1); //time for PA to settle

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
