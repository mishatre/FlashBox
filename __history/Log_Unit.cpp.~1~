// ---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Log_Unit.h"

// ---------------------------------------------------------------------------
#pragma package(smart_init)

TFormatSettings Log::FormatSettings =
	TFormatSettings::Create(LOCALE_SYSTEM_DEFAULT);
UnicodeString Log::_LogFileName = "";
UnicodeString Log::_LineFormat = "D T - M";
UnicodeString Log::_FilePath = ExtractFilePath(Application->ExeName) + "log\\";
TFileStream *Log::_LogFile;
TFileStream *Log::SeparateLogFile;
bool Log::Enabled = false;
bool Log::SeparateLog = false;
UnicodeString Log::SeparateLogPath = _FilePath + DateToStr(FormatDateTime("dd/mm/yyyy hh:mm:ss",
				TDateTime::CurrentDateTime(), FormatSettings)) + "\\";

__fastcall Log::Log() {
}

// ---------------------------------------------------------------------------
bool __fastcall Log::CreateLogFile() {
	try {
		if (Log::_LogFile == NULL) {
			Log::FormatSettings.TimeSeparator = '.';
			Log::_LogFileName = FormatDateTime("dd/mm/yyyy hh:mm:ss",
				TDateTime::CurrentDateTime(), FormatSettings);
			if (!DirectoryExists(_FilePath))
				CreateDirectory(_FilePath.w_str(), NULL);
			Log::_LogFile = new TFileStream(_FilePath + _LogFileName + ".log",
				fmCreate | fmShareCompat);
			return true;
		}
		else
			return false;
	}
	catch (Exception &e) {
		ShowMessage(e.Message);
	}
	return false;
}

// ---------------------------------------------------------------------------
void __fastcall Log::Msg(UnicodeString Message, UnicodeString ClassName,
	int Type) {
	if (Log::Enabled) {
		try {
			switch (Type) {
			case 1:
				Message = LogLineFormat(ClassName + ": " + Message) + "\r\n";
				break;
			case 2:
				Message = LogLineFormat(ClassName + ".Exception: " + Message)
					+ "\r\n";
				break;
			case 3:
				Message = LogLineFormat(ClassName + ".Error: " + Message)
					+ "\r\n";
				break;
			default:
				Message = LogLineFormat("System Message: " + Message) + "\r\n";
			}
			_LogFile->Write(Message.c_str(), Message.Length() * 2);
		}
		catch (Exception &e) {
			ShowMessage(e.Message);
		}
		if(Log::SeparateLog) {
			SeparateMsg(Message, ClassName, Type);
		}
	}
}
// ---------------------------------------------------------------------------

void __fastcall Log::SeparateMsg(UnicodeString Message, UnicodeString ClassName,
		int Type) {
	UnicodeString FileName = ClassName;
	if(FileName.LastDelimiter(":") != 0)
		FileName[FileName.LastDelimiter(":")] = '.';
	if(FileName.LastDelimiter(":") != 0)
		FileName[FileName.LastDelimiter(":")] = ' ';
	SeparateLogFile = new TFileStream(Log::SeparateLogPath + FileName + ".log", fmCreate | fmOpenWrite);
	try {
		switch (Type) {
		case 1:
			Message = LogLineFormat(ClassName + ": " + Message) + "\r\n";
			break;
		case 2:
			Message = LogLineFormat(ClassName + ".Exception: " + Message) + "\r\n";
			break;
		case 3:
			Message = LogLineFormat(ClassName + ".Error: " + Message) + "\r\n";
			break;
		default:
			Message = LogLineFormat("System Message: " + Message) + "\r\n";
		}
		SeparateLogFile->Write(Message.c_str(), Message.Length() * 2);
		SeparateLogFile->Free();
	}
	catch (Exception &e) {
		ShowMessage(e.Message);
	}
}
// ---------------------------------------------------------------------------
void __fastcall Log::Delimiter() {
	if (Log::Enabled) {
		UnicodeString Message;
		try {
			for (int i = 0; i < 75; i++)
				Message += "-";
			Message += "\r\n";
			_LogFile->Write(Message.c_str(), Message.Length() * 2);
		}
		catch (Exception &e) {
			ShowMessage(e.Message);
		}
	}
}

// ---------------------------------------------------------------------------
UnicodeString __fastcall Log::LogLineFormat(UnicodeString Message) {
	UnicodeString FinalMessage = "";
	for (int i = 1; i < _LineFormat.Length() + 1; i++) {
		switch (_LineFormat[i]) {
		case 'D':
			FinalMessage += DateToStr(TDate::CurrentDate());
			break;
		case 'T':
			FinalMessage += TimeToStr(TTime::CurrentTime());
			break;
		case 'M':
			FinalMessage += Message;
			break;
		case ' ':
			FinalMessage += " ";
			break;
		default:
			FinalMessage += _LineFormat[i];
			break;
		}
	}
	return FinalMessage;
}

// ---------------------------------------------------------------------------
bool __fastcall Log::StartLog() {
	Log::CreateLogFile();
	if (Log::Enabled == false)
		Log::Enabled = true;
	return true;
}

// ---------------------------------------------------------------------------
bool __fastcall Log::StopLog() {
	if (Log::Enabled == true)
		Log::Enabled = false;
	return true;
}
void __fastcall Log::EnableSeparateLog() {
	if(Log::SeparateLog == false) {
		if(!DirectoryExists(Log::SeparateLogPath))
			CreateDirectory(Log::SeparateLogPath.c_str(), NULL);
		Log::SeparateLog = true;
	}
}

