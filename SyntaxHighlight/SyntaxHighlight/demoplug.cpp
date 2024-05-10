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
#include <uxtheme.h>

// objekt interfacu pluginu, jeho metody se volaji ze Salamandera
CPluginInterface PluginInterface;
// dalsi casti interfacu CPluginInterface
CPluginInterfaceForArchiver InterfaceForArchiver;
CPluginInterfaceForViewer InterfaceForViewer;
CPluginInterfaceForMenuExt InterfaceForMenuExt;
CPluginInterfaceForFS InterfaceForFS;
CPluginInterfaceForThumbLoader InterfaceForThumbLoader;

// globalni data
const char *PluginNameEN = "Syntax Highlight";    // neprekladane jmeno pluginu, pouziti pred loadem jazykoveho modulu + pro debug veci
const char *PluginNameShort = "HIGHLIGHT"; // jmeno pluginu (kratce, bez mezer)

char Str[MAX_PATH] = "default";
int Number = 0;
int Selection = 1;      // "second" in configuration dialog
BOOL CheckBox = FALSE;
int RadioBox = 13;      // radio 2 in configuration dialog
BOOL CfgSavePosition = FALSE;             // ukladat pozici okna/umistit dle hlavniho okna
WINDOWPLACEMENT CfgWindowPlacement = {0}; // neplatne, pokud CfgSavePosition != TRUE
int Size2FixedWidth = 0;   // sloupec Size2 (archiver): LO/HI-WORD: levy/pravy panel: FixedWidth
int Size2Width = 0;        // sloupec Size2 (archiver): LO/HI-WORD: levy/pravy panel: Width
int CreatedFixedWidth = 0; // sloupec Created (FS): LO/HI-WORD: levy/pravy panel: FixedWidth
int CreatedWidth = 0;      // sloupec Created (FS): LO/HI-WORD: levy/pravy panel: Width
int ModifiedFixedWidth = 0;// sloupec Modified (FS): LO/HI-WORD: levy/pravy panel: FixedWidth
int ModifiedWidth = 0;     // sloupec Modified (FS): LO/HI-WORD: levy/pravy panel: Width
int AccessedFixedWidth = 0;// sloupec Accessed (FS): LO/HI-WORD: levy/pravy panel: FixedWidth
int AccessedWidth = 0;     // sloupec Accessed (FS): LO/HI-WORD: levy/pravy panel: Width
int DFSTypeFixedWidth = 0; // sloupec DFS Type (FS): LO/HI-WORD: levy/pravy panel: FixedWidth
int DFSTypeWidth = 0;      // sloupec DFS Type (FS): LO/HI-WORD: levy/pravy panel: Width

// Naètení hodnoty zvýraznìní syntaxe
DWORD highlightingEnabled = 0;
DWORD LastCfgPage = 0;   // start page (sheet) in configuration dialog

const char* CONFIG_HIGHLIGHT = "Highlighting";
const char *CONFIG_KEY = "test key";
const char *CONFIG_STR = "test string";
const char *CONFIG_NUMBER = "test number";
const char *CONFIG_SAVEPOS = "SavePosition";
const char *CONFIG_WNDPLACEMENT = "WindowPlacement";
const char *CONFIG_SIZE2FIXEDWIDTH = "Size2FixedWidth";
const char *CONFIG_SIZE2WIDTH = "Size2Width";
const char *CONFIG_CREATEDFIXEDWIDTH = "CreatedFixedWidth";
const char *CONFIG_CREATEDWIDTH = "CreatedWidth";
const char *CONFIG_MODIFIEDFIXEDWIDTH = "ModifiedFixedWidth";
const char *CONFIG_MODIFIEDWIDTH = "ModifiedWidth";
const char *CONFIG_ACCESSEDFIXEDWIDTH = "AccessedFixedWidth";
const char *CONFIG_ACCESSEDWIDTH = "AccessedWidth";
const char *CONFIG_DFSTYPEFIXEDWIDTH = "DFSTypeFixedWidth";
const char *CONFIG_DFSTYPEWIDTH = "DFSTypeWidth";

