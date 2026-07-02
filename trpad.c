#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <richedit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>

#define _NO_CRT_STDIO_INLINE

#define WindowWidth      800
#define WindowHeight     640
#define IDC_EDIT         1001
#ifndef EM_EXLIMITTEXT
#define EM_EXLIMITTEXT   (WM_USER+53)
#endif
#ifndef EM_SETCHARFORMAT
#define EM_SETCHARFORMAT (WM_USER+68)
#endif
#ifndef EM_SETEVENTMASK
#define EM_SETEVENTMASK  (WM_USER+69)
#endif
#ifndef EM_SETTARGETDEVICE
#define EM_SETTARGETDEVICE (WM_USER+72)
#endif
#ifndef SCF_ALL
#define SCF_ALL          0x00000004
#endif
#define ENM_CHANGE       0x00000001
#define CFM_FACE         0x20000000
#define MAX_CMD_PATH     128
#define MAX_TITLE        128
#define IDM_SAVE         0x0E100
#define IDM_FILE_NEW     0x0E200
#define IDM_FILE_OPEN    0x0E202
#define IDM_FILE_SAVEAS  0x0E203
#define IDM_FILE_PRINT   0x0E204
#define IDM_FILE_PAGESETUP 0x0E205
#define IDM_FILE_EXIT    0x0E201
#define IDM_EDIT_UNDO    0x0E210
#define IDM_EDIT_CUT     0x0E211
#define IDM_EDIT_COPY    0x0E212
#define IDM_EDIT_PASTE   0x0E213
#define IDM_EDIT_DELETE  0x0E214
#define IDM_EDIT_SELALL  0x0E215
#define IDM_EDIT_TIME    0x0E216
#define IDM_EDIT_FIND    0x0E217
#define IDM_EDIT_FINDNEXT 0x0E218
#define IDM_EDIT_REPLACE 0x0E219
#define IDM_EDIT_GOTO    0x0E21A
#define IDM_FMT_WRAP     0x0E220
#define IDM_FMT_FONT     0x0E221
#define IDM_VIEW_STATUS  0x0E230
#define IDM_HELP_VIEWHELP 0x0E241
#define IDM_HELP_ABOUT   0x0E240
#define IDC_GOEDIT       1000
#define SBHEIGHT         20
#ifndef EM_FINDTEXTEXA
#define EM_FINDTEXTEXA   (WM_USER + 84)
#endif
#ifndef MSGFILTERA
typedef struct tagMSGFILTERA {
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
} MSGFILTERA;
#endif

static HWND hMain = NULL;
static HWND hEdit = NULL;
static HWND hStatus = NULL;
static HINSTANCE hInst = NULL;
static char CmdFile[MAX_CMD_PATH] = {0};
static char TitleBuf[MAX_TITLE] = {0};
static char StatusBuf[48] = {0};
static bool fDirty = false;
static bool fWrap = true;
static bool fStatus = true;
static HWND hFindDlg = NULL;
static UINT uFindMsg = 0;
static FINDREPLACEA fr;
static char FindWhat[128] = {0};
static char ReplaceWith[128] = {0};

static const char UntitledText[] = "Untitled";
static const char NotepadTail[] = " - TinyRetroPad - C Version";
static const char FileFilter[] = "All Files\0*.*\0";
static const char AboutCap[] = "TinyRetroPad";
static const char AboutText[] = "TinyRetroPad - tiny notepad-style editor";
static const char SaveCap[] = "TinyRetroPad";
static const char SaveAskText[] = "Save changes?";
static const char HelpUrl[] = "https://github.com/davepl";
static const char OpenVerb[] = "open";
static const char LnColFmt[] = "  Ln %d, Col %d";

