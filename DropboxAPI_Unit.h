// ---------------------------------------------------------------------------

#ifndef DropboxAPI_UnitH
#define DropboxAPI_UnitH
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Samples.Gauges.hpp>
#include <limits.h>
#include <vector>

// --Oauth2--
#include <REST.Authenticator.OAuth.WebForm.Win.hpp>
#include <Data.Bind.Components.hpp>
#include <Data.Bind.ObjectScope.hpp>
#include <REST.Authenticator.OAuth.hpp>
#include <REST.Client.hpp>
// ---------
#include <System.JSON.hpp>

#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdHTTP.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <IdIOHandler.hpp>
#include <IdIOHandlerSocket.hpp>
#include <IdIOHandlerStack.hpp>
#include <IdSSL.hpp>
#include <IdSSLOpenSSL.hpp>
#include <IPPeerClient.hpp>

#include <typeinfo>
#include <Wininet.h>
#include "Config_Unit.h"
// ---------------------------------------------------------------------------

struct Account_Info {
	String referral_link;
	String display_name;
	String locale;
	String country;
	String email;
	bool   email_verified;
	bool   is_paired;
	int    uid;
	struct qinfo {
		float datastores;
		float shared;
		float quota;
		float normal;
	} quota_info;
	struct ndet {
		String familiar_name;
		String surname;
		String given_name;
	} name_details;

	__fastcall Account_Info(String JSONString);
	__fastcall Account_Info();
};

struct Content {
	String rev;
	String path;
	String icon;
	String modified;
	String size;
	String root;
	String mime_type;
	float  bytes;
	float  revision;
	bool   thumb_exists;
	bool   is_dir;
	bool   read_only;

	__fastcall Content(String JSONString);
	__fastcall Content();
};

struct Metadata {
	String hash;
	String path;
	String icon;
	String root;
	String size;
	float  bytes;
	bool   thumb_exists;
	bool   is_dir;

	std::vector<Content*> Contents;

	__fastcall Metadata(String JSONString, bool OnlyContent = false);
	__fastcall Metadata();
};

enum TReturnValue : unsigned int {
	rvAinfo,
	rvMdata,
	rvSdata,
	rvContent,
	rvSearch
};

enum TSetAction : unsigned int {
	saAinfo,
	saMdata,
	saChAuth,
	saOpFile,
	saUpFile,
	saCreateFolder,
	saMovePath,
	saCopyPath,
	saDeletePath,
	saDeAuth,
	saSearch
};

const String __TaskName[] = {
	"Get account info",
	"Get metadata",
	"Check access token",
	"Open file",
	"Upload file",
	"Create folder",
	"Move path",
	"Copy path",
	"Delete path",
	"Disable access token",
	"Search"
};

struct DataQueue {
	String Data[2];
	String TaskName;
	TSetAction    Action;

	TSetAction &operator = ( TSetAction &action ) {
		Action   = action;
		TaskName = __TaskName[Action];
		return Action;
	}
};

typedef void __fastcall(__closure * TDropboxOnMetadataReady)(Metadata * MData    );
typedef void __fastcall(__closure * TDropboxOnAInfoReady   )(Account_Info * AInfo);
typedef void __fastcall(__closure * TDropboxOnAuthorize    )(bool Authorized     );
typedef void __fastcall(__closure * TDropboxOnItemAdd      )(Content * content   );
typedef void __fastcall(__closure * TDropboxOnItemRemove   )(Content * content   );
typedef void __fastcall(__closure * TDropboxOnDeauthorize  )(                    );
typedef void __fastcall(__closure * TDropboxOnSearchReady  )(Metadata * MData    );
typedef void __fastcall(__closure * TAPIShowMessage        )(String Message      );


class TGETPUTDataThread : public TThread {
private:
	Tfrm_OAuthWebForm      *WebForm;
	TOAuth2Authenticator   *OAuth2;
	TRESTClient            *Client;
	TRESTRequest           *Request;
	TRESTResponse          *Response;
	TIdHTTP                *IdHTTP;
	TGauge                 *OProgress;
	TEvent                 *AddTaskEvent;
	TEvent                 *StopThreadEvent;
	TConfFile               Conf;
	String           TempDir;
	String           LastPath;
	bool                  __isAuthorized;
	bool                  __isThreadIdle;
	bool                  __isInternetAvailable;
	__int64                 MaxFileSize;
	__int64                 Downloaded;
	TMultiReadExclusiveWriteSynchronizer * Mrews;
	TRESTRequestParameterOptions option;

	std::vector<DataQueue>  Queue; //Stack for task