// ConfigVersion: 0 - zadna konfigurace se z Registry nenacetla (jde o instalaci pluginu),
//                1 - prvni verze konfigurace
//                2 - druha verze konfigurace (pridane nejake hodnoty do konfigurace)
//                3 - treti verze konfigurace (pridani pripony "dmp2")
//                4 - ctvrta verze konfigurace (zmena pripony "dmp" (kolize s minidumpy) na "dop" a "dmp2" na "dop2")

int ConfigVersion = 0;            // verze nactene konfigurace z registry (popis verzi viz vyse)
#define CURRENT_CONFIG_VERSION 4  // aktualni verze konfigurace (uklada se do registry pri unloadu pluginu)
const char *CONFIG_VERSION = "Version";

// ukazatele na tabulky mapovani na mala/velka pismena
unsigned char *LowerCase = NULL;
unsigned char *UpperCase = NULL;

HINSTANCE DLLInstance = NULL;       // handle k SPL-ku - jazykove nezavisle resourcy
HINSTANCE HLanguage = NULL;         // handle k SLG-cku - jazykove zavisle resourcy

// obecne rozhrani Salamandera - platne od startu az do ukonceni pluginu
CSalamanderGeneralAbstract *SalamanderGeneral = NULL;

// definice promenne pro "dbg.h"
CSalamanderDebugAbstract *SalamanderDebug = NULL;

// definice promenne pro "spl_com.h"
int SalamanderVersion = 0;

// rozhrani poskytujici upravene Windows controly pouzivane v Salamanderovi
CSalamanderGUIAbstract *SalamanderGUI = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
  {
    DLLInstance = hinstDLL;

    INITCOMMONCONTROLSEX initCtrls;
    initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    initCtrls.dwICC = ICC_BAR_CLASSES;
    if (!InitCommonControlsEx(&initCtrls))
    {
      MessageBox(NULL, "InitCommonControlsEx failed!", "Error", MB_OK | MB_ICONERROR);
      return FALSE;  // DLL won't start
    }
  }

  return TRUE;    // DLL can be loaded
}

char *LoadStr(int resID)
{
  return SalamanderGeneral->LoadStr(HLanguage, resID);
}

void LoadSettingsFromRegistry() {
    HKEY hKey;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwValue = 0;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Altap\\SyntaxHighlight"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, TEXT("HighlightingEnabled"), NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            g_HighlightEnabled = dwValue != 0;
        }
        RegCloseKey(hKey);
    }
}

void SaveSettingsToRegistry() {
    HKEY hKey;
    DWORD dwValue = g_HighlightEnabled ? 1 : 0;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Altap\\SyntaxHighlight"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, TEXT("HighlightingEnabled"), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));
        RegCloseKey(hKey);
    }
}
void
OnConfiguration(HWND hParent)
{
  static BOOL InConfiguration = FALSE;
  if (InConfiguration)
  {
    SalamanderGeneral->SalMessageBox(hParent, LoadStr(IDS_CFG_ALREADY_OPENED), LoadStr(IDS_PLUGINNAME),
                                     MB_ICONINFORMATION | MB_OK);
    return;
  }
  InConfiguration = TRUE;
  char path[MAX_PATH];
  char name[MAX_PATH];
  char name_end[MAX_PATH];
  path[0] = 0;
  name[0] = 0;
  name_end[0] = 0;
  BOOL filePath = FALSE;
  CPathDialog dlg(hParent, path, name, name_end, &filePath, &g_HighlightEnabled);
  if (dlg.Execute() == IDOK)
      ViewerWindowQueue.BroadcastMessage(WM_USER_VIEWERCFGCHNG, 0, 0);

  InConfiguration = FALSE;
}

