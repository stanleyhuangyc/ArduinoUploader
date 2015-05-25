#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define SUCCESS 0
#define ERROR_GENERIC -1
#define ERROR_ANALYZE_SKETCH -2
#define ERROR_BUILD_SKETCH -3
#define ERROR_BUILD_LIB -4
#define ERROR_LINK -5
#define ERROR_SIZE -6

#define SOURCE_OK 0
#define SOURCE_HEX 1
#define SOURCE_ERROR -1

#define BUILDER_VERSION "0.9.0"

#define BOARD_FLAG_1200BPS_RESET 1

typedef struct {
	const char* name;
	const char* mcu;
	const char* variant;
	int baudrate;
	const char* id;
	const char* defines;
	int progMem;
	int dataMem;
	int eepMem;
} BOARD_CONFIG;

typedef enum {
	STATE_IDLE = 0,
	STATE_BUILDING,
	STATE_UPLOADING,
	STATE_PROGRAMMING,
} STATE;

class CArduinoBuilder {
public:
	CArduinoBuilder():board(0),progress(0),state(STATE_IDLE),baudrate(0),startTime(0),corefile(0),libfile(0)
	{
		memset(&proc, 0, sizeof(proc));
		memset(&target, 0, sizeof(target));
#ifdef WIN32
		workDir = getenv("TEMP");
#else
		workDir = "/var/tmp";
#endif
	}
	int BuildSketch();
	int UploadToArduino(const char* eepfile, const char* serial, int baudrate, const char* avrpath);
	std::string GetBinaryInfo(const char* elfpath);
	bool ConvertELFtoHEX(const char* elfpath);
	BOARD_CONFIG* board;
	SHELL_PARAM proc;
	char* hexfile; /* output hex file path*/
	const char* workDir; /* work path */
	const char* buildDir;
	char* corefile;
	char* libfile;
	int freq; /* operating frequency */
	int baudrate;
	const char* sketchPath;	/* main sketch path */
	// binary info
	struct {
		int progBytes;
		int dataBytes;
		int eepBytes;
	} target;
	bool WaitProcess(DWORD timeout)
	{
		return ShellWait(&proc, timeout) != 0;
	}
	int ReadProcessConsole();
	virtual void ConsoleOutput(const char* text, const char* s = 0);
	virtual void ShowProgress(int percentage);
	int progress;
	STATE state;
	DWORD startTime;
private:
	int ScanSourceCode(const char* dir);
	int GenSourceFiles(const char* sketch, const char* srcpath);
	bool ParseSketch(const char* sketch);
	std::string GetFileName(const char* path);
	std::string GetFileNameNoExt(const char* path);
	std::string GetCompileOpts();
	const char* GetCompiler(const char* filename);
	std::vector<std::string> objs;
	std::vector<std::string> cores;
	std::vector<std::string> libs;
	std::vector<std::string> syslibs;
	std::vector<std::string> sources;
};

#ifdef __cplusplus
extern "C" {
#endif
extern BOARD_CONFIG boards[];

#ifdef __cplusplus
}
#endif
