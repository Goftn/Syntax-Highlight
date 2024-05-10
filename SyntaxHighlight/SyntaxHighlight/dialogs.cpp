//****************************************************************************
//
// Copyright (c) ALTAP, spol. s r.o. All rights reserved.
//
// This is a part of the Altap Salamander SDK library.
//
// The SDK is provided "AS IS" and without warranty of any kind and 
// ALTAP EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE and NON-INFRINGEMENT.
//
//****************************************************************************

#include "precomp.h"

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

//****************************************************************************
//
// CCommonDialog
//

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, HWND hParent, CObjectOrigin origin)
: CDialog(hInstance, resID, hParent, origin)
{
}

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, int helpID, HWND hParent, CObjectOrigin origin)
: CDialog(hInstance, resID, helpID, hParent, origin)
{
}

INT_PTR
CCommonDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      // horizontalni i vertikalni vycentrovani dialogu k parentu
      if (Parent != NULL)
        SalamanderGeneral->MultiMonCenterWindow(HWindow, Parent, TRUE);
      break; // chci focus od DefDlgProc
    }
  }
  return CDialog::DialogProc(uMsg, wParam, lParam);
}

void
CCommonDialog::NotifDlgJustCreated()
{
  SalamanderGUI->ArrangeHorizontalLines(HWindow);
}

//
// ****************************************************************************
// CCommonPropSheetPage
//

void
CCommonPropSheetPage::NotifDlgJustCreated()
{
  SalamanderGUI->ArrangeHorizontalLines(HWindow);
}

//
// ****************************************************************************
// CConfigPageFirst
//

CConfigPageFirst::CConfigPageFirst()
  : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGEFIRST, IDD_CFGPAGEFIRST, PSP_HASHELP, NULL)
{
}

void
CConfigPageFirst::Validate(CTransferInfo &ti)
{
  int dummy;  // jen testovaci hodnota (Validate se vola jen pri odchodu s okna pres OK)
  ti.EditLine(IDC_TESTNUMBER, dummy);  // test jde-li o cislo
  if (ti.IsGood() && dummy >= 10)  // test neni-li cislo >= nez 10 (jen jde-li o cislo)
  {
    SalamanderGeneral->SalMessageBox(HWindow, "Number must be less then 10.", "Error",
                                     MB_OK | MB_ICONEXCLAMATION);
    ti.ErrorOn(IDC_TESTNUMBER);
    // PostMessage(GetDlgItem(HWindow, IDC_TESTNUMBER), EM_SETSEL, errorPos1, errorPos2);  // oznaceni pozice chyby
  }
}

void
CConfigPageFirst::Transfer(CTransferInfo &ti)
{
  ti.EditLine(IDC_TESTSTRING, Str, MAX_PATH);
  ti.EditLine(IDC_TESTNUMBER, Number);

  HWND hWnd;
  if (ti.GetControl(hWnd, IDC_COMBO))
  {
    if (ti.Type == ttDataToWindow)  // Transfer() volany pri otevirani okna (data -> okno)
    {
      SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
      SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)"first");
      SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)"second");
      SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)"third");
      SendMessage(hWnd, CB_SETCURSEL, Selection, 0);
    }
    else   // ttDataFromWindow; Transfer() volany pri stisku OK (okno -> data)
    {
      Selection = (int)SendMessage(hWnd, CB_GETCURSEL, 0, 0);
    }
  }
}

//
// ****************************************************************************
// CConfigPageSecond
//

CConfigPageSecond::CConfigPageSecond()
  : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGESECOND, 0, NULL)  // second config page has no help
{
}

void
CConfigPageSecond::Transfer(CTransferInfo &ti)
{
  ti.CheckBox(IDC_CHECK, CheckBox);

  ti.RadioButton(IDC_RADIO1, 10, RadioBox);
  ti.RadioButton(IDC_RADIO2, 13, RadioBox);
  ti.RadioButton(IDC_RADIO3, 20, RadioBox);
}

//
// ****************************************************************************
// CConfigPageViewer
//

