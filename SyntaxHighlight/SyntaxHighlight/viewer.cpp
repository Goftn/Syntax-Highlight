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


#include "ieviewer.h"
// ****************************************************************************
// SEKCE VIEWERU
// ****************************************************************************


#define FILTER_COUNT (CM_FILTER_LAST - CM_FILTER_FIRST + 1)

struct CButtonData
{
  int                      ImageIndex;     // zero base index
  WORD                     ToolTipResID;   // resID se stringem pro tooltip
  WORD                     ID;             // univerzalni Command
  CViewerWindowEnablerEnum Enabler;        // ridici promenna pro enablovani tlacitka
};

CWindowQueue ViewerWindowQueue("Syntax Viewers"); // seznam vsech oken viewru
CThreadQueue ThreadQueue("Syntax Viewers");       // seznam vsech threadu oken

HACCEL ViewerAccels = NULL;  // akceleratory pro viewer

void WINAPI HTMLHelpCallback(HWND hWindow, UINT helpID)
{
  SalamanderGeneral->OpenHtmlHelp(hWindow, HHCDisplayContext, helpID, FALSE);
}

BOOL InitViewer()
{
  if (!InitializeWinLib(PluginNameEN, DLLInstance)) return FALSE;
  SetWinLibStrings(LoadStr(IDS_INVALID_NUM), LoadStr(IDS_PLUGINNAME));
  SetupWinLibHelp(HTMLHelpCallback);
  ViewerAccels = LoadAccelerators(DLLInstance, MAKEINTRESOURCE(IDA_ACCELERATORS));
  return TRUE;
}

void ReleaseViewer()
{
  ReleaseWinLib(DLLInstance);
  DestroyAcceleratorTable(ViewerAccels);
  ViewerAccels = NULL;
}

class CViewerThread: public CThread
{
  protected:
    char Name[MAX_PATH];
    int Left, Top, Width, Height;
    UINT ShowCmd;
    BOOL AlwaysOnTop;
    BOOL ReturnLock;
    HANDLE Continue; // po naplneni nasledujicich navratovych hodnot se tento event prepne do "signaled"
    HANDLE *Lock;
    BOOL *LockOwner;
    BOOL *Success;

    int EnumFilesSourceUID;    // UID zdroje pro enumeraci souboru ve vieweru
    int EnumFilesCurrentIndex; // index prvniho souboru ve vieweru ve zdroji

  public:
    CViewerThread(const char *name, int left, int top, int width, int height,
                  UINT showCmd, BOOL alwaysOnTop, BOOL returnLock,
                  HANDLE *lock, BOOL *lockOwner, HANDLE contEvent,
                  BOOL *success, int enumFilesSourceUID,
                  int enumFilesCurrentIndex): CThread("DOP Viewer")
    {
      lstrcpyn(Name, name, MAX_PATH);
      Left = left;
      Top = top;
      Width = width;
      Height = height;
      ShowCmd = showCmd;
      AlwaysOnTop = alwaysOnTop;
      ReturnLock = returnLock;

      Continue = contEvent;
      Lock = lock;
      LockOwner = lockOwner;
      Success = success;

      EnumFilesSourceUID = enumFilesSourceUID;
      EnumFilesCurrentIndex = enumFilesCurrentIndex;
    }
    
    virtual unsigned Body();
};

