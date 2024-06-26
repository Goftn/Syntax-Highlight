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

#ifndef __DIALOGS_H
#define __DIALOGS_H

//****************************************************************************
//
// CCommonDialog
//
// Dialog centrovany k parentu
//

class CCommonDialog: public CDialog
{
  public:
    CCommonDialog(HINSTANCE hInstance, int resID, HWND hParent, CObjectOrigin origin = ooStandard);
    CCommonDialog(HINSTANCE hInstance, int resID, int helpID, HWND hParent, CObjectOrigin origin = ooStandard);

  protected:
    INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual void NotifDlgJustCreated();
};

//
// ****************************************************************************
// CCommonPropSheetPage
//

class CCommonPropSheetPage: public CPropSheetPage
{
  public:
    CCommonPropSheetPage(TCHAR *title, HINSTANCE modul, int resID,
                         DWORD flags /* = PSP_USETITLE*/, HICON icon,
                         CObjectOrigin origin = ooStatic)
                         : CPropSheetPage(title, modul, resID, flags, icon, origin) {}
    CCommonPropSheetPage(TCHAR *title, HINSTANCE modul, int resID, UINT helpID,
                         DWORD flags /* = PSP_USETITLE*/, HICON icon,
                         CObjectOrigin origin = ooStatic)
                         : CPropSheetPage(title, modul, resID, helpID, flags, icon, origin) {}
  protected:

    virtual void NotifDlgJustCreated();
};

//
// ****************************************************************************
// CConfigPageFirst
//

class CConfigPageFirst: public CCommonPropSheetPage
{
  public:
    CConfigPageFirst();

    virtual void Validate(CTransferInfo &ti);  // aby v pripade chyby neprobehla ani cast transferu dat
    virtual void Transfer(CTransferInfo &ti);
};

//
// ****************************************************************************
// CConfigPageSecond
//

class CConfigPageSecond: public CCommonPropSheetPage
{
  public:
    CConfigPageSecond();

    virtual void Transfer(CTransferInfo &ti);
};

//
// ****************************************************************************
// CConfigPageViewer
//

class CConfigPageViewer: public CCommonPropSheetPage
{
  public:
    CConfigPageViewer();

};

//
// ****************************************************************************
// CConfigDialog
//

class CConfigDialog: public CPropertyDialog
{
  protected:
    CConfigPageFirst PageFirst;
    CConfigPageSecond PageSecond;
    CConfigPageViewer PageViewer;

  public:
    CConfigDialog(HWND parent);
};

//
// ****************************************************************************
// CPathDialog
//

class CPathDialog: public CCommonDialog
{
  public:
    char *KeyWords;   // odkaz na vnejsi buffer s cestou (in/out), min. MAX_PATH znaku
    char *Name;
    char *Name_end;
    BOOL *FilePath; // odkaz na vnejsi BOOL hodnotu (in/out) - TRUE/FALSE - cesta k souboru/adresari
    BOOL *Highlight;

  public:
    CPathDialog(HWND parent, char* path, char* name, char* name_end, BOOL *filePath, BOOL *Highlight);
    virtual void Transfer(CTransferInfo &ti);

  protected:
    virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

//****************************************************************************
//
// CCtrlLangDialog
//

class CCtrlLangDialog: public CCommonDialog
{
  public:
     std::string contentText;
     CCtrlLangDialog(HWND hParent, std::string content);

//    virtual void Transfer(CTransferInfo &ti);

  protected:
    virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
   
};


#endif //__DIALOGS_H
