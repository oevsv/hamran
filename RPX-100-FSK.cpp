/******************************************************************************
 * C++ source of RPX-100-FSK
 *
 * File:   RPX-100-FSK.cpp
 * Author: Bernhard Isemann
 *
 * Created on 08 May 2022, 10:35
 * Updated on 08 May 2022, 17:20
 * Version 2.00
 *****************************************************************************/

#include "RPX-100-FSK.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100 with default settings:\n";
        cout << "Mode: RX" << endl;
        cout << endl;
        cout << "type \033[36m'RPX-100-FSK help'\033[0m to see all options !" << endl;
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
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 0;
                }
                else if (mode == "TX")
                {
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 1;
                }
                else if (mode == "TXPRE")
                {
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 2;
                }
                else if (mode == "TXPA")
                {
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 3;
                }
                else if (mode == "help")
                {
                    cout << "Options for starting RPX-100: RPX-100-FSK \033[36mMODE\033[0m" << endl;
                    cout << endl;
                    cout << "\033[36mMODE\033[0m:" << endl;
                    cout << "     \033[32mRX\033[0m for receive mode" << endl;
                    cout << "     \033[31mTX\033[0m for transmit mode without bandpass filter" << endl;
                    cout << "     \033[31mTXPRE\033[0m for transmit mode with bandpass filter for 50-54 MHz" << endl;
                    cout << "     \033[31mTXPA\033[0m for transmit mode with bandpass filter for 144-146 MHz" << endl;
                    cout << endl;
                    return 0;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'RPX-100-Test help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;
            }
        }
    }

    LogInit();
    Logger("RPX-100-FSK was started succesfully with following settings:\n");
    msgSDR.str("");
    msgSDR << "Mode: " << mode << endl;
    Logger(msgSDR.str());

    // Initialize LimeSDR
    if (SDRinit(frequency, sampleRate, modeSel, normalizedGain) != 0)
    {
        msgSDR.str("");
        msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
        Logger(msgSDR.str());
    }

    // call SDRiniTX (RX)
    if (SDRset(frequency, sampleRate, 0, normalizedGain) != 0)
    {
        msgSDR.str("");
        msgSDR << "ERROR: " << LMS_GetLastErrorMessage();
        Logger(msgSDR.str());
    }

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&SDRmutex, 0);

    if (pthread_create(&threads[4], NULL, sendBeacon, (void *)4) != 0)
    {
        msgSDR.str("");
        msgSDR << "ERROR starting thread 4";
        Logger(msgSDR.str());
    }

    pthread_mutex_destroy(&SDRmutex);
    pthread_exit(NULL);
}