static void ApplyTitle(void);
static void BuildTitle(void);
static void ParseStartupFile(void);
static bool LoadStartupFile(void);
static bool SaveFile(void);
static bool MaybeSaveChanges(void);
static void InsertTimeDate(void);
static void ToggleWrap(void);
static void ChooseFontDlg(void);
static void InitFR(void);
static int DoFindNext(void);
static void DoReplaceOne(void);
static void DoReplaceAll(void);
static void OnFindReplaceMsg(void);
static void PrintDoc(void);
static void UpdateStatus(void);
static void RelayoutClient(void);
static int GoToDlg(void);
static INT_PTR CALLBACK GoToProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void ShowContextMenu(HWND hWndOwner);
static void AppendDisabled(HMENU hMenu, LPCSTR pText);
static void AppendEnabled(HMENU hMenu, UINT uID, LPCSTR pText);
static void CreateNotepadMenus(HWND hWnd);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void BuildTitle(void)
{
    const char *base = CmdFile;
    const char *tail = NULL;
    if (*base == '\0') {
        base = UntitledText;
    } else {
        const char *p = base;
        tail = base;
        while (*p) {
            if (*p == '\\' || *p == '/' || *p == ':') {
                tail = p + 1;
            }
            p++;
        }
        base = tail;
    }

    wsprintfA(TitleBuf, "%s%s", base, NotepadTail);

    //snprintf(TitleBuf, MAX_TITLE, "%s%s", base, NotepadTail);
}

static void ApplyTitle(void)
{
    BuildTitle();
    SetWindowTextA(hMain, TitleBuf);
}

static void ParseStartupFile(void)
{
    const char *cmd = GetCommandLineA();
    if (!cmd || !*cmd) {
        CmdFile[0] = '\0';
        return;
    }

    if (*cmd == '"') {
        cmd++;
        while (*cmd && *cmd != '"') {
            cmd++;
        }
        if (*cmd == '"') {
            cmd++;
        }
    } else {
        while (*cmd && *cmd != ' ' && *cmd != '\t') {
            cmd++;
        }
    }

    while (*cmd == ' ' || *cmd == '\t') {
        cmd++;
    }

    if (!*cmd) {
        CmdFile[0] = '\0';
        return;
    }

    size_t dest = 0;
    if (*cmd == '"') {
        cmd++;
        while (*cmd && *cmd != '"' && dest < MAX_CMD_PATH - 1) {
            CmdFile[dest++] = *cmd++;
        }
    } else {
        while (*cmd && *cmd != ' ' && *cmd != '\t' && dest < MAX_CMD_PATH - 1) {
            CmdFile[dest++] = *cmd++;
        }
    }
    CmdFile[dest] = '\0';
}

static bool LoadStartupFile(void)
{
    if (!CmdFile[0]) {
        return false;
    }

    HANDLE hFile = CreateFileA(CmdFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return false;
    }

    char *buffer = malloc(fileSize + 1);
    if (!buffer) {
        CloseHandle(hFile);
        return false;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
        free(buffer);
        CloseHandle(hFile);
        return false;
    }

    buffer[bytesRead] = '\0';
    SetWindowTextA(hEdit, buffer);

    CHARFORMATW fmt = {0};
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_FACE;
    wcscpy_s(fmt.szFaceName, LF_FACESIZE, L"Courier");
    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&fmt);

    free(buffer);
    CloseHandle(hFile);
    return true;
}

static bool SaveFile(void)
{
    int textLength = (int)SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    if (textLength < 0) {
        return false;
    }

    char *buffer = malloc(textLength + 1);
    if (!buffer) {
        return false;
    }

    SendMessageA(hEdit, WM_GETTEXT, textLength + 1, (LPARAM)buffer);

    HANDLE hFile = CreateFileA(CmdFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        free(buffer);
        return false;
    }

    DWORD bytesWritten = 0;
    WriteFile(hFile, buffer, textLength, &bytesWritten, NULL);
    CloseHandle(hFile);
    free(buffer);

    if (bytesWritten != (DWORD)textLength) {
        return false;
    }

    fDirty = false;
    ApplyTitle();

    return true;
}