void OnAbout(HWND hParent)
{
  char buf[1000];
  _snprintf_s(buf, _TRUNCATE, 
              "%s " "\n\n"
              "%s",
              LoadStr(IDS_PLUGINNAME),
              LoadStr(IDS_PLUGIN_DESCRIPTION));
  SalamanderGeneral->SalMessageBox(hParent, buf, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}

void InitIconOverlays()
{
  // zrusime aktualni icon-overlays, pro pripad chyby, at mame vycisteno
  SalamanderGeneral->SetPluginIconOverlays(0, NULL);

  HINSTANCE imageResDLL = HANDLES(LoadLibraryEx("imageres.dll", NULL, LOAD_LIBRARY_AS_DATAFILE));
  // Salamander bez imageres.dll nebezi, takze tady nacteni nemuze selhat

  // 48x48 az od XP (brzy bude obsolete, pobezime jen na XP+, pak zahodit)
  // ve skutecnosti jsou velke ikonky podporeny uz davno, lze je nahodit
  // Desktop/Properties/???/Large Icons; pozor, nebude pak existovat system image list
  // pro ikonky 32x32; navic bychom meli ze systemu vytahnout realne velikosti ikonek
  // zatim na to kasleme a 48x48 povolime az od XP, kde jsou bezne dostupne
  int iconSizes[3] = {16, 32, 48};
  if (!SalIsWindowsVersionOrGreater(5, 1, 0))  // neni WindowsXPAndLater: neni XP and later
    iconSizes[2] = 32;

  HINSTANCE iconDLL = imageResDLL;
  int iconOverlaysCount = 0;
  HICON iconOverlays[6];
  int iconIndex = 164;  // shared icon-overlay
  for (int i = 0; i < 3; i++)
  {
    iconOverlays[iconOverlaysCount++] = (HICON)LoadImage(iconDLL, MAKEINTRESOURCE(iconIndex),
                                                         IMAGE_ICON, iconSizes[i], iconSizes[i],
                                                         SalamanderGeneral->GetIconLRFlags());
  }

  iconIndex = 97;  // slow file icon-overlay
  for (int i = 0; i < 3; i++)
  {
    iconOverlays[iconOverlaysCount++] = (HICON)LoadImage(iconDLL, MAKEINTRESOURCE(iconIndex),
                                                         IMAGE_ICON, iconSizes[i], iconSizes[i],
                                                         SalamanderGeneral->GetIconLRFlags());
  }

  // nastavime dva overlayes: shared + slow file
  // POZN.: pri chybe loadu ikon SetPluginIconOverlays() selze, ale platne ikony z iconOverlays[] uvolni
  SalamanderGeneral->SetPluginIconOverlays(iconOverlaysCount / 3, iconOverlays);

  if (imageResDLL != NULL) HANDLES(FreeLibrary(imageResDLL));
}

//
// ****************************************************************************
// SalamanderPluginGetReqVer and SalamanderPluginGetSDKVer
//

#ifdef __BORLANDC__
extern "C"{
int WINAPI SalamanderPluginGetReqVer();
int WINAPI SalamanderPluginGetSDKVer();
CPluginInterfaceAbstract * WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract *salamander);
};
#endif // __BORLANDC__

int WINAPI SalamanderPluginGetReqVer()
{
#ifdef DEMOPLUG_COMPATIBLE_WITH_400
  return 100;  // plugin works in Altap Salamander 4.0 or later
#else // DEMOPLUG_COMPATIBLE_WITH_400
  return LAST_VERSION_OF_SALAMANDER;
#endif DEMOPLUG_COMPATIBLE_WITH_400
}

int WINAPI SalamanderPluginGetSDKVer()
{
  return LAST_VERSION_OF_SALAMANDER;  // return current SDK version
}

//
// ****************************************************************************
// SalamanderPluginEntry
//

CPluginInterfaceAbstract * WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract *salamander)
{
  // nastavime SalamanderDebug pro "dbg.h"
  SalamanderDebug = salamander->GetSalamanderDebug();
  // nastavime SalamanderVersion pro "spl_com.h"
  SalamanderVersion = salamander->GetVersion();

  HANDLES_CAN_USE_TRACE();
  CALL_STACK_MESSAGE1("SalamanderPluginEntry()");

//  char path[MAX_PATH];
//  GetModuleFileName(DLLInstance, path, MAX_PATH);
//  GetVerInfo(path, path, MAX_PATH);

  // tento plugin je delany pro aktualni verzi Salamandera a vyssi - provedeme kontrolu
#ifdef DEMOPLUG_COMPATIBLE_WITH_400
  if (SalamanderVersion < 100)  // plugin works in Altap Salamander 4.0 or later
#else // DEMOPLUG_COMPATIBLE_WITH_400
  if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
#endif // DEMOPLUG_COMPATIBLE_WITH_400
  {  // starsi verze odmitneme
    MessageBox(salamander->GetParentWindow(),
#ifdef DEMOPLUG_COMPATIBLE_WITH_400
               "This plugin requires Altap Salamander 4.0 or later.",
#else // DEMOPLUG_COMPATIBLE_WITH_400
               REQUIRE_LAST_VERSION_OF_SALAMANDER,
#endif // DEMOPLUG_COMPATIBLE_WITH_400
               PluginNameEN, MB_OK | MB_ICONERROR);
    return NULL;
  }

  // nechame nacist jazykovy modul (.slg)
  HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), PluginNameEN);
  if (HLanguage == NULL) return NULL;

  // ziskame obecne rozhrani Salamandera
  SalamanderGeneral = salamander->GetSalamanderGeneral();
  SalamanderGeneral->GetLowerAndUpperCase(&LowerCase, &UpperCase);
  // ziskame rozhrani poskytujici upravene Windows controly pouzivane v Salamanderovi
  SalamanderGUI = salamander->GetSalamanderGUI();

  // nastavime jmeno souboru s helpem
  SalamanderGeneral->SetHelpFileName("Syntax_Highlight.chm");

