// ---------------------------------------------------------------------------

#pragma hdrstop
#include "DropboxAPI_Unit.h"
#include "Log_Unit.h"
#pragma package(smart_init)
#pragma comment(lib, "Wininet.lib")

// ---------------------------------------------------------------------------
// TGETPUTDataThread class constructor
// ---------------------------------------------------------------------------
__fastcall TGETPUTDataThread::TGETPUTDataThread() {

	Log.EnableLog();
	Log.Msg("Load DropboxAPI", FuncName);

	AddTaskEvent = new TEvent(NULL, false, false, "AddTaskEvent", false);
	StopThreadEvent = new TEvent(NULL, false, false, "StopThreadEvent", false);

	TempDir  = ExtractFilePath(Application->ExeName) + "temp\\";

	WebForm  = new Tfrm_OAuthWebForm   (Application);
	OAuth2   = new TOAuth2Authenticator(NULL);
	Client   = new TRESTClient         (NULL);
	Request  = new TRESTRequest        (NULL);
	Response = new TRESTResponse       (NULL);
	Mrews    = new TMultiReadExclusiveWriteSynchronizer();

	Client->Authenticator = OAuth2;
	Request->Client       = Client;
	Request->Response     = Response;

	option << poDoNotEncode;

	OAuth2->AuthorizationEndpoint = Conf.Get("AuthorizeURI");
	OAuth2->ClientID              = Conf.Get("Client-ID");
	OAuth2->ClientSecret          = Conf.Get("Client-Secret");
	OAuth2->RedirectionEndpoint   = Conf.Get("Redirect-Endpoint");
	OAuth2->AccessToken           = Conf.Get("Access-Token");
	OAuth2->ResponseType          = TOAuth2ResponseType::rtTOKEN;

	IdHTTP              = new TIdHTTP();
	IdHTTP->IOHandler   = new TIdSSLIOHandlerSocketOpenSSL();

	IdHTTP->OnWork      = OnWork;
	IdHTTP->OnWorkBegin = OnWorkBegin;
	IdHTTP->OnWorkEnd   = OnWorkEnd;

	CheckInternetConnection();
}

// ---------------------------------------------------------------------------
// Execute. Performs tasks in separate thread.
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::Execute() {
	try {
		__isThreadIdle = true;
		Log.Msg("First time start thread", FuncName);
		AddTaskEvent->WaitFor(INT_MAX);
		unsigned int i_count = 0;
		while (!Terminated) {
			CheckInternetConnection();
			if (i_count < Queue.size()) {
				Log.Msg("Current task: " + Queue[i_count].TaskName, FuncName);
				switch (Queue[i_count].Action) {
				case saAinfo:
					__GetAccountInfo();
					break;
				case saMdata:
					__GetMetadata(Queue[i_count].Data[0]);
					break;
				case saChAuth:
					__CheckAuthorize();
					break;
				case saOpFile:
					__OpenFile(Queue[i_count].Data[0]);
					break;
				case saUpFile:
					__UploadFile(Queue[i_count].Data[0], Queue[i_count].Data[1]);
					break;
				case saCreateFolder:
					__CreateFolder(Queue[i_count].Data[0]);
					break;
				case saMovePath:
					__MovePath(Queue[i_count].Data[0],Queue[i_count].Data[1]);
					break;
				case saCopyPath:
					__CopyPath(Queue[i_count].Data[0],Queue[i_count].Data[1]);
					break;
				case saDeletePath:
					__DeletePath(Queue[i_count].Data[0]);
					break;
				case saDeAuth:
					__DisableAccessToken();
					break;
				case saSearch:
					__Search(Queue[i_count].Data[0]);
					break;
				}
				Log.Msg("Task " + Queue[i_count].TaskName + " completed",
					FuncName);
				i_count++;
			} else if (Queue.size() == i_count) {
				TVarRec args[2] = {Queue.size(), i_count};
				Log.Msg(Format("%d operations in the queue, passed %d", args, 2),
					FuncName);
				i_count = 0;
				Log.Msg("The thread enters the waiting for new tasks", FuncName);
				__isThreadIdle = true;
				AddTaskEvent->ResetEvent();
				AddTaskEvent->WaitFor(INT_MAX);
			}
		}
        Log.Msg("Terminate thread",FuncName);
		StopThreadEvent->SetEvent();
	} catch (System::Sysutils::Exception &exception) {
		Log.Msg(exception, FuncName);
	}
}