static bool MaybeSaveChanges(void)
{
    if (!fDirty) {
        return true;
    }

    int result = MessageBoxA(hMain, SaveAskText, SaveCap, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (result == IDCANCEL) {
        return false;
    }
    if (result == IDNO) {
        return true;
    }

    if (!CmdFile[0]) {
        OPENFILENAMEA ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hMain;
        ofn.lpstrFilter = FileFilter;
        ofn.lpstrFile = CmdFile;
        ofn.nMaxFile = MAX_CMD_PATH;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileNameA(&ofn)) {
            return false;
        }
    }

    return SaveFile();
}

static void InsertTimeDate(void)
{
    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);

    char dateBuf[32] = {0};
    char timeBuf[32] = {0};

    GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &sysTime, NULL, dateBuf, (int)sizeof(dateBuf));
    GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, timeBuf, (int)sizeof(timeBuf));

    SendMessageA(hEdit, EM_REPLACESEL, TRUE, (LPARAM)dateBuf);
    SendMessageA(hEdit, EM_REPLACESEL, TRUE, (LPARAM)" ");
    SendMessageA(hEdit, EM_REPLACESEL, TRUE, (LPARAM)timeBuf);
}

static void ToggleWrap(void)
{
    if (fWrap) {
        fWrap = false;
        SendMessageA(hEdit, EM_SETTARGETDEVICE, 0, (LPARAM)-1);
    } else {
        fWrap = true;
        SendMessageA(hEdit, EM_SETTARGETDEVICE, 0, 0);
    }
}

static void ChooseFontDlg(void)
{
    LOGFONTW lf = {0};
    CHOOSEFONTW cf = {0};
    CHARFORMATW fmt = {0};

    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hMain;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_EFFECTS;

    if (!ChooseFontW(&cf)) {
        return;
    }

    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC;
    fmt.dwEffects = 0;
    if (lf.lfWeight >= 700) {
        fmt.dwEffects |= CFE_BOLD;
    }
    if (lf.lfItalic) {
        fmt.dwEffects |= CFE_ITALIC;
    }
    fmt.yHeight = cf.iPointSize * 2;
    wcscpy_s(fmt.szFaceName, LF_FACESIZE, lf.lfFaceName);

    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&fmt);
}

static void InitFR(void)
{
    ZeroMemory(&fr, sizeof(fr));
    fr.lStructSize = sizeof(fr);
    fr.hwndOwner = hMain;
    fr.lpstrFindWhat = FindWhat;
    fr.wFindWhatLen = sizeof(FindWhat);
    fr.lpstrReplaceWith = ReplaceWith;
    fr.wReplaceWithLen = sizeof(ReplaceWith);
    fr.Flags = FR_DOWN;
}

