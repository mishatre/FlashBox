//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Log_Unit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
TLog Log;
// ---------------------------------------------------------------------------
__fastcall TLog::TLog() {
	Enabled  = false;
	LogPath = ExtractFilePath(Application->ExeName) + "log\\";
	FormatSettings.TimeSeparator = '.';
	FormatSettings.DateSeparator = '.';
	LogFileName = FormatDateTime("dd/mm/yy hh:mm:ss",
		TDateTime::CurrentDateTime(), FormatSettings) + ".log";
	MessageStr = new TStringStream();
	LogFile = NULL;
}

// ---------------------------------------------------------------------------
void __fastcall TLog::Msg(UnicodeString Message, UnicodeString ClassName) {
	if(Enabled) {
		String FinalMessage = TimeToStr(TTime::CurrentTime()) + " - ";
		FinalMessage += ClassName + ": " + Message + "\r\n";
		MessageStr->WriteString(FinalMessage);
		MessageStr->SaveToStream(LogFile);
		MessageStr->Clear();
	}
}

// ---------------------------------------------------------------------------
void __fastcall TLog::Msg(System::Sysutils::Exception &exception, UnicodeString ClassName) {
	if(Enabled) {
		String FinalMessage = "Exception!!! " + TimeToStr(TTime::CurrentTime()) + " - ";
		FinalMessage += ClassName + ": " + exception.Message + "\r\n";
		MessageStr->WriteString(FinalMessage);
		MessageStr->SaveToStream(LogFile);
		MessageStr->Clear();
	}
}

// ---------------------------------------------------------------------------
void __fastcall TLog::Msg(Rest::Exception::ERESTException &exception, UnicodeString ClassName) {
	if(Enabled) {
		String FinalMessage = "Rest Exception!!! " + TimeToStr(TTime::CurrentTime()) + " - ";
		FinalMessage += ClassName + ": " + exception.Message + "\r\n";
		MessageStr->WriteString(FinalMessage);
		MessageStr->SaveToStream(LogFile);
		MessageStr->Clear();
	}
}


// ---------------------------------------------------------------------------
void __fastcall TLog::EnableLog() {
	Enabled = true;
	CreateLogFile(LogPath + LogFileName);
}

// ---------------------------------------------------------------------------
void __fastcall TLog::DisableLog() {
	Enabled = false;
}

// ---------------------------------------------------------------------------
void __fastcall TLog::CreateLogFile(String FileName) {
	if(!DirectoryExists(LogPath))
		CreateDirectory(LogPath.c_str(), NULL);
	if(!FileExists(FileName)) {
		LogFile = new TFileStream(FileName, fmCreate);
    }

}

// ---------------------------------------------------------------------------
__fastcall TLog::~TLog() {
	if(LogFile != NULL)
		LogFile->Free();
	MessageStr->Free();
}