	TDropboxOnMetadataReady FOnMDataReady;
	TDropboxOnAInfoReady    FOnAInfoReady;
	TDropboxOnAuthorize     FOnAuthorize;
	TDropboxOnItemAdd       FOnItemAdd;
	TDropboxOnItemRemove    FOnItemRemove;
	TDropboxOnDeauthorize   FOnDeauthorize;
	TDropboxOnSearchReady   FOnSearchReady;

	void* __fastcall ResponseProcess(String Response,
		TReturnValue RValue);
	void  __fastcall AddToQueue     (String Agr1, String Agr2,
		TSetAction Action  );
	void  __fastcall RemoveDirectory(String dir);

	void  __fastcall WebFormOnBeforeRedirect(const System::String AURL,
		bool &DoCloseWebView);

	inline String AuthorizeURL(String URL) {
		return (__isAuthorized == true && OAuth2->AccessToken != "") ?
			URL + "?access_token=" + OAuth2->AccessToken : String("");
	}

	inline bool __fastcall StrToBool(String Value) {
		return Value == "true" ? true : false;
	}


	inline void __fastcall OnWBegin() {
		if (OProgress != NULL) {
			OProgress->Visible = true;
			OProgress->Progress = 0;
			OProgress->MaxValue = MaxFileSize;
		}
	}

	inline void __fastcall OnWEnd() {
		if (OProgress != NULL) {
			OProgress->Progress = MaxFileSize;
			OProgress->Visible = false;
		}
	}

	inline void __fastcall OnW() {
		if (OProgress != NULL)
			OProgress->Progress = Downloaded;
	}

	inline void __fastcall OnWorkBegin(TObject* ASender, TWorkMode AWorkMode,
		__int64 AWorkCountMax) {
		MaxFileSize = AWorkCountMax;
		Synchronize(OnWBegin);
	}

	inline void __fastcall OnWorkEnd(TObject* ASender, TWorkMode AWorkMode) {
		Synchronize(OnWEnd);
	}

	inline void __fastcall OnWork(TObject* ASender, TWorkMode AWorkMode,
		__int64 AWorkCount) {
		Downloaded = AWorkCount;
		Synchronize(OnW);
	}

	// Function's that called only from Execute
	void __fastcall __GetAccountInfo();
	void __fastcall __GetMetadata   (String Path = "");
	void __fastcall __Search        (String SearchString);
	void __fastcall __CheckAuthorize();
	void __fastcall __CreateFolder  (String Path);
	void __fastcall __DeletePath    (String Path);
	void __fastcall __UploadFile    (String SourcePath,
		String DestPath);
	void __fastcall __OpenFile      (String Path);
	void __fastcall __MovePath      (String FromPath,
		String ToPath);
	void __fastcall __CopyPath      (String FromPath,
		String ToPath);

	void __fastcall __DisableAccessToken();
	// ----------------------------------------

	void __fastcall ShowMessage(String Message);

protected:
	void __fastcall Execute();

public:

	void __fastcall CheckInternetConnection();

	void __fastcall Connect       ();
	void __fastcall Authorized    ();
	void __fastcall GetAccountInfo();
	void __fastcall Search        (String SearchString);
	void __fastcall GetMetadata   (String Path = "");
	void __fastcall OpenFile      (String Path);
	void __fastcall CreateFolder  (String Path);
	void __fastcall DeletePath    (String Path);
	void __fastcall UploadFile    (String SourcePath,
		String DestPath);
	void __fastcall MovePath      (String FromPath,
		String ToPath);
	void __fastcall CopyPath      (String FromPath,
		String ToPath);

	void __fastcall Deauthorize   ();

	void __fastcall DestroyObject ();

	//Set Events
	__property TDropboxOnMetadataReady OnMDataReady  = {
		write = FOnMDataReady};
	__property TDropboxOnAInfoReady    OnAInfoReady  = {
		write = FOnAInfoReady};
	__property TDropboxOnAuthorize     OnAuthorize   = {
		write = FOnAuthorize};
	__property TDropboxOnItemAdd       OnItemAdd     = {
		write = FOnItemAdd};
	__property TDropboxOnItemRemove    OnItemRemove  = {
		write = FOnItemRemove};
	__property TDropboxOnDeauthorize   OnDeauthorize = {
		write = FOnDeauthorize};
	__property TDropboxOnSearchReady   OnSearchReady = {
		write = FOnSearchReady};

	//

	TAPIShowMessage FOnShowMessage;

	__property TGauge *ProgressDownloading = {write = OProgress};

	__fastcall  TGETPUTDataThread();
	__fastcall ~TGETPUTDataThread();
};

// ---------------------------------------------------------------------------
extern TGETPUTDataThread *TAPI;
// ---------------------------------------------------------------------------
#endif