//  BYTE c = SalamanderGeneral->GetUserDefaultCharset();

  if (!InitViewer()) return NULL;  // chyba

  if (!InitFS())
  {
    ReleaseViewer();
    return NULL;  // chyba
  }

  InitIconOverlays();

/*
  // ukazka padu aplikace
  int *p = 0;
  *p = 0;       // ACCESS VIOLATION !
*/

#ifndef DEMOPLUG_QUIET
  // vypis testovaciho hlaseni
  char buf[100];
  sprintf(buf, "SalamanderPluginEntry called, Salamander version %d", salamander->GetVersion());
  TRACE_I(buf);  // do TRACE SERVERU
  SalamanderGeneral->SalMessageBox(salamander->GetParentWindow(), buf, LoadStr(IDS_PLUGINNAME),
                                   MB_OK | MB_ICONINFORMATION);  // do okna
#endif // DEMOPLUG_QUIET
  std::string extension = "*.java;*.cpp;*.c;*.cc;*.cs;*.go;*.h;*.hpp;*.javascript;*.prolog;*.php;*.py;*.syslog;*.log;*.html;*.latex;*.bash;*.sh;*.in;*.css;*.ini;*.pkgconfig;*.bat;*.groovy;*.feature;*.hs;*.texi;*.texinfo;*.php4;*.php5;*.perl;*.ruby;*.bison;*.changelog;*.lua;*.csharp;*.C";
  // nastavime zakladni informace o pluginu
  salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME),
                                 FUNCTION_VIEWER,
                                 VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT, LoadStr(IDS_PLUGIN_DESCRIPTION),
                                 PluginNameShort, extension.c_str(), "");
  LoadSettingsFromRegistry();
  // chceme dostavat zpravy o zavedeni/zmene/zruseni master passwordu
  SalamanderGeneral->SetPluginUsesPasswordManager();

  // nastavime URL home-page pluginu
  salamander->SetPluginHomePageURL(LoadStr(IDS_PLUGIN_HOME));

  // ziskame nase FS-name (nemusi byt "dfs", Salamander ho muze upravit)
  SalamanderGeneral->GetPluginFSName(AssignedFSName, 0);
  AssignedFSNameLen = (int)strlen(AssignedFSName);

  // test pridani vice jmen file systemu
  char demoFSAssignedFSName[MAX_PATH]; // melo by byt v globalni promenne (aby se dalo pouzivat po celem pluginu)
  demoFSAssignedFSName[0] = 0;
  int demoFSFSNameIndex;  // melo by byt v globalni promenne (aby se dalo pouzivat po celem pluginu)
  if (salamander->AddFSName("Highlight", &demoFSFSNameIndex))
    SalamanderGeneral->GetPluginFSName(demoFSAssignedFSName, demoFSFSNameIndex);
  char demoAssignedFSName[MAX_PATH]; // melo by byt v globalni promenne (aby se dalo pouzivat po celem pluginu)
  demoAssignedFSName[0] = 0;
  int demoFSNameIndex;  // melo by byt v globalni promenne (aby se dalo pouzivat po celem pluginu)
  if (salamander->AddFSName("Highlight", &demoFSNameIndex))
    SalamanderGeneral->GetPluginFSName(demoAssignedFSName, demoFSNameIndex);


  // nastavime pluginu, aby se loadil pri startu Salamandera, potrebujeme pravidelne cistit
  // tmp-dir od predchozich instanci DEMOPLUGu
  SalamanderGeneral->SetFlagLoadOnSalamanderStart(TRUE);

  // pokud doslo k loadu pluginu na zaklade flagu "load on start", zkontrolujeme jestli
  // je nas proces Salamandera prvni spusteny a pripadne zavolame cisteni tmp-diru
  if (salamander->GetLoadInformation() & LOADINFO_LOADONSTART)
  {
    if (SalamanderGeneral->IsFirstInstance3OrLater())
      SalamanderGeneral->PostMenuExtCommand(MENUCMD_CHECKDEMOPLUGTMPDIR, TRUE);  // doruci se az "sal-idle" (jinak by nesmelo byt v entry-pointu)
    SalamanderGeneral->PostUnloadThisPlugin();  // az se zpracuji vsechny prikazy, provede se unload pluginu (je zbytecne aby zustaval naloadeny)
  }

  return &PluginInterface;
}

