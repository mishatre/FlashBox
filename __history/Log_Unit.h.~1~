// ---------------------------------------------------------------------------

#ifndef LogUnitH
#define LogUnitH
#include <System.Classes.hpp>
#include <typeinfo>

#define CName typeid(this).name()
#define FuncName __FUNC__

class Log {
private:
	static TFormatSettings FormatSettings;

	static UnicodeString _LogFileName;
	static UnicodeString _LineFormat;
	static UnicodeString _FilePath;
	static TFileStream *_LogFile;

	static TFileStream *SeparateLogFile;
	static UnicodeString SeparateLogPath;

	static bool Enabled;
	static bool SeparateLog;

	// static UnicodeString __fastcall getLineFormat();
	// static void __fastcall setLineFormat(UnicodeString LineFormat);
	static UnicodeString __fastcall LogLineFormat(UnicodeString Message);

	// __property String LineFormat = {read = getLineFormat, write = setLineFormat};
	public :

	__fastcall Log();
	static bool __fastcall CreateLogFile();
	static void __fastcall SaveLogFile();
	static void __fastcall Msg(UnicodeString Message, UnicodeString ClassName,
		int Type = 1);
	static void __fastcall SeparateMsg(UnicodeString Message, UnicodeString ClassName,
		int Type = 1);
	static void __fastcall Delimiter();
	// 1 - System Message
	// 2 - Exceptions
	// 3 - Errors
	//
	static bool __fastcall StartLog();
	static bool __fastcall StopLog();
	static void __fastcall EnableSeparateLog();
};
// ---------------------------------------------------------------------------
#endif

