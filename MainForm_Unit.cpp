// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "MainForm_Unit.h"
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "SHDocVw_OCX"
#pragma resource "*.dfm"
TGETPUTDataThread *TAPI = new TGETPUTDataThread();
TMainForm *MainForm;

// 5
// 325
// 385
// ---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner) : TForm(Owner) {
	DragAcceptFiles(Handle, true);
	Application->Icon = GetIcon("Folder");
	IconList = new TStringList();
	IL_FileIcon->ColorDepth = cd32Bit;
	IL_FileIcon->Width = 16;
	IL_FileIcon->Height = 16;

	TAPI->OnAuthorize = OnAuthorize;
	TAPI->OnMDataReady = OnMetadataReady;
	TAPI->OnAInfoReady = OnAInfoReady;
	TAPI->OnItemAdd = OnItemAdd;
	TAPI->OnItemRemove = OnItemRemove;
	TAPI->OnDeauthorize = OnDeauthorize;
	TAPI->FOnShowMessage = OnShowMessage;

	TAPI->ProgressDownloading = FileDownloadProgress;

	Timer = new TTimer(this);
	Timer->Enabled = false;
	TAPI->Authorized();

}

void __fastcall TMainForm::OnShowMessage(UnicodeString Message) {
	ShowMessage(Message);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OnAuthorize(bool Authorized) {
	if (Authorized) {
		TAPI->GetAccountInfo();
		TAPI->GetMetadata();
	}
	isAuthorized = Authorized;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OnAInfoReady(Account_Info *Ainfo) {
	float Quota = FormatFloat("0.##",
		Ainfo->quota_info.quota / 1073741824) * 100;
	float Normal = FormatFloat("0.##",
		Ainfo->quota_info.normal / 1073741824) * 100;
	TVarRec args[2] = {float(Normal / 100), float(Quota / 100)};
	LB_Quota_Size->Caption = Format("%n ГБ из %n ГБ", args, 2);

	PB->MaxValue = Quota;
	PB->Progress = Normal;

	LB_Email->Caption = Ainfo->email;
	LB_Country->Caption = Ainfo->country;

	this->Caption = ExtractFilePath(Application->ExeName) + "Dropbox";
	EDT_Path->Text = "/Dropbox";
	Log::Msg("Account info data recieved", FuncName);
	ClosePanel(Left_P_BeforeAuth);
	delete Ainfo;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OnMetadataReady(Metadata *Mdata) {
	Log::Msg("Loaded '" + Mdata->path + "' path from Dropbox ", FuncName);
	if(Last_Hash != Mdata->hash) {
		Last_Hash = Mdata->hash;
		UnicodeString path = Mdata->path;

		Lw->Items->BeginUpdate();

		for (int i = 0; i < Lw->Items->Count; i++) {
			delete Lw->Items->Item[i]->Data;
		}

		Lw->Items->Clear();

		for(unsigned int i = 0; i < Mdata->Contents.size(); i++) {
			AddItem(Mdata->Contents[i]);
        }

		Lw->Items->EndUpdate();

		if (Mdata->path == "/")
			this->Caption = "Dropbox";
		else
			this->Caption = path.SubString(path.LastDelimiter("/") + 1, path.Length());

		EDT_Path->Text = path;

		__can_forward = false;

		if (Mdata->path == "/")
			__can_backward = false;
		else
			__can_backward = true;

		TComboExItem * item = EDT_Path->ItemsEx->Add();
		item->Caption = Mdata->path;
		item->ImageIndex = IconList->IndexOf("Folder");
		UpdateIcon();

	} else {
		for(unsigned int i = 0; i < Mdata->Contents.size(); i++) {
			delete Mdata->Contents[i];
        }
	}
	delete Mdata;

}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::AddItem(Content *Data) {
	TListItem *Item = Lw->Items->Add();
	UnicodeString file_path = Data->path;
	UnicodeString file_name = file_path.SubString(file_path.LastDelimiter("/") + 1, file_path.Length());
	UnicodeString file_ext;
	Item->Caption = file_name;

	Item->SubItems->Add(ConvertDateTime(Data->modified));

	if (Data->is_dir) {
		Item->SubItems->Add("Папка с файлами");
		file_ext = "Folder";
	} else {
		Item->SubItems->Add("");
		file_ext = file_name.SubString(file_name.LastDelimiter("."), file_name.Length());
	}

	if (Data->bytes != 0)
		Item->SubItems->Add(Data->size);

	if (IconList->IndexOf(file_ext) == -1)
		GetIconFile(file_ext, IL_FileIcon);

	Item->ImageIndex = IconList->IndexOf(file_ext);
	Item->Data = Data;
	LB_Item_Count->Caption = "Элементов: " + IntToStr(Lw->Items->Count);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::RemoveItem(Content *Data) {
	UnicodeString file_path = Data->path;
	UnicodeString file_name = file_path.SubString(file_path.LastDelimiter("/") + 1, file_path.Length());
	for(int i = 0; i < Lw->Items->Count; i++)
		if(Lw->Items->Item[i]->Caption == file_name) {
			delete Lw->Items->Item[i]->Data;
			Lw->Items->Delete(i);
		}

}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OnDeauthorize() {
	isAuthorized = false;
	for (int i = 0; i < Lw->Items->Count; i++)
		delete Lw->Items->Item[i]->Data;
	Lw->Items->Clear();
	OpenPanel(Left_P_BeforeAuth);
	EDT_Path->Clear();
	Last_Hash = "";
}

// ---------------------------------------------------------------------------
UnicodeString __fastcall TMainForm::ConvertDateTime(UnicodeString DateTime) {
	struct tm tm;
	char time_fin[20];

	strptime(AnsiString(DateTime).c_str(),
		"%a, %d %b %Y %H:%M:%S %z", &tm);
	strftime(time_fin, 20, "%d.%m.%Y %H:%M", &tm);
	return time_fin;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTN_LogoutMouseEnter(TObject *Sender) {
	TColor Dest = ((TPanel*)Sender)->Color;
	((TPanel*)Sender)->Color = ((TPanel*)Sender)->Font->Color;
	((TPanel*)Sender)->Font->Color = Dest;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::LwDblClick(TObject *Sender) {
	if(Lw->ItemIndex != -1)
		if (((Content*)Lw->Items->Item[Lw->ItemIndex]->Data)->is_dir)
			TAPI->GetMetadata
				(((Content*)Lw->Items->Item[Lw->ItemIndex]->Data)->path);
		else
			TAPI->OpenFile(((Content*)Lw->Items->Item[Lw->ItemIndex]->Data)->path);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTN_BackwardClick(TObject *Sender) {
	if (EDT_Path->Text != "/") {
		UnicodeString PrevFolder =
			EDT_Path->Text.SubString(0, EDT_Path->Text.LastDelimiter("/") - 1);
		Log::Msg("Load previous folder", FuncName);
		TAPI->GetMetadata(PrevFolder);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OnItemAdd(Content * content) {
	AddItem(content);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OnItemRemove(Content * content) {
	RemoveItem(content);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::GetIconFile(UnicodeString ext, TImageList *imgList) {
	SHFILEINFO FileInfo;
	if (ext == "Folder")
		SHGetFileInfo(L"C://", FILE_ATTRIBUTE_DIRECTORY, &FileInfo,
		sizeof(FileInfo),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	else if (ext != "Folder")
		SHGetFileInfo(ext.w_str(), FILE_ATTRIBUTE_NORMAL, &FileInfo,
		sizeof(FileInfo),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

	TIcon* ico = new TIcon;
	ico->Handle = FileInfo.hIcon;
	int indx = imgList->AddIcon(ico);
	if (indx != -1)
		IconList->Insert(indx, ext);
	delete ico;
	Log::Msg(ext + " ico not available. Loaded", FuncName);
}

// ---------------------------------------------------------------------------
TIcon* __fastcall TMainForm::GetIcon(UnicodeString ext) {
	SHFILEINFO FileInfo;
	if (ext == "Folder")
		SHGetFileInfo(L"C://", FILE_ATTRIBUTE_DIRECTORY, &FileInfo,
		sizeof(FileInfo),
		SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
	else if (ext != "Folder")
		SHGetFileInfo(ext.w_str(), FILE_ATTRIBUTE_NORMAL, &FileInfo,
		sizeof(FileInfo),
		SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);

	TIcon* ico = new TIcon;
	ico->Handle = FileInfo.hIcon;
	return ico;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::NavigateBTNMouseLeave(TObject *Sender) {
	TImage *Img = dynamic_cast<TImage*>(Sender);
	switch (Img->Tag) {
	case 0: {
			if (__can_backward) {
				Img->Picture->Assign(NULL);
				IL_Navigate->GetIcon(0, Img->Picture->Icon);
			}
			break;
		}
	case 1: {
			if (__can_forward) {
				Img->Picture->Assign(NULL);
				IL_Navigate->GetIcon(3, Img->Picture->Icon);
			}
			break;
		}
	case 2: {
			Img->Picture->Assign(NULL);
			IL_UpBtn->GetIcon(0, Img->Picture->Icon);
			break;
		}
	case 3: {
			Img->Picture->Assign(NULL);
			IL_UpdateBtn->GetIcon(0, Img->Picture->Icon);
			break;
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::NavigateBTNMouseEnter(TObject *Sender) {
	TImage *Img = dynamic_cast<TImage*>(Sender);
	switch (Img->Tag) {
	case 0: {
			if (__can_backward) {
				Img->Picture->Assign(NULL);
				IL_Navigate->GetIcon(1, Img->Picture->Icon);
			}
			break;
		}
	case 1: {
			if (__can_forward) {
				Img->Picture->Assign(NULL);
				IL_Navigate->GetIcon(4, Img->Picture->Icon);
			}
			break;
		}
	case 2: {
			Img->Picture->Assign(NULL);
			IL_UpBtn->GetIcon(1, Img->Picture->Icon);
			break;
		}
	case 3: {
			Img->Picture->Assign(NULL);
			IL_UpdateBtn->GetIcon(1, Img->Picture->Icon);
			break;
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTN_AuthClick(TObject *Sender) {
	if (!isAuthorized) {
		TAPI->Connect();
	}
	else
		ClosePanel(Left_P_BeforeAuth);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::ClosePanel(TPanel* Panel) {
	Log::Msg("Close auth panel", FuncName);
	TimerP = Panel;
	Timer->Interval = 1;
	Timer->OnTimer = OnTimer;
	Timer->Tag = 0;
	Timer->Enabled = true;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OpenPanel(TPanel* Panel) {
	Log::Msg("Open auth panel", FuncName);
	TimerP = Panel;
	Timer->Interval = 1;
	Timer->OnTimer = OnTimer;
	Timer->Tag = 1;
	Timer->Enabled = true;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::OnTimer(TObject *Sender) {
	if (Timer->Tag == 0 && TimerP->Width > 0)
		TimerP->Width -= 4;
	else if (Timer->Tag == 1 && TimerP->Width < TimerP->Tag)
		TimerP->Width += 4;
	else {
		Timer->Enabled = false;
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTN_LogoutClick(TObject *Sender) {
	if(isAuthorized) {
		TAPI->Deauthorize();
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::EDT_SearchKeyDown(TObject *Sender, WORD &Key,
	TShiftState Shift) {
	// if (Key == VK_RETURN);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::UpdateIcon() {

	Log::Msg("Update navigate icon", FuncName);
	BTN_Forward->Picture->Assign(NULL);
	BTN_Backward->Picture->Assign(NULL);
	if (__can_forward) {
		IL_Navigate->GetIcon(3, BTN_Forward->Picture->Icon);
	}
	else {
		IL_Navigate->GetIcon(5, BTN_Forward->Picture->Icon);
	}
	if (__can_backward) {
		IL_Navigate->GetIcon(0, BTN_Backward->Picture->Icon);
	}
	else {
		IL_Navigate->GetIcon(2, BTN_Backward->Picture->Icon);
	}
	BTN_Forward->Invalidate();
	BTN_Backward->Invalidate();
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::WmDropFiles(TWMDropFiles& Message) {
	wchar_t buff[MAX_PATH];
	UnicodeString file_path;
	UnicodeString file_name;
	HDROP hDrop = (HDROP)Message.Drop;
	unsigned int file_count = DragQueryFile(hDrop, -1, NULL, NULL);

	for (unsigned int i = 0; i < file_count; i++) {
		DragQueryFile(hDrop, i, buff, sizeof(buff));
		file_path = ExtractFilePath(buff);
		file_name = ExtractFileName(buff);
		TAPI->UploadFile(file_path + file_name, EDT_Path->Text + "/" + file_name);
	}
	DragFinish(hDrop);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::FormDestroy(TObject *Sender) {
	for(int i = 0; i < Lw->Items->Count; i++)
		delete Lw->Items->Item[i]->Data;
	TAPI->DestroyObject();
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTM_MainMouseEnter(TObject *Sender) {
	((TPanel*)Sender)->BevelKind = bkFlat;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTM_MainMouseLeave(TObject *Sender) {
	((TPanel*)Sender)->BevelKind = bkNone;
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTN_UpdateClick(TObject *Sender) {
	TAPI->GetMetadata(EDT_Path->Text);
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::BTN_UpClick(TObject *Sender) {
	if (EDT_Path->Text != "/") {
		UnicodeString PrevFolder =
			EDT_Path->Text.SubString(0, EDT_Path->Text.LastDelimiter("/") - 1);
		Log::Msg("Load previous folder", FuncName);
		TAPI->GetMetadata(PrevFolder);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMainForm::LwMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y)
{
	if(Button == mbRight && Lw->ItemIndex == -1)
		Lw->PopupMenu->CloseMenu();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::OpenExecute(TObject *Sender)
{
	if(Lw->ItemIndex != -1)
		if (((Content*)Lw->Selected->Data)->is_dir)
			TAPI->GetMetadata
				(((Content*)Lw->Selected->Data)->path);
		else
			TAPI->OpenFile(((Content*)Lw->Selected->Data)->path);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormShow(TObject *Sender)
{
	Lw->SetFocus();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::LwClick(TObject *Sender)
{
	//TAPI->CreateFolder("/1 курс/Folder");
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DeleteExecute(TObject *Sender)
{
	if(Lw->ItemIndex != -1)
		TAPI->DeletePath(((Content*)Lw->Selected->Data)->path);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::EDT_PathKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if(Key == VK_RETURN) {
		TAPI->GetMetadata(EDT_Path->Text);
    }
}
//---------------------------------------------------------------------------