void __fastcall TGETPUTDataThread::ShowMessage(String Message) {
	Mrews->BeginRead();
	FOnShowMessage(Message);
	Mrews->EndRead();
}

// ---------------------------------------------------------------------------
// Check internet connection
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::CheckInternetConnection() {
	Log.Msg("Check internet connection",FuncName);
	if(InternetCheckConnection(L"https://www.google.com",FLAG_ICC_FORCE_CONNECTION,0))
		__isInternetAvailable = true;
	else
		__isInternetAvailable = false;
}

// ---------------------------------------------------------------------------
// Connect to dropbox and request access token
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::Connect() {
	if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	if (!__isAuthorized) {
		Log.Msg("Open WebForm. Try get access token", WebForm->Name);
		WebForm->OnAfterRedirect = WebFormOnBeforeRedirect;
		try {
			WebForm->ShowWithURL(OAuth2->AuthorizationRequestURI());
		} catch (Rest::Exception::ERESTException &exception) {
			ShowMessage(exception.Message);
			Log.Msg(exception, WebForm->Name);
		}
		if (OAuth2->AccessToken != "") {
			Conf.Set("Access-Token", OAuth2->AccessToken);
			__isAuthorized = true;
			if(FOnAuthorize != NULL) {
				Log.Msg("Call function FOnAuthorize.", FuncName);
				FOnAuthorize(__isAuthorized);
			} else
				Log.Msg("Function FOnAuthorize not defined", FuncName);
		}
	}
}

// ---------------------------------------------------------------------------
// Processing response and extract access token
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::WebFormOnBeforeRedirect
	(const System::String AURL, bool &DoCloseWebView) {
	int pos = AURL.Pos("#access_token=");
	if (pos != 0) {
		String AccessToken =
			AURL.SubString(pos + strlen("#access_token="), AURL.Length());
		OAuth2->AccessToken = AccessToken.SubString(0,AccessToken.Pos("&") - 1);
		Log.Msg("Access token received", WebForm->Name);
		DoCloseWebView = true;
	}
}