static int DoFindNext(void)
{
    FINDTEXTEXA ft = {0};
    CHARRANGE cr = {0};

    SendMessageA(hEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ft.chrg.cpMin = cr.cpMax;
    ft.chrg.cpMax = -1;
    ft.lpstrText = FindWhat;

    UINT flags = (fr.Flags & FR_MATCHCASE) | FR_DOWN;
    LRESULT pos = SendMessageA(hEdit, EM_FINDTEXTEXA, flags, (LPARAM)&ft);
    if (pos == -1) {
        return 0;
    }

    SendMessageA(hEdit, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
    SendMessageA(hEdit, EM_SCROLLCARET, 0, 0);
    return 1;
}

static void DoReplaceOne(void)
{
    SendMessageA(hEdit, EM_REPLACESEL, TRUE, (LPARAM)ReplaceWith);
    DoFindNext();
}

static void DoReplaceAll(void)
{
    CHARRANGE cr = {0};
    SendMessageA(hEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
    while (DoFindNext()) {
        SendMessageA(hEdit, EM_REPLACESEL, TRUE, (LPARAM)ReplaceWith);
    }
}

static void OnFindReplaceMsg(void)
{
    if (fr.Flags & FR_DIALOGTERM) {
        hFindDlg = NULL;
        return;
    }
    if (fr.Flags & FR_REPLACEALL) {
        DoReplaceAll();
        return;
    }
    if (fr.Flags & FR_REPLACE) {
        DoReplaceOne();
        return;
    }
    DoFindNext();
}

static void PrintDoc(void)
{
    PRINTDLGA pd = {0};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hMain;
    pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;

    if (!PrintDlgA(&pd)) {
        return;
    }

    HDC hPrnDC = pd.hDC;
    DOCINFOA docInfo = {0};
    docInfo.cbSize = sizeof(docInfo);
    docInfo.lpszDocName = "TinyRetroPad - C Version";
    StartDocA(hPrnDC, &docInfo);

    FORMATRANGE fmt = {0};
    fmt.hdc = hPrnDC;
    fmt.hdcTarget = hPrnDC;

    int horz = GetDeviceCaps(hPrnDC, HORZRES);
    int logpixx = GetDeviceCaps(hPrnDC, LOGPIXELSX);
    fmt.rc.right = fmt.rcPage.right = (horz * 1440) / logpixx;

    int vert = GetDeviceCaps(hPrnDC, VERTRES);
    int logpixy = GetDeviceCaps(hPrnDC, LOGPIXELSY);
    fmt.rc.bottom = fmt.rcPage.bottom = (vert * 1440) / logpixy;

    int txtLen = (int)SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    fmt.chrg.cpMin = 0;
    fmt.chrg.cpMax = txtLen;

    do {
        StartPage(hPrnDC);
        LRESULT next = SendMessageA(hEdit, EM_FORMATRANGE, TRUE, (LPARAM)&fmt);
        fmt.chrg.cpMin = (LONG)next;
        EndPage(hPrnDC);
    } while (fmt.chrg.cpMin < txtLen);

    SendMessageA(hEdit, EM_FORMATRANGE, FALSE, 0);
    EndDoc(hPrnDC);
    DeleteDC(hPrnDC);
}

static void UpdateStatus(void)
{
    if (!fStatus) {
        return;
    }

    CHARRANGE cr = {0};
    SendMessageA(hEdit, EM_EXGETSEL, 0, (LPARAM)&cr);

    int line = (int)SendMessageA(hEdit, EM_EXLINEFROMCHAR, 0, cr.cpMax);
    int lineStart = (int)SendMessageA(hEdit, EM_LINEINDEX, line, 0);
    int col = cr.cpMax - lineStart + 1;

    if (line < 0) {
        line = 0;
    }
    if (lineStart < 0) {
        lineStart = 0;
    }
    if (col < 1) {
        col = 1;
    }

    wsprintfA(StatusBuf, LnColFmt, line + 1, col);
    SetWindowTextA(hStatus, StatusBuf);
}

static void RelayoutClient(void)
{
    SendMessageA(hMain, WM_SIZE, 0, 0);
}

static int GoToDlg(void)
{
    BYTE dlgTemplate[256];
    ZeroMemory(dlgTemplate, sizeof(dlgTemplate));

    DLGTEMPLATE *dlg = (DLGTEMPLATE *)dlgTemplate;
    dlg->style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlg->dwExtendedStyle = 0;
    dlg->cdit = 2;
    dlg->x = 0;
    dlg->y = 0;
    dlg->cx = 150;
    dlg->cy = 46;

    BYTE *p = (BYTE *)(dlg + 1);
    *((WORD *)p) = 0; p += 2;
    *((WORD *)p) = 0; p += 2;
    strcpy((char *)p, "Go To");
    p += strlen("Go To") + 1;
    while (((ULONG_PTR)p & 3) != 0) {
        *p++ = 0;
    }

    DLGITEMTEMPLATE *item = (DLGITEMTEMPLATE *)p;
    item->style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP;
    item->dwExtendedStyle = 0;
    item->x = 7;
    item->y = 7;
    item->cx = 136;
    item->cy = 12;
    item->id = IDC_GOEDIT;
    p = (BYTE *)(item + 1);
    *((WORD *)p) = 0xFFFF; p += 2;
    *((WORD *)p) = 0x0081; p += 2;
    *((WORD *)p) = 0; p += 2;
    *((WORD *)p) = 0; p += 2;
    while (((ULONG_PTR)p & 3) != 0) {
        *p++ = 0;
    }

    item = (DLGITEMTEMPLATE *)p;
    item->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP;
    item->dwExtendedStyle = 0;
    item->x = 50;
    item->y = 26;
    item->cx = 50;
    item->cy = 14;
    item->id = IDOK;
    p = (BYTE *)(item + 1);
    *((WORD *)p) = 0xFFFF; p += 2;
    *((WORD *)p) = 0x0080; p += 2;
    strcpy((char *)p, "OK");
    p += strlen("OK") + 1;
    *((WORD *)p) = 0; p += 2;
    while (((ULONG_PTR)p & 3) != 0) {
        *p++ = 0;
    }

    int result = DialogBoxIndirectParamA(hInst, (LPCDLGTEMPLATE)dlgTemplate, hMain, GoToProc, 0);
    return result;
}

static INT_PTR CALLBACK GoToProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;
    if (uMsg == WM_INITDIALOG) {
        return TRUE;
    }
    if (uMsg == WM_COMMAND) {
        int id = LOWORD(wParam);
        if (id == IDOK) {
            BOOL translated = FALSE;
            int line = GetDlgItemInt(hDlg, IDC_GOEDIT, &translated, FALSE);
            if (translated) {
                EndDialog(hDlg, line);
                return TRUE;
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        if (id == IDCANCEL) {
            EndDialog(hDlg, 0);
            return TRUE;
        }
    }
    return FALSE;
}

static void ShowContextMenu(HWND hWndOwner)
{
    HMENU hCtx = CreatePopupMenu();
    if (!hCtx) {
        return;
    }

    AppendEnabled(hCtx, IDM_EDIT_UNDO, "Undo");
    AppendDisabled(hCtx, NULL);
    AppendEnabled(hCtx, IDM_EDIT_CUT, "Cut");
    AppendEnabled(hCtx, IDM_EDIT_COPY, "Copy");
    AppendEnabled(hCtx, IDM_EDIT_PASTE, "Paste");
    AppendEnabled(hCtx, IDM_EDIT_DELETE, "Delete");
    AppendDisabled(hCtx, NULL);
    AppendEnabled(hCtx, IDM_EDIT_SELALL, "Select All");

    DWORD pos = GetMessagePos();
    POINT pt = { LOWORD(pos), HIWORD(pos) };
    TrackPopupMenu(hCtx, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWndOwner, NULL);
    DestroyMenu(hCtx);
}

static void AppendDisabled(HMENU hMenu, LPCSTR pText)
{
    if (pText) {
        AppendMenuA(hMenu, MF_STRING | MF_GRAYED, 0, pText);
    } else {
        AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    }
}

static void AppendEnabled(HMENU hMenu, UINT uID, LPCSTR pText)
{
    AppendMenuA(hMenu, MF_STRING, uID, pText);
}

static void CreateNotepadMenus(HWND hWnd)
{
    HMENU hMenuBar = CreateMenu();
    if (!hMenuBar) {
        return;
    }

    HMENU hPopup = CreatePopupMenu();
    AppendEnabled(hPopup, IDM_FILE_NEW, "&New");
    AppendEnabled(hPopup, IDM_FILE_OPEN, "&Open...");
    AppendEnabled(hPopup, IDM_SAVE, "&Save");
    AppendEnabled(hPopup, IDM_FILE_SAVEAS, "Save &As...");
    AppendDisabled(hPopup, NULL);
    AppendEnabled(hPopup, IDM_FILE_PAGESETUP, "Page Set&up...");
    AppendEnabled(hPopup, IDM_FILE_PRINT, "&Print...");
    AppendDisabled(hPopup, NULL);
    AppendEnabled(hPopup, IDM_FILE_EXIT, "E&xit");
    AppendMenuA(hMenuBar, MF_POPUP | MF_STRING, (UINT_PTR)hPopup, "&File");

    hPopup = CreatePopupMenu();
    AppendEnabled(hPopup, IDM_EDIT_UNDO, "&Undo");
    AppendDisabled(hPopup, NULL);
    AppendEnabled(hPopup, IDM_EDIT_CUT, "Cu&t");
    AppendEnabled(hPopup, IDM_EDIT_COPY, "&Copy");
    AppendEnabled(hPopup, IDM_EDIT_PASTE, "&Paste");
    AppendEnabled(hPopup, IDM_EDIT_DELETE, "De&lete");
    AppendDisabled(hPopup, NULL);
    AppendEnabled(hPopup, IDM_EDIT_FIND, "&Find...");
    AppendEnabled(hPopup, IDM_EDIT_FINDNEXT, "Find &Next");
    AppendEnabled(hPopup, IDM_EDIT_REPLACE, "&Replace...");
    AppendEnabled(hPopup, IDM_EDIT_GOTO, "&Go To...");
    AppendDisabled(hPopup, NULL);
    AppendEnabled(hPopup, IDM_EDIT_SELALL, "Select &All");
    AppendEnabled(hPopup, IDM_EDIT_TIME, "Time/&Date");
    AppendMenuA(hMenuBar, MF_POPUP | MF_STRING, (UINT_PTR)hPopup, "&Edit");

    hPopup = CreatePopupMenu();
    AppendEnabled(hPopup, IDM_FMT_WRAP, "&Word Wrap");
    AppendEnabled(hPopup, IDM_FMT_FONT, "&Font...");
    AppendMenuA(hMenuBar, MF_POPUP | MF_STRING, (UINT_PTR)hPopup, "F&ormat");

    hPopup = CreatePopupMenu();
    AppendEnabled(hPopup, IDM_VIEW_STATUS, "&Status Bar");
    AppendMenuA(hMenuBar, MF_POPUP | MF_STRING, (UINT_PTR)hPopup, "&View");

    hPopup = CreatePopupMenu();
    AppendEnabled(hPopup, IDM_HELP_VIEWHELP, "&View Help");
    AppendDisabled(hPopup, NULL);
    AppendEnabled(hPopup, IDM_HELP_ABOUT, "&About TinyRetroPad");
    AppendMenuA(hMenuBar, MF_POPUP | MF_STRING, (UINT_PTR)hPopup, "&Help");

    SetMenu(hWnd, hMenuBar);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE: {
        const DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
        hEdit = CreateWindowExA(0, "RICHEDIT50W", "", style, 0, 0, 0, 0, hWnd, NULL, NULL, NULL);
        if (!hEdit) {
            return -1;
        }

        SendMessageA(hEdit, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_MOUSEEVENTS | ENM_SELCHANGE);
        SendMessageA(hEdit, EM_EXLIMITTEXT, 0, 0x7FFFFFFE);

        CHARFORMATW richFont = {0};
        richFont.cbSize = sizeof(richFont);
        richFont.dwMask = CFM_FACE;
        wcscpy_s(richFont.szFaceName, LF_FACESIZE, L"Courier");
        SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&richFont);

        HMENU sysMenu = GetSystemMenu(hWnd, FALSE);
        if (sysMenu) {
            AppendMenuA(sysMenu, MF_STRING, IDM_SAVE, "Save");
        }

        CreateNotepadMenus(hWnd);

        hStatus = CreateWindowExA(WS_EX_STATICEDGE, "STATIC", "", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, hInst, NULL);
        UpdateStatus();
        return 0;
    }

    case WM_SYSCOMMAND:
        if ((UINT)wParam == IDM_SAVE) {
            SaveFile();
            return 0;
        }
        break;

    case WM_COMMAND: {
        WORD code = HIWORD(wParam);
        WORD id = LOWORD(wParam);

        if (code == EN_CHANGE) {
            if (!fDirty) {
                fDirty = true;
                ApplyTitle();
            }
            return 0;
        }

        switch (id) {
        case IDM_FILE_NEW:
            if (MaybeSaveChanges()) {
                CmdFile[0] = '\0';
                SetWindowTextA(hEdit, "");
                fDirty = false;
                ApplyTitle();
            }
            return 0;
        case IDM_FILE_OPEN:
            if (MaybeSaveChanges()) {
                OPENFILENAMEA ofn = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hMain;
                ofn.lpstrFilter = FileFilter;
                ofn.lpstrFile = CmdFile;
                ofn.nMaxFile = MAX_CMD_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
                if (GetOpenFileNameA(&ofn)) {
                    LoadStartupFile();
                    fDirty = false;
                    ApplyTitle();
                }
            }
            return 0;
        case IDM_SAVE:
            if (!CmdFile[0]) {
                OPENFILENAMEA ofn = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hMain;
                ofn.lpstrFilter = FileFilter;
                ofn.lpstrFile = CmdFile;
                ofn.nMaxFile = MAX_CMD_PATH;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
                if (!GetSaveFileNameA(&ofn)) {
                    return 0;
                }
            }
            SaveFile();
            return 0;
        case IDM_FILE_SAVEAS: {
            OPENFILENAMEA ofn = {0};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hMain;
            ofn.lpstrFilter = FileFilter;
            ofn.lpstrFile = CmdFile;
            ofn.nMaxFile = MAX_CMD_PATH;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
            if (GetSaveFileNameA(&ofn)) {
                SaveFile();
            }
            return 0;
        }
        case IDM_FILE_PRINT:
            PrintDoc();
            return 0;
        case IDM_FILE_PAGESETUP: {
            PAGESETUPDLGA psd = {0};
            psd.lStructSize = sizeof(psd);
            psd.hwndOwner = hMain;
            PageSetupDlgA(&psd);
            return 0;
        }
        case IDM_FILE_EXIT:
            if (MaybeSaveChanges()) {
                DestroyWindow(hWnd);
            }
            return 0;
        case IDM_EDIT_UNDO:
            SendMessageA(hEdit, WM_UNDO, 0, 0);
            return 0;
        case IDM_EDIT_CUT:
            SendMessageA(hEdit, WM_CUT, 0, 0);
            return 0;
        case IDM_EDIT_COPY:
            SendMessageA(hEdit, WM_COPY, 0, 0);
            return 0;
        case IDM_EDIT_PASTE:
            SendMessageA(hEdit, WM_PASTE, 0, 0);
            return 0;
        case IDM_EDIT_DELETE:
            SendMessageA(hEdit, WM_CLEAR, 0, 0);
            return 0;
        case IDM_EDIT_SELALL:
            SetFocus(hEdit);
            SendMessageA(hEdit, EM_SETSEL, 0, (LPARAM)-1);
            return 0;
        case IDM_EDIT_TIME:
            SetFocus(hEdit);
            InsertTimeDate();
            return 0;
        case IDM_EDIT_FIND:
            InitFR();
            hFindDlg = FindTextA(&fr);
            return 0;
        case IDM_EDIT_FINDNEXT:
            DoFindNext();
            return 0;
        case IDM_EDIT_REPLACE:
            InitFR();
            hFindDlg = ReplaceTextA(&fr);
            return 0;
        case IDM_EDIT_GOTO: {
            int line = GoToDlg();
            if (line > 0) {
                int index = (int)SendMessageA(hEdit, EM_LINEINDEX, line - 1, 0);
                if (index != -1) {
                    CHARRANGE cr = { index, index };
                    SendMessageA(hEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
                    SendMessageA(hEdit, EM_SCROLLCARET, 0, 0);
                    SetFocus(hEdit);
                }
            }
            return 0;
        }
        case IDM_FMT_WRAP:
            ToggleWrap();
            return 0;
        case IDM_FMT_FONT:
            ChooseFontDlg();
            return 0;
        case IDM_VIEW_STATUS:
            fStatus = !fStatus;
            ShowWindow(hStatus, fStatus ? SW_SHOW : SW_HIDE);
            RelayoutClient();
            UpdateStatus();
            return 0;
        case IDM_HELP_ABOUT:
            MessageBoxA(hWnd, AboutText, AboutCap, MB_OK | MB_ICONINFORMATION);
            return 0;
        case IDM_HELP_VIEWHELP:
            ShellExecuteA(NULL, OpenVerb, HelpUrl, NULL, NULL, SW_SHOWNORMAL);
            return 0;
        }
        break;
    }

    case WM_NOTIFY: {
        NMHDR *hdr = (NMHDR *)lParam;
        if (hdr->code == EN_SELCHANGE) {
            UpdateStatus();
            return 0;
        }
        if (hdr->code == EN_MSGFILTER) {
            MSGFILTERA *mf = (MSGFILTERA *)lParam;
            if (mf->msg == WM_RBUTTONUP) {
                ShowContextMenu(hWnd);
                return 0;
            }
        }
        break;
    }

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        int editHeight = height;
        if (fStatus) {
            editHeight -= SBHEIGHT;
            SetWindowPos(hStatus, NULL, 0, editHeight, width, SBHEIGHT, SWP_NOZORDER);
        }
        SetWindowPos(hEdit, NULL, 0, 0, width, editHeight, SWP_NOZORDER);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        if (uMsg == uFindMsg) {
            OnFindReplaceMsg();
            return 0;
        }
        break;
    }

    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

void MyMain(void) 
{
    WinMain(GetModuleHandleA(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}



int __stdcall WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;
    hInst = hInstance;
    LoadLibraryA("Msftedit.dll");
    uFindMsg = RegisterWindowMessageA("commdlg_FindReplace");

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = ".";

    RegisterClassA(&wc);

    ParseStartupFile();

    hMain = CreateWindowExA(0, ".", ".", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth, WindowHeight,
        NULL, NULL, hInstance, NULL);

    if (!hMain) {
        return 0;
    }

    LoadStartupFile();
    fDirty = false;
    ApplyTitle();

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        if (hFindDlg && IsDialogMessageA(hFindDlg, &msg)) {
            continue;
        }

        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_F3) {
                SendMessageA(hMain, WM_COMMAND, IDM_EDIT_FINDNEXT, 0);
                continue;
            }
            if (msg.wParam == VK_F5) {
                SendMessageA(hMain, WM_COMMAND, IDM_EDIT_TIME, 0);
                continue;
            }
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                switch (msg.wParam) {
                case 'N': SendMessageA(hMain, WM_COMMAND, IDM_FILE_NEW, 0); continue;
                case 'O': SendMessageA(hMain, WM_COMMAND, IDM_FILE_OPEN, 0); continue;
                case 'S':
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        SendMessageA(hMain, WM_COMMAND, IDM_FILE_SAVEAS, 0);
                    } else {
                        SendMessageA(hMain, WM_COMMAND, IDM_SAVE, 0);
                    }
                    continue;
                case 'P': SendMessageA(hMain, WM_COMMAND, IDM_FILE_PRINT, 0); continue;
                case 'F': SendMessageA(hMain, WM_COMMAND, IDM_EDIT_FIND, 0); continue;
                case 'H': SendMessageA(hMain, WM_COMMAND, IDM_EDIT_REPLACE, 0); continue;
                case 'G': SendMessageA(hMain, WM_COMMAND, IDM_EDIT_GOTO, 0); continue;
                }
            }
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