unsigned
CViewerThread::Body()
{
  CALL_STACK_MESSAGE1("CViewerThread::Body()");
  TRACE_I("Begin");
  // ukazka padu aplikace
//  int *p = 0;
//  *p = 0;       // ACCESS VIOLATION !
    CViewerWindow* window = new CViewerWindow(EnumFilesSourceUID, EnumFilesCurrentIndex);
    if (window != NULL)
    {
        if (ReturnLock)
        {
            *Lock = window->GetLock();
            *LockOwner = TRUE;
        }
        CALL_STACK_MESSAGE1("ViewerThreadBody::CreateWindowEx");
        if (!ReturnLock || *Lock != NULL)
        {
            if (CfgSavePosition && CfgWindowPlacement.length != 0)
            {
                WINDOWPLACEMENT place = CfgWindowPlacement;
                // GetWindowPlacement cti Taskbar, takze pokud je Taskbar nahore nebo vlevo,
                // jsou hodnoty posunute o jeho rozmery. Provedeme korekci.
                RECT monitorRect;
                RECT workRect;
                SalamanderGeneral->MultiMonGetClipRectByRect(&place.rcNormalPosition, &workRect, &monitorRect);
                OffsetRect(&place.rcNormalPosition, workRect.left - monitorRect.left,
                    workRect.top - monitorRect.top);
                SalamanderGeneral->MultiMonEnsureRectVisible(&place.rcNormalPosition, TRUE);
                Left = place.rcNormalPosition.left;
                Top = place.rcNormalPosition.top;
                Width = place.rcNormalPosition.right - place.rcNormalPosition.left;
                Height = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
                ShowCmd = place.showCmd;
            }

            // POZNAMKA: na existujicim okne/dialogu je da top-most zaridit jednoduse:
            //SetWindowPos(HWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            char Viewer[256];
            char* fileName;
            fileName = strrchr(Name, '\\');
            if (fileName != NULL) {
                fileName++;
            }
            else {
                fileName = Name;
            }
            snprintf(Viewer, sizeof(Viewer), "Syntax Viewer - [%s]", fileName);
            if (window->CreateEx(AlwaysOnTop ? WS_EX_TOPMOST : 0,
                CWINDOW_CLASSNAME2,
                Viewer,
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                Left,
                Top,
                Width,
                Height,
                NULL,
                NULL,
                DLLInstance,
                window) != NULL)
            {
                HICON hMyIcon = LoadIcon(DLLInstance, MAKEINTRESOURCE(IDI_FS));
                SendMessage(window->HWindow, WM_SETICON, ICON_BIG, (LPARAM)hMyIcon);
                CALL_STACK_MESSAGE1("ViewerThreadBody::ShowWindow");
                ShowWindow(window->HWindow, ShowCmd);
                SetForegroundWindow(window->HWindow);
                UpdateWindow(window->HWindow);
                *Success = TRUE;
            }
            else
            {
                if (ReturnLock && *Lock != NULL) HANDLES(CloseHandle(*Lock));
            }

        }

        CALL_STACK_MESSAGE1("ViewerThreadBody::SetEvent");
        BOOL openFile = *Success;
        SetEvent(Continue);    // pustime dale hl. thread, od tohoto bodu nejsou platne nasl. promenne:  
        // pokud probehlo vse bez potizi, otevreme v okne pozadovany soubor
        if (openFile)
        {

            CALL_STACK_MESSAGE1("ViewerThreadBody::OpenFile");
            window->SyntaxHighlight(Name, FALSE);
             

            CALL_STACK_MESSAGE1("ViewerThreadBody::message-loop");
            // message loopa
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0))
            {

                if (msg.wParam == VK_ESCAPE) PostMessage(window->HWindow, WM_CLOSE, 0, 0);
                /*
                * Pokus o pøepínání souborù. asi by to chtìlo dìlat nìkde jinde než v message loope....
                if (msg.message == WM_KEYDOWN)
                {

                    BOOL ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    BOOL altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;

                    switch (msg.wParam)
                    {
                    case VK_BACK :
                    {
                        if (!shiftPressed && !altPressed)
                        {
                            if (ctrlPressed)  // zastarala hot-key: pouzivat Backspace (klavesy + prikazy v menu viz PictView, menu File/Other Files)
                            {
                            ok = SalamanderGeneral->GetPreviousFileNameForViewer(EnumFilesSourceUID,
                                                    &EnumFilesCurrentIndex,
                                                    Name, FALSE, TRUE,
                                                    fileName, &noMoreFiles,
                                                    &srcBusy);
                            }
                            if (ok) window->SyntaxHighlight(fileName); // mame nove jmeno
                            else
                            {
                            if (noMoreFiles) TRACE_I("Next/previous file does not exist.");
                            else
                            {
                                if (srcBusy) TRACE_I("Connected panel or Find window is busy, please try to repeat your request later.");
                                else
                                {
                                if (EnumFilesSourceUID == -1)
                                    TRACE_I("This service is not available from archive nor file system path.");
                                else
                                    TRACE_I("Connected panel or Find window does not contain original list of files.");
                                }
                            }
                            }

                        }
                        break;
                    }
                    case VK_SPACE :
                        {

                            if (!shiftPressed && !altPressed)
                            {
                                if (ctrlPressed)  // zastarala hot-key: pouzivat Backspace (klavesy + prikazy v menu viz PictView, menu File/Other Files)
                                {
                                    CALL_STACK_MESSAGE1("ViewerThreadBody::POST MESSAGE");
                                    MessageBox(window->HWindow, LPCSTR(msg.message + msg.wParam + msg.lParam), "POSTED MESSAGE", NULL);
                                    SendMessage(window->HWindow, msg.message, msg.wParam, msg.lParam);
                                }
                            }
                            break;
                    }

                    default:
                        // Pokud klávesa není zpracovávána zvláš, pokraèujeme dál
                        if (window->m_IEViewer.TranslateAccelerator(&msg) != S_OK)
                        {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                        break;
                    }
                }
                */
                if (!TranslateAccelerator(window->HWindow, ViewerAccels, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        if (window != NULL) {
            delete window;
        }
        TRACE_I("End");
        return 0;
    }
    else {
        return 1;
    }
  
}

std::vector<std::string> getFileExtensions(const std::string& filename) {
    std::vector<std::string> extensions;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string extension;
        if (std::getline(iss, extension, '=')) {
            extension.erase(std::remove_if(extension.begin(), extension.end(), ::isspace), extension.end());
            extensions.push_back(extension);
        }
    }
    return extensions;
}


bool isExtensionAllowed(const std::string& name, const std::vector<std::string>& extensions) {
    size_t pos = name.find_last_of('.');
    if (pos != std::string::npos) {
        std::string fileExtension = name.substr(pos + 1);
        for (const auto& extension : extensions) {
            if (fileExtension == extension) {
                return true;
            }
        }
    }
    return false;
}
BOOL WINAPI
CPluginInterfaceForViewer::ViewFile(const char *name, int left, int top, int width, int height,
                                    UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE *lock,
                                    BOOL *lockOwner, CSalamanderPluginViewerData *viewerData,
                                    int enumFilesSourceUID, int enumFilesCurrentIndex)
{
  HANDLE contEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
  if (contEvent == NULL)
  {
    TRACE_E("Unable to create Continue event.");
    return FALSE;
  }
  BOOL success = FALSE;

  char pluginDLLPath[MAX_PATH];
  if (GetModuleFileName(DLLInstance, pluginDLLPath, MAX_PATH) != 0)
  {

      char* lastSlash = strrchr(pluginDLLPath, '\\');
      if (lastSlash != NULL)
      {
          *lastSlash = '\0'; // file name deletion
      }
      std::string wokringDir = std::string(pluginDLLPath) + "\\src-highlite-rel_3.1.8\\src\\";

      std::string lang_map = wokringDir + "lang.map";
      std::vector<std::string> extensions = getFileExtensions(lang_map);
      if(g_HighlightEnabled && isExtensionAllowed(name, extensions)){//kontrola pripon
          // 'viewerData' se v DemoPlug nepouzivaji, jinak by bylo potreba predat hodnoty (ne odkazem)
          // do threadu vieweru...
          CViewerThread* t = new CViewerThread(name, left, top, width, height,
              showCmd, alwaysOnTop, returnLock, lock,
              lockOwner, contEvent, &success,
              enumFilesSourceUID, enumFilesCurrentIndex);
          if (t != NULL)
          {
              if (t->Create(ThreadQueue) != NULL) // thread se spustil
              {
                  t = NULL;   // zbytecne nulovani, jen pro poradek (ukazatel uz muze byt dealokovany)
                  WaitForSingleObject(contEvent, INFINITE); // pockame, az thread zpracuje predana data a vrati vysledky
              }
              else
                  delete t; // pri chybe je potreba dealokovat objekt threadu
          }
      }
      else {
          int err;
          CSalamanderPluginViewerData viewerData;
          viewerData.Size = sizeof(viewerData);
          viewerData.FileName = name;
          BOOL ok = SalamanderGeneral->ViewFileInPluginViewer(NULL, &viewerData, FALSE, NULL, NULL, err);
          success = TRUE;
      }
  }
  HANDLES(CloseHandle(contEvent));
  return success;
}

//
// ****************************************************************************
// CRendererWindow
//

CRendererWindow::CRendererWindow()
 : CWindow(ooStatic)
{
}

CRendererWindow::~CRendererWindow()
{
}


LRESULT
CRendererWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_COMMAND:
    {
      SendMessage(Viewer->HWindow, WM_COMMAND, wParam, lParam);  // forward commands from context menu 
      break;
    }


  }
  return CWindow::WindowProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CViewerWindow
