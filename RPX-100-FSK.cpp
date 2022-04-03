/******************************************************************************
 * C++ source of RPX-100-FSK
 *
 * File:   RPX-100-FSK.cpp
 * Author: Bernhard Isemann
 *         Marek Honek
 *
 * Created on 20 Feb 2022, 10:35
 * Updated on 20 Feb 2022, 17:20
 * Version 2.00
 *****************************************************************************/

#include "RPX-100-FSK.h"
#include <pthread.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100-FSK with default settings:\n";
        cout << "Mode: TX6mPTT" << endl;
        cout << endl;
        cout << "type \033[36m'RF-100-FSK help'\033[0m to see all options !" << endl;
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
                else if (mode == "TXDirectPTT")
                {
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 5;
                }
                else if (mode == "TX6mPTT")
                {
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 6;
                }
                else if (mode == "TX2mPTT")
                {
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 7;
                }
                else if (mode == "TX70cmPTT")
                {
                    cout << "Starting RPX-100-FSK with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 8;
                }
                else if (mode == "help")
                {
                    cout << "Options for starting RPX-100-FSK: RF-100-FSK \033[36mMODE\033[0m" << endl;
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
                    cout << "Wrong settings, please type  \033[36m'RPX-100-FSK help'\033[0m to see all options !" << endl;
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

    // Initialize LimeSDR
    modeSel = 0;
    if (SDRinit(52.8e6, 4e6, modeSel, 1) != 0)
    {
        msg.str("");
        msg << "ERROR: " << LMS_GetLastErrorMessage();
        Logger(msg.str());
    }

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