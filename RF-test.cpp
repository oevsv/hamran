/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   RF-test.cpp
 * Author: Bernhard Isemann
 *
 * Created on 24 Sep 2021, 11:35
 * Updated on 24 Sep 2021, 15:18
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
//#include <wiringPi.h>
//#include <wiringSerial.h>
#include "lime/LimeSuite.h"
#include <chrono>
#include <math.h>
//#include "liquid/liquid.h"

using namespace std;
lms_device_t *device = NULL;
std::stringstream msg;
std::stringstream HEXmsg;

uint8_t setRX = 0x08;       //all other bit = 0 --> 6m
uint8_t setTXDirect= 0x07;  //all other bit = 0 --> direct path without BP
uint8_t setTX6m = 0x03;     //all other bit = 0 --> 6m with BP
uint8_t setTX2m = 0x01;     //all other bit = 0 --> 2m with BP
uint8_t setTX70cm = 0x05;   //all other bit = 0 --> 70cm with BP

int error();
void print_gpio(uint8_t gpio_val);

string mode = "RX";
int modeSelector = 0;

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100 with default settings:\n";
        cout << "Mode: " << mode << endl;
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
                    modeSelector = 0;
                }
                else if (mode == "TXDirect")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 1;
                }
                else if (mode == "TX6m")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 2;
                }
                else if (mode == "TX2m")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 3;
                }
                else if (mode == "TX70cm")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 4;
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

    // if (wiringPiSetup() == -1) /* initializes wiringPi setup */
    // {
    //     msg.str("");
    //     msg << "Unable to start wiringPi: " << strerror(errno);
    //     return 1;
    // }

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
        if (LMS_GPIOWrite(device, &setTXDirect, 1) != 0)
        {
            error();
        }
        break;

    case 2:
        if (LMS_GPIOWrite(device, &setTX2m, 1) != 0)
        {
            error();
        }
        break;

    case 3:
        if (LMS_GPIOWrite(device, &setTX6m, 1) != 0)
        {
            error();
        }
        break;

    case 4:
        if (LMS_GPIOWrite(device, &setTX70cm, 1) != 0)
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