//

CViewerWindow::CViewerWindow(int enumFilesSourceUID, int enumFilesCurrentIndex): CWindow(ooStatic)
{
  Lock = NULL;
  Name[0] = 0;

  EnumFilesSourceUID = enumFilesSourceUID;
  EnumFilesCurrentIndex = enumFilesCurrentIndex;

  ZeroMemory(Enablers, sizeof(Enablers));
}

void
CViewerWindow::LayoutWindows()
{
  RECT r;
  GetClientRect(HWindow, &r);
  SendMessage(HWindow, WM_SIZE, SIZE_RESTORED,
              MAKELONG(r.right - r.left, r.bottom - r.top));
}


LRESULT
CViewerWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_CREATE:
  {
        if (!m_IEViewer.CreateSite(HWindow)) {
            return -1;
        }
      
      SetFocus(m_IEViewer.HWindow);
      ViewerWindowQueue.Add(new CWindowQueueItem(HWindow));

      break;
  }
    case WM_CLOSE:
    {
        if (!m_IEViewer.CanClose())
            return 0;
        break;
    }

    case WM_DESTROY:
    {
      if (Lock != NULL)
      {
          SetEvent(Lock);
          Lock = NULL;
      }
      if (m_IEViewer.HWindow != NULL)
          m_IEViewer.CloseSite();


      ViewerWindowQueue.Remove(HWindow);

      if (Lock != NULL)
      {
        SetEvent(Lock);
        Lock = NULL;
      }
      PostQuitMessage(0);
      break;
    }
    case WM_NOTIFY:
    {
      LPNMHDR lphdr = (LPNMHDR)lParam;
      if (lphdr->code == RBN_AUTOSIZE)
      {
        LayoutWindows();
        return 0;
      }
      break;
    }

    case WM_SIZE:
    {
        RECT r;
        GetClientRect(HWindow, &r);
        if (m_IEViewer.HWindow != NULL) {
            SetWindowPos(m_IEViewer.HWindow, HWND_TOP, 0, 0, LOWORD(lParam), HIWORD(lParam), 0);
        }

      break;
    }


    case WM_USER_VIEWERCFGCHNG:
    {
      TRACE_I("CViewerWindow::WindowProc - config has changed");
      g_HighlightEnabled = (BOOL)wParam;
      InvalidateRect(HWindow, NULL, TRUE);
      return 0;
    }

    case WM_ACTIVATE:
    {
      if (!LOWORD(wParam))
      {
        // hlavni okno pri prepnuti z viewru nebude delat refresh
        SalamanderGeneral->SkipOneActivateRefresh();
      }
      break;
    }


    case WM_COMMAND:
    {

      switch (LOWORD(wParam))
      {

        case CM_VIEWER_EXIT:
        {
          PostMessage(HWindow, WM_CLOSE, 0, 0);
          break;
        }

        case CM_VIEWER_CFG:
        {
          OnConfiguration(HWindow);
          break;
        }

        case CM_VIEWER_ABOUT:
        {
          OnAbout(HWindow);
          break;
        }
      }
      return 0;
    }


  }
  return CWindow::WindowProc(uMsg, wParam, lParam);
}