//
// ****************************************************************************
// CPluginInterface
//

void WINAPI
CPluginInterface::About(HWND parent)
{
  OnAbout(parent);
}

BOOL WINAPI
CPluginInterface::Release(HWND parent, BOOL force)
{
  CALL_STACK_MESSAGE2("CPluginInterface::Release(, %d)", force);
  BOOL ret = ViewerWindowQueue.Empty();
  if (!ret && (force || SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_VIEWER_OPENWNDS),
                                                         LoadStr(IDS_PLUGINNAME),
                                                         MB_YESNO | MB_ICONQUESTION) == IDYES))
  {
    ret = ViewerWindowQueue.CloseAllWindows(force) || force;
  }
  if (ret)
  {
    if (!ThreadQueue.KillAll(force) && !force) ret = FALSE;
    else
    {
      // test CallLoadOrSaveConfiguration
      //SalamanderGeneral->CallLoadOrSaveConfiguration(FALSE, TestLoadOrSaveConfiguration, (void *)0x1234);

      ReleaseViewer();
      ReleaseFS();

      // zrusime vsechny kopie souboru z FS v disk-cache (teoreticky zbytecne, kazdy FS po sobe kopie rusi)
      char uniqueFileName[MAX_PATH];
      strcpy(uniqueFileName, AssignedFSName);
      strcat(uniqueFileName, ":");
      // jmena na disku jsou "case-insensitive", disk-cache je "case-sensitive", prevod
      // na mala pismena zpusobi, ze se disk-cache bude chovat take "case-insensitive"
      SalamanderGeneral->ToLowerCase(uniqueFileName);
      SalamanderGeneral->RemoveFilesFromCache(uniqueFileName);
    }
  }
  SaveSettingsToRegistry();
  if (ret && InterfaceForFS.GetActiveFSCount() != 0)
  {
    TRACE_E("Some FS interfaces were not closed (count=" << InterfaceForFS.GetActiveFSCount() << ")");
  }
  return ret;
}

void WINAPI
CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract *registry)
{
  CALL_STACK_MESSAGE1("CPluginInterface::LoadConfiguration(, ,)");

  if (regKey != NULL)   // load z registry
  {
    if (!registry->GetValue(regKey, CONFIG_VERSION, REG_DWORD, &ConfigVersion, sizeof(DWORD)))
      ConfigVersion = 1;  // verze s konfiguraci v registry (uz byla pouzivana, jen bez cisla konfigurace)

    HKEY actKey;
    if (registry->OpenKey(regKey, CONFIG_KEY, actKey))
    {
      registry->GetValue(actKey, CONFIG_STR, REG_SZ, Str, MAX_PATH);
      registry->CloseKey(actKey);
    }

    registry->GetValue(regKey, CONFIG_NUMBER, REG_DWORD, &Number, sizeof(DWORD));

    registry->GetValue(regKey, CONFIG_SAVEPOS, REG_DWORD, &CfgSavePosition, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_WNDPLACEMENT, REG_BINARY, &CfgWindowPlacement, sizeof(WINDOWPLACEMENT));

    registry->GetValue(regKey, CONFIG_SIZE2FIXEDWIDTH, REG_DWORD, &Size2FixedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_SIZE2WIDTH, REG_DWORD, &Size2Width, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_CREATEDFIXEDWIDTH, REG_DWORD, &CreatedFixedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_CREATEDWIDTH, REG_DWORD, &CreatedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_MODIFIEDFIXEDWIDTH, REG_DWORD, &ModifiedFixedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_MODIFIEDWIDTH, REG_DWORD, &ModifiedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_ACCESSEDFIXEDWIDTH, REG_DWORD, &AccessedFixedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_ACCESSEDWIDTH, REG_DWORD, &AccessedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_DFSTYPEFIXEDWIDTH, REG_DWORD, &DFSTypeFixedWidth, sizeof(DWORD));
    registry->GetValue(regKey, CONFIG_DFSTYPEWIDTH, REG_DWORD, &DFSTypeWidth, sizeof(DWORD));
  }
}