// ---------------------------------------------------------------------------
// Function that's add task to queue
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::Authorized() {
	AddToQueue("", "", saChAuth);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::GetAccountInfo() {
	AddToQueue("", "", saAinfo);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::GetMetadata(String Path) {
	AddToQueue(Path, "123", saMdata);
	LastPath = Path;
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::UploadFile(String SourcePath,
	String DestPath) {
	AddToQueue(SourcePath, DestPath, saUpFile);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::OpenFile(String Path) {
	AddToQueue(Path, "", saOpFile);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::CreateFolder(String Path) {
	AddToQueue(Path, "", saCreateFolder);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::MovePath(String FromPath,
		String ToPath) {
	AddToQueue(FromPath, ToPath, saMovePath);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::CopyPath(String FromPath,
		String ToPath) {
	AddToQueue(FromPath, ToPath, saCopyPath);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::DeletePath(String Path) {
	AddToQueue(Path, "", saDeletePath);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::Deauthorize() {
	AddToQueue("", "", saDeAuth);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::Search(String SearchString) {
	AddToQueue(SearchString, "", saSearch);
}

// ---------------------------------------------------------------------------
// Function handles the contents and returns the result as a structure
// ---------------------------------------------------------------------------
void* __fastcall TGETPUTDataThread::ResponseProcess(String Response,
	TReturnValue RValue) {
	if (Response.Length() > 1) {
		switch (RValue) {
		case rvAinfo:
				return new Account_Info(Response);
		case rvMdata:
				return new Metadata(Response);
		case rvContent:
				return new Content(Response);
		case rvSearch:
				return new Metadata(Response, true);
		}
	}
	return NULL;
}

// ---------------------------------------------------------------------------
// This function adds the task to the queue and clears it
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::AddToQueue(String Agr1,
	String Agr2, TSetAction Action) {
	try {
		if(__isThreadIdle){
			if(Queue.size() != 0) {
				Log.Msg("Clear stack", FuncName);
				Queue.clear();
			}
		}
		DataQueue queue_elem;
		queue_elem.Data[0] = Agr1;
		queue_elem.Data[1] = Agr2;
		queue_elem = Action;
		Queue.push_back(queue_elem);
		Log.Msg("Add action to stack: " + queue_elem.TaskName, FuncName);

		if(__isThreadIdle){
			__isThreadIdle = false;
			AddTaskEvent->SetEvent();
		}
	}
	catch(System::Sysutils::Exception &exception) {
		Log.Msg(exception, FuncName);
	}
}

// ---------------------------------------------------------------------------
// Remove temporary directory
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::RemoveDirectory(String dir) {
	Log.Msg("Remove temp folder", FuncName);
	dir = "temp";
	if (DirectoryExists(dir)) {
		PCZZTSTR pFrom = dir.c_str();
		SHFILEOPSTRUCT file_op = {
			NULL, FO_DELETE, pFrom, NULL,
			FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI | FOF_SILENT, false,
			0, NULL};
		int error_code = SHFileOperation(&file_op);
		if (error_code != 0)
			Log.Msg("Delete temp directory returned with error code - " +
			IntToStr(error_code), FuncName);
	}
}

// ---------------------------------------------------------------------------
// Destroy the component of class
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::DestroyObject() {
	RemoveDirectory(TempDir);
	this->Terminate();
	AddTaskEvent->SetEvent();
	AddTaskEvent->Free();
	StopThreadEvent->WaitFor(INT_MAX);
	WebForm->Free();
	OAuth2->Free();
	Client->Free();
	Request->Free();
	Response->Free();
	IdHTTP->Free();

	Queue.clear();
	this->Free();
}

// ---------------------------------------------------------------------------
__fastcall TGETPUTDataThread::~TGETPUTDataThread() {

}

// ---------------------------------------------------------------------------
// Function's that called only from Execute
// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__GetAccountInfo() {
	if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	if (FOnAInfoReady != NULL) {
		try {
			Client->BaseURL = Conf.Get("ApiBaseURI");
			Request->Method = rmGET;
			Request->Resource = Conf.Get("AccountInfo");
			Request->Execute();
			Log.Msg("Request Content given. Call function FOnAInfoReady", FuncName);
			Mrews->BeginWrite();
			FOnAInfoReady((Account_Info*)ResponseProcess(Response->Content,
				rvAinfo));
			Mrews->EndWrite();
		} catch (Rest::Exception::ERESTException &exception) {
			Log.Msg(exception, FuncName);
		}
	}
	else
		Log.Msg("Function FOnAInfoReady not defined", FuncName);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__GetMetadata(String Path) {
	if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	if (FOnMDataReady != NULL) {
		try {
			Client->BaseURL = Conf.Get("ApiBaseURI");
			Request->Method = rmGET;
			Request->Resource = Conf.Get("Metadata") + Path;
			Request->Execute();
			Log.Msg("Metadata given. Call function FOnMDataReady", FuncName);
			Mrews->BeginWrite();
			FOnMDataReady((Metadata*)ResponseProcess(Response->Content, rvMdata));
			Mrews->EndWrite();
		} catch (Rest::Exception::ERESTException &exception) {
			Log.Msg(exception, FuncName);
		}
	}
	else
		Log.Msg("Function FOnMDataReady not defined", FuncName);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__CheckAuthorize() {
	if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	if (FOnAuthorize != NULL) {
		Log.Msg("Check access token.", FuncName);
		if (OAuth2->AccessToken != "") {
			TJSONObject *Obj;
			try {
				Client->BaseURL = Conf.Get("ApiBaseURI");
				Request->Method = rmGET;
				Request->Resource = Conf.Get("AccountInfo");
				Request->Execute();
				Obj = (TJSONObject*) TJSONObject::ParseJSONValue
					(TEncoding::ASCII->GetBytes(Response->Content), 0);
			} catch (Rest::Exception::ERESTException &exception) {
				Log.Msg(exception, FuncName);
			}

			if (Obj->ToJSON().Pos("error") != 0) {
				Log.Msg(Obj->Get("error")->JsonValue->Value(), FuncName);
				__isAuthorized = false;
			} else {
				OAuth2->AccessToken = Conf.Get("Access-Token");
				Log.Msg("Access token Valid.", FuncName);
				__isAuthorized = true;
			}
			Obj->Free();
		} else {
			Log.Msg("No access token in config file", FuncName);
			__isAuthorized = false;
		}
		Log.Msg("Call function FOnAuthorize.", FuncName);
		Mrews->BeginWrite();
		FOnAuthorize(__isAuthorized);
		Mrews->EndWrite();
	} else
		Log.Msg("Function FOnAuthorize not defined", FuncName);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__UploadFile(String SourcePath,
	String DestPath) {
	if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	TFileStream *File = new TFileStream(SourcePath, fmOpenRead);
	String URL = Conf.Get("ApiContentURI") + Conf.Get("FilesPut") + DestPath;
	URL = AuthorizeURL(IdHTTP->URL->URLEncode(URL));
	try {
		String Response;
		Response = IdHTTP->Put(URL, File);
		Log.Msg("File " + ExtractFileName(SourcePath) + " Uploaded.", FuncName);
		File->Free();
		if(FOnItemAdd != NULL) {
			Mrews->BeginWrite();
			FOnItemAdd((Content*)ResponseProcess(Response,rvContent));
			Mrews->EndWrite();
		}
		else
			Log.Msg("Function FOnItemAdd not defined", FuncName);
	} catch (System::Sysutils::Exception &exception) {
		Log.Msg(exception, FuncName);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__OpenFile(String Path) {
	Path = Path.SubString(2, Path.Length());
	String file_name = Path.SubString(Path.LastDelimiter("/") + 1,
		Path.Length());
	String file_path = TempDir + file_name;

	if (!DirectoryExists(TempDir))
		CreateDirectory(TempDir.c_str(), NULL);
	if (!FileExists(file_path)) {
		if(!__isInternetAvailable) {
			ShowMessage("The computer is not connected to the Internet");
			return;
		}
		TFileStream *File = new TFileStream(file_path, fmCreate);
		String URL = Conf.Get("ApiContentURI") + Conf.Get("FilesGet") + Path;

		Log.Msg("File " + file_name + " doesn't exist. Dowloading...",
			FuncName);
		URL = AuthorizeURL(IdHTTP->URL->URLEncode(URL));
		try {
			IdHTTP->Get(URL, File);
			Log.Msg("File " + ExtractFileName(file_path) + " downloaded.",
				FuncName);
		} catch (System::Sysutils::Exception &exception) {
			Log.Msg(exception, FuncName);
		}
		File->Free();
	} else
		Log.Msg("File " + file_name + " exist.", FuncName);
	Log.Msg("File " + file_name + " open", FuncName);
	ShellExecute(NULL, NULL, file_path.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__CreateFolder(String Path) {
    if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	try {
		Client->BaseURL = Conf.Get("ApiBaseURI");
		Request->Method = rmPOST;
		Request->Resource = Conf.Get("CreateFolder");
		Request->AddParameter("root","auto");
		Request->AddParameter("path",IdHTTP->URL->ParamsEncode(Path),pkGETorPOST, option);
		Request->Execute();
		Log.Msg("Request Content given. Call function FOnItemAdd", FuncName);
		if(FOnItemAdd != NULL) {
			Mrews->BeginWrite();
			FOnItemAdd((Content*)ResponseProcess(Response->Content,
				rvContent));
			Mrews->EndWrite();
		}
		else
			Log.Msg("Function FOnItemAdd not defined", FuncName);

	} catch (Rest::Exception::ERESTException &exception) {
		Log.Msg(exception, FuncName);
	}
	Request->Params->Clear();
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__DeletePath(String Path) {
    if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	try {
		Client->BaseURL = Conf.Get("ApiBaseURI");
		Request->Method = rmPOST;
		Request->Resource = Conf.Get("Delete");
		Request->AddParameter("root","auto");
		Request->AddParameter("path", IdHTTP->URL->ParamsEncode(Path),pkGETorPOST, option);
		Request->Execute();
		Log.Msg("Request Content given. Call function FOnItemRemove", FuncName);
		if(FOnItemRemove != NULL) {
			Mrews->BeginWrite();
			FOnItemRemove((Content*)ResponseProcess(Response->Content,
				rvContent));
			Mrews->EndWrite();
		}
		else
			Log.Msg("Function FOnItemRemove not defined", FuncName);
	} catch (Rest::Exception::ERESTException &exception) {
		Log.Msg(exception, FuncName);
	}
	Request->Params->Clear();
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__MovePath(String FromPath,
	String ToPath) {
    if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	try {
		Client->BaseURL = Conf.Get("ApiBaseURI");
		Request->Method = rmPOST;
		Request->Resource = Conf.Get("Move");
		Request->AddParameter("root","auto");
		Request->AddParameter("from_path",IdHTTP->URL->ParamsEncode(FromPath),pkGETorPOST, option);
		Request->AddParameter("to_path",IdHTTP->URL->ParamsEncode(ToPath),pkGETorPOST, option);
		Request->Execute();
		Log.Msg("Request Content given. Call function FOnItemRemove", FuncName);
		if(FOnItemRemove != NULL) {
			Mrews->BeginWrite();
			FOnItemRemove((Content*)ResponseProcess(Response->Content,
				rvContent));
			Mrews->EndWrite();
		}
		else
			Log.Msg("Function FOnItemRemove not defined", FuncName);
	} catch (Rest::Exception::ERESTException &exception) {
		Log.Msg(exception, FuncName);
	}
	Request->Params->Clear();
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__CopyPath(String FromPath,
	String ToPath) {
    if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	try {
		Client->BaseURL = Conf.Get("ApiBaseURI");
		Request->Method = rmPOST;
		Request->Resource = Conf.Get("Copy");
		Request->AddParameter("root","auto");
		Request->AddParameter("from_path",IdHTTP->URL->ParamsEncode(FromPath),pkGETorPOST, option);
		Request->AddParameter("to_path",IdHTTP->URL->ParamsEncode(ToPath),pkGETorPOST, option);
		Request->Execute();
		Log.Msg("Request Content given. Call function FOnItemAdd", FuncName);
		if(FOnItemAdd != NULL) {
			Mrews->BeginWrite();
			FOnItemAdd((Content*)ResponseProcess(Response->Content,
				rvContent));
			Mrews->EndWrite();
		}
		else
			Log.Msg("Function FOnItemAdd not defined", FuncName);
	} catch (Rest::Exception::ERESTException &exception) {
		Log.Msg(exception, FuncName);
	}
	Request->Params->Clear();
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__DisableAccessToken() {
	if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	try {
		Client->BaseURL = Conf.Get("ApiBaseURI");
		Request->Method = rmPOST;
		Request->Resource = Conf.Get("DisableToken");
		Request->Execute();
		Log.Msg("Access token disabled. Call function FOnDeauthorize", FuncName);
		Conf.Set("Access-Token", "");
		OAuth2->AccessToken = "";
		if (FOnDeauthorize != NULL) {
			Mrews->BeginWrite();
			FOnDeauthorize();
            Mrews->EndWrite();
		}
		else
			Log.Msg("Function FOnDeauthorize not defined", FuncName);

	} catch (Rest::Exception::ERESTException &exception) {
		Log.Msg(exception, FuncName);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TGETPUTDataThread::__Search(String SearchString) {
	if(!__isInternetAvailable) {
		ShowMessage("The computer is not connected to the Internet");
		return;
	}
	if (FOnSearchReady != NULL) {
		try {
			Client->BaseURL = Conf.Get("ApiBaseURI");
			Request->Method = rmPOST;
			Request->Resource = Conf.Get("Search");
			Request->AddParameter("query",IdHTTP->URL->ParamsEncode(SearchString),pkGETorPOST, option);
			Request->Execute();
			if(Response->StatusCode != 400) {
				Log.Msg("Metadata given. Call function FOnMDataReady", FuncName);
				Mrews->BeginWrite();
				FOnSearchReady((Metadata*)ResponseProcess(Response->Content, rvSearch));
				Mrews->EndWrite();
			}
		} catch (Rest::Exception::ERESTException &exception) {
			Log.Msg(exception, FuncName);
		}
	}
	else
		Log.Msg("Function FOnSearchReady not defined", FuncName);
	Request->Params->Clear();
}

// ---------------------------------------------------------------------------
// Constructors of Content, Metadata and Account_Info struct
// ---------------------------------------------------------------------------
__fastcall Content::Content() {}

// ---------------------------------------------------------------------------
__fastcall Content::Content(String JSONString) {
	TJSONObject *Obj = (TJSONObject*) TJSONObject::ParseJSONValue(JSONString);
	for(int i = 0; i < Obj->Count; i++) {
		if(Obj->Pairs[i]->JsonString->Value() == "rev")
			rev = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "thumb_exists")
			thumb_exists = StrToBool(Obj->Pairs[i]->JsonValue->Value());//
		else if(Obj->Pairs[i]->JsonString->Value() == "path")
			path = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "is_dir")
			is_dir = StrToBool(Obj->Pairs[i]->JsonValue->Value());//
		else if(Obj->Pairs[i]->JsonString->Value() == "icon")
			icon =  Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "bytes")
			bytes = Obj->Pairs[i]->JsonValue->Value().ToDouble();//
		else if(Obj->Pairs[i]->JsonString->Value() == "modified")
			modified = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "size")
			size = Obj->Pairs[i]->JsonValue->Value(); //
		else if(Obj->Pairs[i]->JsonString->Value() == "root")
			root = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "revision")
			revision = Obj->Pairs[i]->JsonValue->Value().ToDouble(); //
		else if(Obj->Pairs[i]->JsonString->Value() == "mime_type")
			mime_type = Obj->Pairs[i]->JsonValue->Value(); //
	}
	Obj->Free();
}

// ---------------------------------------------------------------------------
__fastcall Metadata::Metadata() {}

// ---------------------------------------------------------------------------
__fastcall Metadata::Metadata(String JSONString, bool OnlyContent) {
	if(!OnlyContent) {
		TJSONObject *Obj = (TJSONObject*) TJSONObject::ParseJSONValue(JSONString);
		for(int i = 0; i < Obj->Count; i++) {
			if(Obj->Pairs[i]->JsonString->Value() == "hash")
				hash = Obj->Pairs[i]->JsonValue->Value();//
			else if(Obj->Pairs[i]->JsonString->Value() == "thumb_exists")
				thumb_exists = StrToBool(Obj->Pairs[i]->JsonValue->Value());//
			else if(Obj->Pairs[i]->JsonString->Value() == "path")
				path = Obj->Pairs[i]->JsonValue->Value();//
			else if(Obj->Pairs[i]->JsonString->Value() == "is_dir")
				is_dir = StrToBool(Obj->Pairs[i]->JsonValue->Value());//
			else if(Obj->Pairs[i]->JsonString->Value() == "icon")
				icon =  Obj->Pairs[i]->JsonValue->Value();//
			else if(Obj->Pairs[i]->JsonString->Value() == "root")
				root = Obj->Pairs[i]->JsonValue->Value();//
			else if(Obj->Pairs[i]->JsonString->Value() == "size")
				size = Obj->Pairs[i]->JsonValue->Value(); //
			else if(Obj->Pairs[i]->JsonString->Value() == "contents") {
				TJSONArray *Arr  = (TJSONArray*) TJSONObject::ParseJSONValue
							(Obj->Pairs[i]->JsonValue->ToJSON());
				for (int j = 0; j < Arr->Count; j++)
					Contents.push_back(new Content(Arr->Items[j]->ToJSON()));
				Arr->Free();
			}
		}
		Obj->Free();
	}
	else {
    	TJSONArray *Arr  = (TJSONArray*) TJSONObject::ParseJSONValue
					(JSONString);
		for (int j = 0; j < Arr->Count; j++)
			Contents.push_back(new Content(Arr->Items[j]->ToJSON()));
		Arr->Free();
	}
}

// ---------------------------------------------------------------------------
__fastcall Account_Info::Account_Info() {}

// ---------------------------------------------------------------------------
__fastcall Account_Info::Account_Info(String JSONString){
	TJSONObject *Obj = (TJSONObject*) TJSONObject::ParseJSONValue(JSONString);

	for(int i = 0; i < Obj->Count; i++) {
		if(Obj->Pairs[i]->JsonString->Value() == "referral_link")
			referral_link = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "display_name")
			display_name = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "uid")
			uid = Obj->Pairs[i]->JsonValue->Value().ToInt();//
		else if(Obj->Pairs[i]->JsonString->Value() == "locale")
			display_name = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "email_verified")
			email_verified = StrToBool(Obj->Pairs[i]->JsonValue->Value());//
		else if(Obj->Pairs[i]->JsonString->Value() == "email")
			email = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "country")
			country = Obj->Pairs[i]->JsonValue->Value();//
		else if(Obj->Pairs[i]->JsonString->Value() == "quota_info") {
			TJSONObject *QObj = (TJSONObject*) TJSONObject::ParseJSONValue
						(Obj->Pairs[i]->JsonValue->ToJSON());
			for(int j = 0; j < QObj->Count; j++) {
				if(QObj->Pairs[j]->JsonString->Value() == "datastores")
					quota_info.datastores = QObj->Pairs[j]->JsonValue->Value().ToDouble();//
				else if(QObj->Pairs[j]->JsonString->Value() == "shared")
					quota_info.shared = QObj->Pairs[j]->JsonValue->Value().ToDouble();//
				else if(QObj->Pairs[j]->JsonString->Value() == "quota")
					quota_info.quota = QObj->Pairs[j]->JsonValue->Value().ToDouble();//
				else if(QObj->Pairs[j]->JsonString->Value() == "normal")
					quota_info.normal = QObj->Pairs[j]->JsonValue->Value().ToDouble();//
			}
			QObj->Free();
		}
	}
	Obj->Free();
}

