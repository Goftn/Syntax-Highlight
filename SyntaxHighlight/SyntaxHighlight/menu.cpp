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

// ****************************************************************************
// SEKCE MENU
// ****************************************************************************

DWORD WINAPI
CPluginInterfaceForMenuExt::GetMenuItemState(int id, DWORD eventMask)
{
  if (id == MENUCMD_DOPFILES)
  {
    // musi byt na disku nebo v nasem pluginu
    if ((eventMask & (MENU_EVENT_DISK | MENU_EVENT_THIS_PLUGIN_ARCH)) == 0) return 0;

    // vezmeme bud oznacene soubory nebo soubor pod fokusem
    const CFileData *file = NULL;
    BOOL isDir;
    if ((eventMask & MENU_EVENT_FILES_SELECTED) == 0)
    {
      if ((eventMask & MENU_EVENT_FILE_FOCUSED) == 0) return 0;
      file = SalamanderGeneral->GetPanelFocusedItem(PANEL_SOURCE, &isDir);  // selected neni nic -> enumerace failne
    }

    BOOL ret = TRUE;
    int count = 0;

    char mask[10];
    SalamanderGeneral->PrepareMask(mask, "*.*");

    int index = 0;
    if (file == NULL) file = SalamanderGeneral->GetPanelSelectedItem(PANEL_SOURCE, &index, &isDir);
    while (file != NULL)
    {
      if (!isDir && SalamanderGeneral->AgreeMask(file->Name, mask, *file->Ext != 0)) count++;
      else
      {
        ret = FALSE;
        break;
      }
      file = SalamanderGeneral->GetPanelSelectedItem(PANEL_SOURCE, &index, &isDir);
    }

    return (count != 0 && ret) ? MENU_ITEM_STATE_ENABLED : 0;  // vsechny oznacene soubory jsou *.dop
  }
  else
  {
    if (id == MENUCMD_SEP || id == MENUCMD_HIDDENITEM)    // osetrime separator a polozku schovavajici se na stisk Shiftu
    {
      return MENU_ITEM_STATE_ENABLED |
             ((GetKeyState(VK_SHIFT) & 0x8000) ? MENU_ITEM_STATE_HIDDEN : 0);
    }
    else TRACE_E("Unexpected call to CPluginInterfaceForMenuExt::GetMenuItemState()");
  }
  return 0;
}

struct CTestItem
{
  int a;
  char b[100];
  CTestItem(int i)
  {
    a = i;
    b[0] = 0;
  }
  ~CTestItem()
  {
    a = 0;
  }
};

struct CDEMOPLUGOperFromDiskData
{
  CQuadWord *RetSize;
  HWND Parent;
  BOOL Success;
};

void WINAPI DEMOPLUGOperationFromDisk(const char *sourcePath, SalEnumSelection2 next,
                                      void *nextParam, void *param)
{
  CDEMOPLUGOperFromDiskData *data = (CDEMOPLUGOperFromDiskData *)param;
  CQuadWord *retSize = data->RetSize;

  data->Success = TRUE;  // zatim hlasime uspech operace

  BOOL isDir;
  const char *name;
  const char *dosName;   // dummy
  CQuadWord size;
  DWORD attr;
  FILETIME lastWrite;
  CQuadWord totalSize(0, 0);
  int errorOccured;
  while ((name = next(data->Parent, 3, &dosName, &isDir, &size, &attr, &lastWrite,
                      nextParam, &errorOccured)) != NULL)
  {
    if (errorOccured == SALENUM_ERROR) // sem SALENUM_CANCEL prijit nemuze
      data->Success = FALSE;
    if (!isDir) totalSize += size;
  }
  if (errorOccured != SALENUM_SUCCESS)
  {
    data->Success = FALSE;
    // nastala chyba pri enumeraci a uzivatel si preje ukonceni operace (cancel)
    // if (errorOccured == SALENUM_CANCEL);
  }

  *retSize = totalSize;
}

BOOL WINAPI
CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract *salamander,
                                            HWND parent, int id, DWORD eventMask)
{
  switch (id)
  {
    case MENUCMD_ALWAYS:
    {

      SalamanderGeneral->ShowMessageBox("Always", LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
      SalamanderGeneral->SetUserWorkedOnPanelPath(PANEL_SOURCE);  // tento prikaz povazujeme za praci s cestou (objevi se v Alt+F12)
      break;
    }
    
    case MENUCMD_CONFIG:   
    {

      char path[MAX_PATH];
      char name[MAX_PATH];
      char name_end[MAX_PATH];
      path[0] = 0;
      name[0] = 0;
      name_end[0] = 0;
      int type;
      if (SalamanderGeneral->GetPanelPath(PANEL_TARGET, path, MAX_PATH, &type, NULL))
      {
        if (type != PATH_TYPE_WINDOWS) path[0] = 0;
      }

      BOOL curPathIsDisk = FALSE;
      char curPath[MAX_PATH];
      curPath[0] = 0;
      if (SalamanderGeneral->GetPanelPath(PANEL_SOURCE, curPath, MAX_PATH, &type, NULL))
      {
        if (type != PATH_TYPE_WINDOWS) curPath[0] = 0;  
        else curPathIsDisk = TRUE;
      }

      SalamanderGeneral->SalUpdateDefaultDir(TRUE);    // update pred pouzitim SalParsePath

      BOOL filePath = FALSE;  
      BOOL success = FALSE;    

      CPathDialog dlg(parent, path, name, name_end, &filePath, &g_HighlightEnabled);
      if (dlg.Execute() == IDOK)
      {
          break;
      }

      return FALSE;
        
    }



    case MENUCMD_CHECKDEMOPLUGTMPDIR:
    {
      ClearTEMPIfNeeded(SalamanderGeneral->GetMsgBoxParent());
      return FALSE;  // neodznacovat
    }

    
    default: SalamanderGeneral->ShowMessageBox("Unknown command.", LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR); break;
  }

  return FALSE;  // neodznacovat polozky v panelu
}

BOOL WINAPI
CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
  int helpID = 0;
  switch (id)
  {
    case MENUCMD_CONFIG: helpID = IDH_ENTERDISKPATH; break;
  }
  if (helpID != 0) SalamanderGeneral->OpenHtmlHelp(parent, HHCDisplayContext, helpID, FALSE);
  return helpID != 0;
}

void WINAPI
CPluginInterfaceForMenuExt::BuildMenu(HWND parent, CSalamanderBuildMenuAbstract *salamander)
{
#ifdef ENABLE_DYNAMICMENUEXT
    salamander->AddMenuItem(0, "N&astavení Syntax Highlightingu", SALHOTKEY('T', HOTKEYF_CONTROL | HOTKEYF_SHIFT), MENUCMD_CONFIG, FALSE, MENU_EVENT_TRUE, MENU_EVENT_DISK, MENU_SKILLLEVEL_ALL);
#endif // ENABLE_DYNAMICMENUEXT
}