HANDLE
CViewerWindow::GetLock()
{
  if (Lock == NULL) Lock = NOHANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
  return Lock;
}


void
CViewerWindow::SyntaxHighlight(const char *name, BOOL setLock)
{
    if (name != NULL && *name != '\0')
    {
    std::string outputFile;
    char pluginDLLPath[MAX_PATH];
    if (GetModuleFileName(DLLInstance, pluginDLLPath, MAX_PATH) != 0)
    {
    
            char* lastSlash = strrchr(pluginDLLPath, '\\');
            if (lastSlash != NULL)
            {
                *lastSlash = '\0'; // file name deletion
            }

        std::string sourceHighlightCmd = std::string(pluginDLLPath) + "\\src-highlite-rel_3.1.8\\src\\source-highlight.exe";
        std::string wokringDir = std::string(pluginDLLPath) + "\\src-highlite-rel_3.1.8\\src\\";
        std::string inputFile = name;
        std::string lang_map = wokringDir + "lang.map";
        outputFile = std::string(pluginDLLPath) + "\\out.html"; 

        std::string parameters = " -i \"" + inputFile + "\" -o \"" + outputFile + "\"";

        SHELLEXECUTEINFO sei = { 0 };
        sei.cbSize = sizeof(SHELLEXECUTEINFO);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = "open";
        sei.lpFile = sourceHighlightCmd.c_str(); // path to script
        sei.lpParameters = parameters.c_str(); // arguments
        sei.lpDirectory = wokringDir.c_str(); // wokring dir
        sei.nShow = SW_HIDE;
        ShellExecuteEx(&sei);
        if (ShellExecuteEx(&sei) == FALSE) {
            // Handle error
            MessageBox(HWindow, "Error in Source-highlight execution","Error", NULL);
            PostMessage(HWindow, WM_CLOSE, 0, 0);
            return;
        }


        /* from https://forums.codeguru.com/showthread.php?382333-RESOLVED-ShellExecuteEx-requesting-the-return-code */
        if (sei.hProcess != NULL) {
            WaitForSingleObject(sei.hProcess, INFINITE); // wait for syntax highlight
            DWORD exitCode;
            if (GetExitCodeProcess(sei.hProcess, &exitCode)) {
                if (exitCode == STILL_ACTIVE) {
                    MessageBox(HWindow, "Syntax highlighting program is still running", "Warning", MB_OK);
                }
            }
            CloseHandle(sei.hProcess);
        }
        else {
            MessageBox(HWindow, "Couldn't run Syntax Highlighting", "Error", MB_OK);
            PostMessage(HWindow, WM_CLOSE, 0, 0);
            return;
        }
            
        if (name != NULL && *name != '0')
        {
            if (m_IEViewer.HWindow != NULL) {
                m_IEViewer.Navigate(outputFile.c_str(), NULL);
            }
        }
        else {
            MessageBox(HWindow, "Coudn't load file", "Error.", NULL);
            PostMessage(HWindow, WM_CLOSE, 0, 0);
            return;
        }
        
    }

  }
  InvalidateRect(HWindow, NULL, TRUE);

}