CConfigPageViewer::CConfigPageViewer()
  : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGEVIEWER, IDD_CFGPAGEVIEWER, PSP_HASHELP, NULL)
{

}

//
// ****************************************************************************
// CConfigDialog
//

// pomocny objekt pro centrovani konfiguracniho dialogu k parentovi
class CCenteredPropertyWindow: public CWindow
{
  protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      switch (uMsg)
      {
        case WM_WINDOWPOSCHANGING:
        {
          WINDOWPOS *pos = (WINDOWPOS *)lParam;
          if (pos->flags & SWP_SHOWWINDOW)
          {
            HWND hParent = GetParent(HWindow);
            if (hParent != NULL)
              SalamanderGeneral->MultiMonCenterWindow(HWindow, hParent, TRUE);
          }
          break;
        }

        case WM_APP + 1000:   // mame se odpojit od dialogu (uz je vycentrovano)
        {
          DetachWindow();
          delete this;  // trochu prasarna, ale uz se 'this' nikdo ani nedotkne, takze pohoda
          return 0;
        }
      }
      return CWindow::WindowProc(uMsg, wParam, lParam);
    }
};

#ifndef LPDLGTEMPLATEEX
#include <pshpack1.h>
typedef struct DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;
#include <poppack.h>
#endif // LPDLGTEMPLATEEX

// pomocny call-back pro centrovani konfiguracniho dialogu k parentovi a vyhozeni '?' buttonku z captionu
int CALLBACK CenterCallback(HWND HWindow, UINT uMsg, LPARAM lParam)
{
  if (uMsg == PSCB_INITIALIZED)   // pripojime se na dialog
  {
    CCenteredPropertyWindow *wnd = new CCenteredPropertyWindow;
    if (wnd != NULL)
    {
      wnd->AttachToWindow(HWindow);
      if (wnd->HWindow == NULL) delete wnd;  // okno neni pripojeny, zrusime ho uz tady
      else
      {
        PostMessage(wnd->HWindow, WM_APP + 1000, 0, 0);  // pro odpojeni CCenteredPropertyWindow od dialogu
      }
    }
  }
  if (uMsg == PSCB_PRECREATE)   // odstraneni '?' buttonku z headeru property sheetu
  {
    // Remove the DS_CONTEXTHELP style from the dialog box template
    if (((LPDLGTEMPLATEEX)lParam)->signature == 0xFFFF) ((LPDLGTEMPLATEEX)lParam)->style &= ~DS_CONTEXTHELP;
    else ((LPDLGTEMPLATE)lParam)->style &= ~DS_CONTEXTHELP;
  }
  return 0;
}

CConfigDialog::CConfigDialog(HWND parent)
           : CPropertyDialog(parent, HLanguage, LoadStr(IDS_CFG_TITLE),
                             LastCfgPage, PSH_USECALLBACK | PSH_NOAPPLYNOW | PSH_HASHELP,
                             NULL, &LastCfgPage, CenterCallback)
{
  Add(&PageFirst);
  Add(&PageSecond);
  Add(&PageViewer);
}

//
// ****************************************************************************
// CPathDialog
//

CPathDialog::CPathDialog(HWND parent, char *path, char *name, char *name_end, BOOL *filePath, BOOL *highlight)
  : CCommonDialog(HLanguage, IDD_PATHDLG, IDD_PATHDLG, parent)
{
    KeyWords = path;
    Name = name;
    Name_end = name_end;
    FilePath = filePath;
    Highlight = highlight;
}

