#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include "ctb-0.16/ctb.h"
extern "C" {
#include "syspil.h"
#include "processpil.h"
}
#include "sketchbuild.h"

using namespace std;
vector<string> ports;

static string SearchNewPort()
{
	DWORD tick = GetTickCount();
	vector<string> newports;

	bool found = true;
	do {
		newports.clear();
		ctb::GetAvailablePorts( newports );
		for( int i = 0; i < (int)newports.size(); i++) {
			// look for port in previous ports list
			found = false;
			for (int j = 0; j < ports.size(); j++) {
				if (newports[i].compare(ports[j]) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				return newports[i];
			}
		}
		newports.clear();
		msleep(1000);
	} while (GetTickCount() - tick < 10000 && found);
	return "";
}

static string ResetLeonardo(const char* port)
{
	string uploadPort;
	ctb::SerialPort serialPort;
	char devname[64];
	sprintf(devname, "\\\\.\\%s", port);
	if (serialPort.Open(devname, 1200) != 0) {
		return "";
	}
	//serialPort.ClrLineState(ctb::LinestateDtr);
	serialPort.Close();

	uploadPort = SearchNewPort();
	return uploadPort;
}

void ResetBoard(const char* port)
{
	ctb::SerialPort serialPort;
	char devname[64];
	sprintf(devname, "\\\\.\\%s", port);
	if (serialPort.Open(devname) == 0) {
		serialPort.SetLineState(ctb::LinestateDtr);
		Sleep(10);
		serialPort.SetLineState(ctb::LinestateNull);
	}
}

int CArduinoBuilder::UploadToArduino(const char* eepfile, const char* serial, int ispBaudrate, const char* avrpath)
{
    char cmd[512];
	char *p;
	int ret = 0;

	ports.clear();
	progress = 0;
	do {
		string uploadPort;
		if (!IsFileExist(hexfile)) {
			ConsoleOutput("%s does not exist", hexfile);
			break;
		}
		if (!serial) {
			ConsoleOutput("No serial port is specified.\r\n");
			break;
		}
		// enumerate serial ports
		if( !ctb::GetAvailablePorts( ports )) {
			ConsoleOutput("No serial port is accessible.\r\n");
			break;
		}
		if (!strcmp(board->mcu, "atmega32u4") && stricmp(serial, "usbasp") && !baudrate) {
			ConsoleOutput("Forcing reset using 1200bps open/close on %s... ", serial);
			uploadPort = ResetLeonardo(serial);
			if (!uploadPort.empty()) {
				ConsoleOutput("%s found!\r\n", uploadPort.c_str());
			} else {
				ConsoleOutput("no new port found\r\n");
				proc.iRetCode = -1;
				break;
			}
		}
		p = cmd;
#ifdef WIN32
		p += sprintf(p, "\"%savrdude.exe\" -C %savrdude.conf", avrpath, avrpath);
		if (!stricmp(serial, "usbasp")) {
			p += sprintf(p, " -F -cusbasp -Pusb");
		} else if (ispBaudrate) {
			// ArduinoISP
			p += sprintf(p, " -cstk500v1 -P\\\\.\\%s -b%d", serial, ispBaudrate);
		} else if (!strcmp(board->mcu, "atmega32u4")) {
			p += sprintf(p, " -cavr109 -P\\\\.\\%s -b%d -D", uploadPort.c_str(), board->baudrate);
		} else if (!strcmp(board->mcu, "atmega2560")) {
			p += sprintf(p, " -cwiring -P\\\\.\\%s -b%d -D", serial, board->baudrate);
		} else {
			p += sprintf(p, " -carduino -P\\\\.\\%s -b%d -D", serial, board->baudrate);
		}
#else
		p += sprintf(p, "%savrdude -C /etc/avrdude.conf", avrpath);
		if (!strcmp(serial, "usbasp")) {
			p += sprintf(p, " -F -cusbasp -Pusb");
		} else if (!strcmp(board->mcu, "atmega32u4")) {
			p += sprintf(p, " -cavr109 -P%s -b%d -D", uploadPort.c_str(), board->baudrate);
		} else if (!strcmp(board->mcu, "atmega2560")) {
			p += sprintf(p, " -cwiring -P%s -b%d -D", serial, board->baudrate);
		} else {
			p += sprintf(p, " -carduino -P%s -b%d -D", serial, board->baudrate);
		}
#endif
		p += sprintf(p, " -V -p%s -Uflash:w:\"%s\":i", board->mcu, hexfile);
		if (eepfile) {
			p += sprintf(p, " -Ueeprom:w:\"%s\":i", eepfile);
		}
		if (!stricmp(serial, "usbasp")) {
			//p += sprintf(p, " -U lock:w:0xe8:m");
		}
		if (ShellExec(&proc, cmd) != 0) {
			break;
		}
		//ResetLeonardo(serial.c_str());
		return 0;
	} while(0);
    return -1;
}
