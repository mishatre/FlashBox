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
	String ConfFileName;

	void __fastcall LoadDefault();
	void __fastcall SaveToFile();

	public:

	String __fastcall Get(String Name);
	void __fastcall Set(String Name, String Value);



	__fastcall TConfFile();
	__fastcall ~TConfFile();
};

#endif