void
CPathDialog::Transfer(CTransferInfo &ti)
{
    ti.EditLine(IDC_EDIT3, Name, MAX_PATH);
    ti.EditLine(IDC_EDIT2, Name_end, MAX_PATH);
    ti.EditLine(IDC_PATHSTRING, KeyWords, MAX_PATH);
    ti.CheckBox(IDC_CHECK1, *Highlight);
    ti.CheckBox(IDC_FILECHECK, *FilePath);
}
INT_PTR
CPathDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CALL_STACK_MESSAGE4("CPathDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);



  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
        HICON hMyIcon = LoadIcon(DLLInstance, MAKEINTRESOURCE(IDI_FS));
        SendMessage(HWindow, WM_SETICON, ICON_BIG, (LPARAM)hMyIcon);
        CheckDlgButton(HWindow, IDC_CHECK1, g_HighlightEnabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(HWindow, IDC_CHECK2, BST_UNCHECKED);
        return TRUE;

      break;
    }

    case WM_COMMAND: {

        switch (LOWORD(wParam))
        {
        case IDC_BUTTON1: { // LOAD FILE
            OPENFILENAME ofn;       
            char filePath[MAX_PATH] = { 0 };  

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = HWindow;
            ofn.lpstrFile = filePath;
            ofn.nMaxFile = sizeof(filePath);
            ofn.lpstrFilter = "All files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {

                std::ifstream file(filePath);
                if (file.is_open())
                {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    std::string contents = buffer.str();
                    file.close();

                    size_t pos = 0;
                    while ((pos = contents.find("\n", pos)) != std::string::npos) {
                        if (pos == 0 || contents[pos - 1] != '\r') {
                            contents.replace(pos, 1, "\r\n");
                            pos += 2;
                        }
                        else {
                            ++pos; 
                        }
                    }

                    SetDlgItemText(HWindow, IDC_PATHSTRING, contents.c_str());


                    std::string pathStr(filePath);
                    size_t lastSlash = pathStr.find_last_of("\\/");
                    size_t lastDot = pathStr.find_last_of('.');


                    std::string fileName = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1, lastDot - lastSlash - 1) : pathStr;
                    std::string fileExt = (lastDot != std::string::npos) ? pathStr.substr(lastDot + 1) : "";
                    std::string directoryPath = (lastSlash != std::string::npos) ? pathStr.substr(0, lastSlash) : "";


                    SetDlgItemText(HWindow, IDC_EDIT4, directoryPath.c_str());
                    SetDlgItemText(HWindow, IDC_EDIT3, fileName.c_str());
                    SetDlgItemText(HWindow, IDC_EDIT2, fileExt.c_str());

                }
                else
                {
                    MessageBox(HWindow, "Couldn't open file", "Error", MB_ICONERROR);
                }
            }
            break;
        }
        case IDC_BUTTON2: { //SAVE FILE

            char directoryPath[MAX_PATH] = { 0 };
            GetDlgItemText(HWindow, IDC_EDIT4, directoryPath, sizeof(directoryPath)); 

            char fileName[MAX_PATH] = { 0 };
            char fileExtension[MAX_PATH] = { 0 };
            GetDlgItemText(HWindow, IDC_EDIT3, fileName, sizeof(fileName)); 
            GetDlgItemText(HWindow, IDC_EDIT2, fileExtension, sizeof(fileExtension)); 

            BOOL isCheckboxChecked = IsDlgButtonChecked(HWindow, IDC_CHECK2);
            if (isCheckboxChecked) {
                char pluginDLLPath[MAX_PATH];
                if (GetModuleFileName(DLLInstance, pluginDLLPath, MAX_PATH) != 0)
                {

                    char* lastSlash = strrchr(pluginDLLPath, '\\');
                    if (lastSlash != NULL)
                    {
                        *lastSlash = '\0';  
                    }

                    std::string lang_map = std::string(pluginDLLPath) + "\\src-highlite-rel_3.1.8\\src\\lang.map";

                    char userText[256]; 
                    GetDlgItemText(HWindow, IDC_EDIT2, userText, 256); 
                    std::string userString = userText; 

                    std::ofstream file(lang_map, std::ios::app);
                    if (file.is_open())
                    {

                        std::string line = userString + " = " + userString + ".lang";
                        file << std::endl << line;
                        file.close();
                        MessageBox(HWindow, "File succesfully saved", "Saved", MB_ICONINFORMATION);
                    }
                    else
                    {
                        MessageBox(HWindow, "Can't save file", "Error", MB_ICONERROR);
                    }
                }
            }
            
            char saveFilePath[MAX_PATH] = { 0 };
            _snprintf_s(saveFilePath, MAX_PATH, _TRUNCATE, "%s\\%s.%s", directoryPath, fileName, fileExtension);

            std::ofstream file(saveFilePath);
            if (file.is_open())
            {
                int len = GetWindowTextLength(GetDlgItem(HWindow, IDC_PATHSTRING)); 
                char* buffer = new char[len + 1];
                GetDlgItemText(HWindow, IDC_PATHSTRING, buffer, len + 1); 
                file.write(buffer, len);
                file.close();
                delete[] buffer;
                MessageBox(HWindow, "File succesfully saved", "Saved", MB_ICONINFORMATION);
            }
            else
            {
                MessageBox(HWindow, "Can't save file", "Error", MB_ICONERROR);
            }
            break;
        }

        case IDC_BUTTON3: { //SHOW 
            char pluginDLLPath[MAX_PATH];
            if (GetModuleFileName(DLLInstance, pluginDLLPath, MAX_PATH) != 0)
            {

                char* lastSlash = strrchr(pluginDLLPath, '\\');
                if (lastSlash != NULL)
                {
                    *lastSlash = '\0';  
                }

                std::string lang_map = std::string(pluginDLLPath) + "\\src-highlite-rel_3.1.8\\src\\lang.map";

                
                std::ifstream file(lang_map);
                if (!file.is_open()) {
                    MessageBox(HWindow, "Coudln't open lang.map make sure it is in right directory.", "Error", NULL);
                }

                std::string line;
                std::stringstream buffer;

                while (getline(file, line)) {
                    buffer << line << "\r\n";
                }

                CCtrlLangDialog exampleDialog(HWindow, buffer.str());
                exampleDialog.Execute();
                break;
            }
        }

        case IDOK: // OK button
        {


            g_HighlightEnabled = IsDlgButtonChecked(HWindow, IDC_CHECK1) == BST_CHECKED;
            ViewerWindowQueue.BroadcastMessage(WM_USER_VIEWERCFGCHNG, (WPARAM)*Highlight, 0);

            EndDialog(HWindow, 1); 
            return TRUE;
        }
        case IDCANCEL: // Cancel button
        {
            EndDialog(HWindow, 0);
            return TRUE;
        }
        }
    }
    }
  return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}