void WINAPI
CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract *registry)
{
  CALL_STACK_MESSAGE1("CPluginInterface::SaveConfiguration(, ,)");

  DWORD v = CURRENT_CONFIG_VERSION;
  registry->SetValue(regKey, CONFIG_VERSION, REG_DWORD, &v, sizeof(DWORD));

  HKEY actKey;
  if (registry->CreateKey(regKey, CONFIG_KEY, actKey))
  {
    registry->SetValue(actKey, CONFIG_STR, REG_SZ, Str, -1);
    registry->CloseKey(actKey);
  }

  registry->SetValue(regKey, CONFIG_NUMBER, REG_DWORD, &Number, sizeof(DWORD));

  registry->SetValue(regKey, CONFIG_SAVEPOS, REG_DWORD, &CfgSavePosition, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_WNDPLACEMENT, REG_BINARY, &CfgWindowPlacement, sizeof(WINDOWPLACEMENT));
  registry->SetValue(regKey, CONFIG_SIZE2FIXEDWIDTH, REG_DWORD, &Size2FixedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_SIZE2WIDTH, REG_DWORD, &Size2Width, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_CREATEDFIXEDWIDTH, REG_DWORD, &CreatedFixedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_CREATEDWIDTH, REG_DWORD, &CreatedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_MODIFIEDFIXEDWIDTH, REG_DWORD, &ModifiedFixedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_MODIFIEDWIDTH, REG_DWORD, &ModifiedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_ACCESSEDFIXEDWIDTH, REG_DWORD, &AccessedFixedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_ACCESSEDWIDTH, REG_DWORD, &AccessedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_DFSTYPEFIXEDWIDTH, REG_DWORD, &DFSTypeFixedWidth, sizeof(DWORD));
  registry->SetValue(regKey, CONFIG_DFSTYPEWIDTH, REG_DWORD, &DFSTypeWidth, sizeof(DWORD));
}

void WINAPI
CPluginInterface::Configuration(HWND parent)
{
  CALL_STACK_MESSAGE1("CPluginInterface::Configuration()");
  OnConfiguration(parent);
}

