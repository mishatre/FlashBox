//---------------------------------------------------------------------------

#ifndef Log_UnitH
#define Log_UnitH
#define FuncName __FUNC__
//---------------------------------------------------------------------------
#include <REST.Exception.hpp>

class TLog {
	private:
	String LogPath;
	String LogFileName;
	TFileStream *LogFile;
	TStringStream *MessageStr;
	TFormatSettings FormatSettings;
	bool Enabled;

	void __fastcall CreateLogFile(String FileName);
	String __fastcall FormatMessage(UnicodeString Message, UnicodeString ClassName, int Type = 0);

	public:

	void __fastcall Msg(UnicodeString Message, UnicodeString ClassName);
	void __fastcall Msg(System::Sysutils::Exception &exception, UnicodeString ClassName);
	void __fastcall Msg(Rest::Exception::ERESTException &exception, UnicodeString ClassName);

	void __fastcall EnableLog();
	void __fastcall DisableLog();
	__fastcall TLog();
	__fastcall ~TLog();

};

extern TLog Log;
#endif