//****************************************************************************
//
// CToolTipExample
//

class CToolTipExample : public CWindow
{
  public: 
    CToolTipExample(HWND hDlg, int ctrlID) : CWindow(hDlg, ctrlID) {}

  protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      switch (uMsg)
      {
        case WM_MOUSEMOVE:
        {
          DWORD mousePos = GetMessagePos();
          POINT p;
          p.x = GET_X_LPARAM(mousePos);
          p.y = GET_Y_LPARAM(mousePos);
          
          BOOL hit = (WindowFromPoint(p) == HWindow);

          if (GetCapture() == HWindow)
          {
            if (!hit)
            {
              ReleaseCapture();
            }
          }
          else
          {
            if (hit)
            {
              SalamanderGUI->SetCurrentToolTip(HWindow, 0);
              SetCapture(HWindow);
            }
          }
          break;
        }

        case WM_CAPTURECHANGED:
        {
          SalamanderGUI->SetCurrentToolTip(NULL, 0);
          break;
        }

        case WM_USER_TTGETTEXT:
        {
          DWORD id = (DWORD)wParam;
          char *text = (char *)lParam;
          lstrcpyn(text, "ToolTip", TOOLTIP_TEXT_MAX);
          return 0;
        }
      }
      return CWindow::WindowProc(uMsg, wParam, lParam);
    }

};

//****************************************************************************
//
// CCtrlLangDialog
//

CCtrlLangDialog::CCtrlLangDialog(HWND hParent, std::string content)
: CCommonDialog(HLanguage, IDD_CTRLEXAMPLE, hParent)
{
    contentText = content;
}


INT_PTR
CCtrlLangDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CALL_STACK_MESSAGE4("CPathDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
  switch (uMsg)
  {
  case WM_INITDIALOG: { 
      SetDlgItemText(HWindow, IDC_EDIT1, contentText.c_str());
      return TRUE;
  }
  }
  return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