void WINAPI
CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract *salamander)
{
  CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");
    std::string extension = "*.java;*.cpp;*.c;*.cc;*.cs;*.go;*.h;*.hpp;*.javascript;*.prolog;*.php;*.py;*.syslog;*.log;*.html;*.latex;*.bash;*.sh;*.in;*.css;*.ini;*.pkgconfig;*.bat;*.groovy;*.feature;*.hs;*.texi;*.texinfo;*.php4;*.php5;*.perl;*.ruby;*.bison;*.changelog;*.lua;*.csharp;*.C";  
  // zakladni cast:
  // salamander->AddCustomPacker("DemoPlug (Plugin)", "dop", FALSE);      // prvni verze: 'update==FALSE' (proste nejde o upgrade)
  // salamander->AddCustomUnpacker("DemoPlug (Plugin)", "*.dop", FALSE);  // prvni verze: 'update==FALSE' (proste nejde o upgrade)
  //salamander->AddCustomPacker("SyntaxHighlighting (Plugin)", "*", ConfigVersion < 4); // ve verzi 4 jsme zmenili priponu na "dop", proto je 'update==ConfigVersion < 4' (u starsich verzi nez 4 je treba updatnout priponu na "dop")
  salamander->AddCustomUnpacker("SyntaxHighlighting (Plugin)", extension.c_str(), ConfigVersion < 4);  // ve verzi 4 jsme zmenili pripony na *.dop a *.dop2, proto je 'update==ConfigVersion < 4' (u starsich verzi nez 4 je treba updatnout pripony na "*.dop;*.dop2")
  //salamander->AddPanelArchiver("*", FALSE, FALSE);
  salamander->AddViewer(extension.c_str(), FALSE);

#if !defined(ENABLE_DYNAMICMENUEXT)
  salamander->AddMenuItem(0, "C&onfiguration of Syntax Highlighting", SALHOTKEY('H', HOTKEYF_CONTROL | HOTKEYF_SHIFT), MENUCMD_CONFIG, FALSE, MENU_EVENT_TRUE, MENU_EVENT_DISK, MENU_SKILLLEVEL_ALL);
#endif // !defined(ENABLE_DYNAMICMENUEXT)

  CGUIIconListAbstract *iconList = SalamanderGUI->CreateIconList();
  iconList->Create(16, 16, 1);
  HICON hIcon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_FS), IMAGE_ICON, 16, 16, SalamanderGeneral->GetIconLRFlags());
  iconList->ReplaceIcon(0, hIcon);
  DestroyIcon(hIcon);
  salamander->SetIconListForGUI(iconList); // o destrukci iconlistu se postara Salamander

  //salamander->SetChangeDriveMenuItem("\Demoplug FS", 0);
  salamander->SetPluginIcon(0);
  salamander->SetPluginMenuAndToolbarIcon(0);


  if (ConfigVersion < 4)   // verze 4: zmena pripon "dmp/dmp2" na "dop/dop2"
  {
    // pridani novych pripon
    //salamander->AddPanelArchiver(extensionsForFunction1.c_str(), TRUE, TRUE);
    //salamander->AddViewer(extensionsForFunction.c_str(), FALSE);
    // vyhozeni starych pripon
    salamander->ForceRemovePanelArchiver("dmp");
    salamander->ForceRemovePanelArchiver("dmp2");
    salamander->ForceRemoveViewer("*.dmp");
    salamander->ForceRemoveViewer("*.dmp2");
  }
}

void WINAPI
CPluginInterface::ReleasePluginDataInterface(CPluginDataInterfaceAbstract *pluginData)
{
  if (pluginData != &ArcPluginDataInterface)  // jde-li o alokovany objekt FS dat, uvolnime ho
  {
    delete ((CPluginFSDataInterface *)pluginData);
  }
}

void WINAPI
CPluginInterface::ClearHistory(HWND parent)
{
  ViewerWindowQueue.BroadcastMessage(WM_USER_CLEARHISTORY, 0, 0);
}

void WINAPI
CPluginInterface::PasswordManagerEvent(HWND parent, int event)
{
  TRACE_I("PasswordManagerEvent(): event=" << event);
}

CPluginInterfaceForArchiverAbstract * WINAPI
CPluginInterface::GetInterfaceForArchiver()
{
  return &InterfaceForArchiver;
}

CPluginInterfaceForViewerAbstract * WINAPI
CPluginInterface::GetInterfaceForViewer()
{
  return &InterfaceForViewer;
}

CPluginInterfaceForMenuExtAbstract * WINAPI
CPluginInterface::GetInterfaceForMenuExt()
{
  return &InterfaceForMenuExt;
}

CPluginInterfaceForFSAbstract * WINAPI
CPluginInterface::GetInterfaceForFS()
{
  return &InterfaceForFS;
}

CPluginInterfaceForThumbLoaderAbstract * WINAPI
CPluginInterface::GetInterfaceForThumbLoader()
{
  return &InterfaceForThumbLoader;
}

void WINAPI
CPluginInterface::Event(int event, DWORD param)
{
  switch (event)
  {
    case PLUGINEVENT_COLORSCHANGED:
    {
      InitIconOverlays();

      // nutne DFSImageList != NULL, jinak by entry-point vratil chybu
      COLORREF bkColor = SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL);
      if (ImageList_GetBkColor(DFSImageList) != bkColor)
        ImageList_SetBkColor(DFSImageList, bkColor);
      break;
    }

    case PLUGINEVENT_SETTINGCHANGE:
    {
      ViewerWindowQueue.BroadcastMessage(WM_USER_SETTINGCHANGE, 0, 0);
      break;
    }
  }
}

