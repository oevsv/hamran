/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   RPX-100.cpp
 * Author: Bernhard Isemann
 *
 * Created on 01 Jan 2022, 10:35
 * Updated on 07 Jan 2022, 18:20
 * Version 1.00
 *****************************************************************************/

#include "RPX-100.h"
#include <pthread.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100 with default settings:\n";
        cout << "Mode: RX" << endl;
        cout << endl;
        cout << "type \033[36m'RF-test help'\033[0m to see all options !" << endl;
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
                    modeSel = 0;
                }
                else if (mode == "TXDirect")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 1;
                }
                else if (mode == "TX6m")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 2;
                }
                else if (mode == "TX2m")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 3;
                }
                else if (mode == "TX70cm")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 4;
                }
                else if (mode == "TXDirectPTT")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 5;
                }
                else if (mode == "TX6mPTT")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 6;
                }
                else if (mode == "TX2mPTT")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 7;
                }
                else if (mode == "TX70cmPTT")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSel = 8;
                }
                else if (mode == "help")
                {
                    cout << "Options for starting RPX-100: RF-test \033[36mMODE\033[0m" << endl;
                    cout << endl;
                    cout << "\033[36mMODE\033[0m:" << endl;
                    cout << "     \033[32mRX\033[0m for receive mode" << endl;
                    cout << "     \033[31mTXDirect\033[0m for transmit mode without bandpass filter" << endl;
                    cout << "     \033[31mTX6m\033[0m for transmit mode with bandpass filter for 50-54 MHz" << endl;
                    cout << "     \033[31mTX2m\033[0m for transmit mode with bandpass filter for 144-146 MHz" << endl;
                    cout << "     \033[31mTX70cm\033[0m for transmit mode with bandpass filter for 430-440 MHz" << endl;
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
                    cout << "Wrong settings, please type  \033[36m'RF-test help'\033[0m to see all options !" << endl;
                    return 0;
                }
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

    // Initialize LimeSDR
    if (SDRinit(52.8e6, 2e6, modeSel, 1) != 0)
    {
        msg.str("");
        msg << "ERROR: " << LMS_GetLastErrorMessage();
        Logger(msg.str());
    }

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&SDRmutex,0);

    // Start thread for SocketServer
    // if (pthread_create(&threads[1], NULL, startSocketServer, (void *)1) != 0)
    // {
    //     msg.str("");
    //     msg << "ERROR starting thread 1";
    //     Logger(msg.str());
    // }

    // sleep(1);

    // Start thread for WebSocket
    if (pthread_create(&threads[2], NULL, startWebSocket, (void *)2) != 0)
    {
        msg.str("");
        msg << "ERROR starting thread 2";
        Logger(msg.str());
    }
    
    // Start thread for SDR Stream
    if (pthread_create(&threads[3], NULL, startSDRStream, (void *)3) != 0)
    {
        msg.str("");
        msg << "ERROR starting thread 3";
        Logger(msg.str());
    }
    pthread_mutex_destroy(&SDRmutex);
    pthread_exit(NULL);
}