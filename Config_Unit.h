//---------------------------------------------------------------------------

#ifndef Config_UnitH
#define Config_UnitH

#include <vcl.h>
#include <System.JSON.hpp>
//---------------------------------------------------------------------------


class TConfFile {
	private:
	TJSONObject *Data;
	TFileStream *ConfFile;
	TStringStream *JSONString;
	UnicodeString ConfFileName;

	void __fastcall LoadDefault();
	void __fastcall SaveToFile();

	public:

	UnicodeString __fastcall Get(UnicodeString Name);
	void __fastcall Set(UnicodeString Name, UnicodeString Value);



	__fastcall TConfFile();
	__fastcall ~TConfFile();
};

#endif
