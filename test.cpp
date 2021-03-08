//
// Created by ztj on 2020/11/24.
//

#include <iostream>
#include <cmath>
#include "SerialPort/SerialPort.h"

using namespace std;

#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <list>


string saveFileName="data.log";


static string get_driver(const string& tty) {
    struct stat st;
    string devicedir = tty;

    // Append '/device' to the tty-path
    devicedir += "/device";

    // Stat the devicedir and handle it if it is a symlink
    if (lstat(devicedir.c_str(), &st)==0 && S_ISLNK(st.st_mode)) {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        // Append '/driver' and return basename of the target
        devicedir += "/driver";

        if (readlink(devicedir.c_str(), buffer, sizeof(buffer)) > 0)
            return basename(buffer);
    }
    return "";
}

static void register_comport( list<string>& comList, list<string>& comList8250, const string& dir) {
    // Get the driver the device is using
    string driver = get_driver(dir);

    // Skip devices without a driver
    if (driver.size() > 0) {
        string devfile = string("/dev/") + basename(dir.c_str());

        // Put serial8250-devices in a seperate list
        if (driver == "serial8250") {
            comList8250.push_back(devfile);
        } else
            comList.push_back(devfile);
    }
}

static void probe_serial8250_comports(list<string>& comList, list<string> comList8250) {
    struct serial_struct serinfo;
    list<string>::iterator it = comList8250.begin();

    // Iterate over all serial8250-devices
    while (it != comList8250.end()) {

        // Try to open the device
        int fd = open((*it).c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);

        if (fd >= 0) {
            // Get serial_info
            if (ioctl(fd, TIOCGSERIAL, &serinfo)==0) {
                // If device type is no PORT_UNKNOWN we accept the port
                if (serinfo.type != PORT_UNKNOWN)
                    comList.push_back(*it);
            }
            close(fd);
        }
        it ++;
    }
}


list<string> getComList() {
    int n;
    struct dirent **namelist;
    list<string> comList;
    list<string> comList8250;
    const char* sysdir = "/sys/class/tty/";

    // Scan through /sys/class/tty - it contains all tty-devices in the system
    n = scandir(sysdir, &namelist, NULL, NULL);
    if (n < 0)
        perror("scandir");
    else {
        while (n--) {
            if (strcmp(namelist[n]->d_name,"..") && strcmp(namelist[n]->d_name,".")) {

                // Construct full absolute file path
                string devicedir = sysdir;
                devicedir += namelist[n]->d_name;

                // Register the device
                register_comport(comList, comList8250, devicedir);
            }
            free(namelist[n]);
        }
        free(namelist);
    }

    // Only non-serial8250 has been added to comList without any further testing
    // serial8250-devices must be probe to check for validity
    probe_serial8250_comports(comList, comList8250);

    // Return the lsit of detected comports
    return comList;
}



int main() {

    //自动识别串口
    list<string> l = getComList();

    list<string>::iterator it = l.begin();

    std::string serial_port= "";
    std::string b = "ACM";
    string::size_type idx;

    int success_num = 0;

    while (it != l.end()) {
        cout << *it << endl;

        idx = it->find(b);
        if(idx != string::npos)
        {
            cout<<"found\n";
            success_num++;
            serial_port = *it;
        }
        it++;
    }

    if(success_num == 1)
    {
        std::cout<<"find serial port, OK!!!"<<std::endl;
        std::cout<<"serial_port == "<<serial_port<<std::endl;
    }

    if(success_num > 1)
    {
        std::cout<<"find more than one serial port,please choose check!!!"<<std::endl;
    }

    if(success_num < 1)
    {
        std::cout<<"not found right serial port,please check!!!"<<std::endl;
    }

    
    SerialPort UART;
    UART.Config.BaudRate = SerialPort::BR115200;
    UART.Config.DataBits = SerialPort::DataBits8;
    UART.Config.StopBits = SerialPort::StopBits1;
    UART.Config.Parity = SerialPort::ParityNone;
    //UART.Config.DevicePath = (char *) &"/dev/ttyACM0";
    UART.Config.DevicePath = const_cast<char*>(serial_port.c_str());

    if (UART.Open() == false)printf("OPEN error!\n");
    else printf("OPEN OK!\n");
    if (UART.LoadConfig() == false)printf("Set error!\n");
    else printf("Set OK!\n");

    //string RXtxt = "apg";

    //UWB需要两次输入enter，这里的13对应的是enter对应的char
    UART << 13;
    usleep(100);
    UART << 13;
    
    int cmdID = 0;
    while (1) {
        string scmd = "";
        cin >> scmd;
        if (scmd == "exit") {
            break;
        }
        std::cout<<"input == "<<scmd.c_str()<<std::endl;
        UART << scmd.c_str();

        UART << '\n';
        cmdID++;
        cout << "cmdID = " << cmdID << endl;
//        usleep(100);
    }

    cout << "finish run!" << endl;
    return 0;
}
