#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <direct.h>
extern "C" {
#include "syspil.h"
#include "processpil.h"
}
#include "sketchbuild.h"

using namespace std;

char* appdir;

int main(int argc, char* argv[])
{
    char* filepath;
    const char* serial = 0;
	char *p;
	int ret = 0;
	int boardIndex = 0;
	int ispBaudrate = 0;
	char* corefile = 0;
	char* libfile = 0;
	BOARD_CONFIG* board = boards;
	CArduinoBuilder builder;

	cout << "Arduino Compiler & Uploader Version " << BUILDER_VERSION << endl << "(C)2013 Developed by Stanley Huang <stanleyhuangyc@gmail.com>" << endl << "Distributed under GPL license" << endl << endl;
    if (argc < 2) {
		int i;
		cout << "Command line syntax:" << endl << argv[0] << " [sketch/hex file] [board type] [serial port/usbasp] [MCU frequency in MHz]" << endl << endl
			<< "Board types:" << endl;
		for (i = 0; boards[i].name; i++) {
			cout << (i + 1) << " - " << boards[i].name << endl;
		}
		cout << endl;
        return -1;
    }

    // get app dir
	appdir = strdup(argv[0]);
	p = strrchr(appdir, '/');
	if (!p) p = strrchr(appdir, '\\');
	if (p)
        *(p + 1) = 0;
    else
        *appdir = 0;
	SetCurrentDirectory(appdir);

#ifdef WIN32
	// set toolchain path
	char* pathenv = new char[4096];
	GetEnvironmentVariable("PATH", pathenv, 4096 - strlen(appdir) - 16);
	strcat(pathenv, ";");
	strcat(pathenv, appdir);
	strcat(pathenv, "bin");
	SetEnvironmentVariable("PATH", pathenv);
	delete pathenv;
#endif

    filepath = argv[1];
	p = filepath + strlen(filepath) - 1;
	if (*p == '.') *(p--) = 0;
	if (*p == '/' || *p == '\\') *p = 0;
	if (IsDir(filepath)) {
		// building core and library mode
		builder.hexfile = 0;
	} else {
		builder.hexfile = new char[strlen(filepath) + 5];
		sprintf(builder.hexfile, "%s.hex", filepath);
	}

	if (argc > 2) {
		int boardCount;
		for (boardCount = 0; boards[boardCount].name; boardCount++);
		if (isdigit(argv[2][0])) {
			boardIndex = atoi(argv[2]);
			boardIndex--;
		} else {
			for (boardIndex = 0; boardIndex < boardCount; boardIndex++) {
				if (strstr(boards[boardIndex].id, argv[2])) break;
			}
		}
		if (boardIndex < 0 || boardIndex >= boardCount) {
			cout << "No valid board selected." << endl;
			return -2;
		}
	}
	board = boards + boardIndex;

	if (argc > 3) {
	    serial = argv[3];
		if (!strcmp(serial, "-")) serial = 0;
	}

	if (argc > 4)
	    builder.freq =atoi(argv[4]);
	else
		builder.freq = 16;

	if (argc > 5)
	    ispBaudrate = atoi(argv[5]);

	if (argc > 6) libfile = argv[6];
	if (argc > 7) corefile = argv[7];

	if (libfile) {
		// create library file dir
		char *s = _strdup(libfile);
		char *p = strrchr(s, '/');
		if (!p) p = strrchr(s, '\\');
		if (p) {
			*p = 0;
			if (!IsDir(s)) _mkdir(s);
		}
		free(s);
	}

	// initialize builder
	if (boardIndex >= 0) {
		builder.board = boards + boardIndex;
	}

	if (!strstr(filepath, ".hex") && !strstr(filepath, ".HEX")) {
		char* buildDir = new char [strlen(filepath) + 1];

		if (builder.hexfile) {
			strcpy(buildDir, filepath);
			p = strrchr(buildDir, '/');
			if (!p) p = strrchr(buildDir, '\\');
			if (p) {
				*p = 0;
			} else {
				strcpy(buildDir, ".");
			}
			remove(builder.hexfile);
		} else {
			strcpy(buildDir, filepath);
		}

		if (strstr(filepath, ".elf")) {
			builder.proc.flags = SF_REDIRECT_STDOUT;
			if (!builder.ConvertELFtoHEX(filepath)) {
				builder.ConsoleOutput("Error generating HEX file");
				ret = ERROR_SIZE;
			}
		} else {
			builder.ConsoleOutput("Build Target: %s ", builder.board->name);
			builder.ConsoleOutput("(MCU: %s)\r\n", builder.board->mcu);
			
			builder.sketchPath = filepath;
			builder.buildDir = buildDir;
			if (corefile) {
				builder.corefile = corefile;
			} else if (IsDir("cores")) {
				builder.corefile = new char[128];
				sprintf(builder.corefile, "cores/core_%s_%d.a", builder.board->id, builder.freq);
			} else {
				builder.corefile = 0;
			}
			builder.libfile = libfile;

			ret = builder.BuildSketch();
			builder.proc.iRetCode = ret;

			if (builder.hexfile && !IsFileExist(builder.hexfile)) {
				cout << endl << "Error occurred during compiliation" << endl;
				if (ret == 0) ret = -1;
			}
			cout << endl;
		}
		if (ret == SUCCESS && serial) {
			char* eepfile = 0;
			builder.ConsoleOutput("Starting upload for %s ", builder.board->name);
			builder.ConsoleOutput("via %s...\r\n", serial);
			builder.proc.flags = SF_REDIRECT_STDERR;
			if (builder.target.eepBytes > 0) {
				eepfile = new char[strlen(builder.hexfile) + 5];
				sprintf(eepfile, "%s.eep", builder.hexfile);
			}
			builder.proc.flags = SF_ALLOC | SF_REDIRECT_STDERR;
			ret = builder.UploadToArduino(eepfile, serial, ispBaudrate, "bin/");
			if (ret == 0) {
				do {
					while (ShellRead(&builder.proc, 3000) > 0) cout << builder.proc.buffer;
				} while (ShellWait(&builder.proc, 0) == 0);
				ret = builder.proc.iRetCode;
			}
			ShellClean(&builder.proc);
			if (eepfile) delete eepfile;
		}

		delete buildDir;
		if (!corefile && builder.corefile) delete builder.corefile;
	} else if (serial) {
		ret = builder.UploadToArduino(0, serial, ispBaudrate, "bin/");
	}
	if (builder.hexfile) free(builder.hexfile);
	free(appdir);

#ifdef _DEBUG
	printf("Press ENTER to exit...");
	getchar();
#endif
    return ret;
}
