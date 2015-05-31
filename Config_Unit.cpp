//---------------------------------------------------------------------------

#pragma hdrstop

#include "Config_Unit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

// ---------------------------------------------------------------------------
__fastcall TConfFile::TConfFile() {
	ConfFileName = ExtractFilePath(Application->ExeName) + "ConfFile.cfg";
	if(FileExists(ConfFileName)) {
		JSONString = new TStringStream();
		ConfFile = new TFileStream(ConfFileName, fmOpenRead);
		JSONString->LoadFromStream(ConfFile);
		Data = (TJSONObject*) TJSONObject::ParseJSONValue(JSONString->DataString);
		JSONString->Free();
		ConfFile->Free();
	}
	if(Data == NULL)
		LoadDefault();
}

// ---------------------------------------------------------------------------
String __fastcall TConfFile::Get(String Name) {
	if(Data != NULL) {
		for(int i = 0; i < Data->Count; i++) {
			if(Data->Pairs[i]->JsonString->Value() == Name)
				return Data->Pairs[i]->JsonValue->Value();
		}
	}
	return "";
}

// ---------------------------------------------------------------------------
void __fastcall TConfFile::Set(String Name, String Value) {
	if(Data != NULL) {
		if(Name == "")
			return;
		for(int i = 0; i < Data->Count; i++) {
			if(Data->Pairs[i]->JsonString->Value() == Name) {
				Data->RemovePair(Name);
				i--;
			}
		}
		Data->AddPair(Name,Value);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TConfFile::LoadDefault() {
	Data = new TJSONObject();
	Set("AuthorizeURI","https://www.dropbox.com/1/oauth2/authorize");
	Set("ApiBaseURI","https://api.dropbox.com/1/");
	Set("ApiContentURI","https://api-content.dropbox.com/1/");
	Set("DisableToken","/disable_access_token");
	Set("AccountInfo","/account/info");
	Set("Metadata","/metadata/auto/");
	Set("Search","/search/auto/");
	Set("Copy","/fileops/copy");
	Set("CreateFolder","/fileops/create_folder");
	Set("Delete","/fileops/delete");
	Set("Move","/fileops/move");
	Set("FilesPut","/files_put/auto/");
	Set("FilesGet","/files/auto/");

	Set("Redirect-Endpoint","https://ya.ru/");
	Set("Client-ID","lybglhu7ccoiplk");
	Set("Client-Secret","on2u5qrf5f7dneb");
	Set("Access-Token","");
}

// ---------------------------------------------------------------------------
void __fastcall TConfFile::SaveToFile() {

	if(FileExists(ConfFileName))
		ConfFile = new TFileStream(ConfFileName, fmOpenWrite);
	else
		ConfFile = new TFileStream(ConfFileName, fmCreate);
	ConfFile->Size = 0;
	JSONString = new TStringStream();
	JSONString->WriteString(Data->ToJSON());
	JSONString->SaveToStream(ConfFile);
}

// ---------------------------------------------------------------------------
__fastcall TConfFile::~TConfFile() {
	SaveToFile();
	ConfFile->Free();
	Data->Free();
}
