#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wininet.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <wincodec.h>

#define MAX_SLIDES 4000
#define MAX_LINES 40
#define MAX_LINE_LEN 200
#define MAX_SETS 20
#define MAX_LANGS 16
#define PATH_BUF 1024

#define IDM_TOOLS_DOWNLOAD 1001
#define IDM_TOOLS_REFRESH 1002
#define IDM_TOOLS_ALERT 1003

#define IDC_GOTO 120
#define IDC_BTN_GOTO 121
#define IDC_CHK_LOOP 122

#define IDDL_BIBLE 301
#define IDDL_SONGBOOK 302
#define IDDL_LIST 303
#define IDDL_DOWNLOAD 304
#define IDDL_CLOSE 305
#define IDDL_PROGRESS 306
#define IDDL_STATUS 307

#define IDAL_TEXT 401
#define IDAL_OK 402
#define IDAL_CANCEL 403

#define MAX_DL_ITEMS 600
#define DL_LEN 180
#define VERSES_PER_SLIDE 20
#define IMG_CACHE_SIZE 12

#define IDC_SETS 100
#define IDC_SLIDES 101
#define IDC_PREVIEW 102
#define IDC_BTN_PREV 103
#define IDC_BTN_NEXT 104
#define IDC_BTN_AUTO 105
#define IDC_BTN_RESTART 106
#define IDC_BTN_FULLSCREEN 107
#define IDC_STATUS 108
#define IDC_LANG 109
#define CTL_TIMER 1
#define DISP_TIMER 2

typedef struct {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int lineCount;
    int duration;
    char bg[260];
    char img[260];
    char audio[260];
    COLORREF fg;
    COLORREF shadow;
    int useFg;
    int useShadow;
    int lowThird;
    int margin;
} Slide;

typedef struct {
    char name[MAX_LINE_LEN];
    Slide slides[MAX_SLIDES];
    int count;
} SlideSet;

static SlideSet g_set;
static char g_currentFile[260];
static int g_index = 0;
static int g_auto = 1;
static int g_remaining = 0;
static int g_defaultDur = 8;

static char g_exeDir[PATH_BUF];
static char g_slidesDir[PATH_BUF];
static int g_setCounts[MAX_LANGS];
static char g_setFiles[MAX_LANGS][MAX_SETS][260];
static char g_setTitles[MAX_LANGS][MAX_SETS][260];
static int g_langCount = 0;
static int g_langIndex = 0;

static char g_langFolders[MAX_LANGS][260];

typedef struct {
    int isSongbook;
    char code[DL_LEN];
    char title[DL_LEN];
    char langCode[24];
    char langName[DL_LEN];
    const char *content;
} DlItem;

static DlItem g_dlItems[MAX_DL_ITEMS];
static int g_dlCount = 0;
static int g_dlMap[MAX_DL_ITEMS];
static int g_dlMapCount = 0;
static int g_dlShowBibles = 1;
static int g_dlBusy = 0;
static HWND g_dlHwnd = NULL;

typedef struct {
    char path[PATH_BUF];
    int w, h, stride;
    unsigned char *bits;
} ImgData;

static ImgData g_imgCache[IMG_CACHE_SIZE];
static int g_imgHead = 0;
static int g_currentAudio = 0;
static int g_loop = 1;

static char g_alert[512];
static int g_alertActive = 0;
static int g_alertElapsed = 0;
static int g_alertTotal = 20000;
static HWND g_alertHwnd = NULL;

static void refreshLanguagesUI(void);
static void openDownloadDialog(HWND parent);
static void openAlertDialog(HWND parent);

static HWND g_main = NULL;
static HWND g_disp = NULL;
static HWND g_preview = NULL;
static HFONT g_dispFontBig = NULL;
static HFONT g_dispFontSmall = NULL;
static HFONT g_prevFont = NULL;
static int g_dispFullscreen = 0;

static void getExeDir(const char *modulePath)
{
    char buf[PATH_BUF];
    char *last;
    strncpy(buf, modulePath, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    last = strrchr(buf, '\\');
    if (last == NULL)
        last = strrchr(buf, '/');
    if (last == NULL) {
        g_exeDir[0] = '\0';
    } else {
        *last = '\0';
        strcpy(g_exeDir, buf);
    }
}

static int dirExists(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

static void findSlidesDir(void)
{
    const char *bases[3];
    char cand[PATH_BUF];
    int i;
    bases[0] = g_exeDir;
    bases[1] = "";
    bases[2] = "..";
    for (i = 0; i < 3; i++) {
        if (bases[i][0] != '\0')
            snprintf(cand, sizeof(cand), "%s\\slides", bases[i]);
        else
            snprintf(cand, sizeof(cand), "slides");
        if (dirExists(cand)) {
            strcpy(g_slidesDir, cand);
            return;
        }
    }
    if (g_exeDir[0] != '\0') {
        snprintf(cand, sizeof(cand), "%s\\..\\..\\slides", g_exeDir);
        if (dirExists(cand)) {
            strcpy(g_slidesDir, cand);
            return;
        }
    }
    g_slidesDir[0] = '\0';
}

static void enumerateSets(int langIdx)
{
    char folder[PATH_BUF];
    char pattern[PATH_BUF];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    int count = 0;
    if (g_slidesDir[0] == '\0') {
        g_setCounts[langIdx] = 0;
        return;
    }
    snprintf(folder, sizeof(folder), "%s\\%s", g_slidesDir, g_langFolders[langIdx]);
    if (!dirExists(folder)) {
        g_setCounts[langIdx] = 0;
        return;
    }
    snprintf(pattern, sizeof(pattern), "%s\\*.txt", folder);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        g_setCounts[langIdx] = 0;
        return;
    }
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && count < MAX_SETS) {
            strncpy(g_setFiles[langIdx][count], fd.cFileName, 259);
            g_setFiles[langIdx][count][259] = '\0';
            count++;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    g_setCounts[langIdx] = count;
}

static void resolveRel(char *out, int cap, const char *file, int langIdx)
{
    if (file[0] == '\\' || file[0] == '/' ||
        (file[0] != '\0' && file[1] == ':') ||
        strncmp(file, "\\\\", 2) == 0) {
        snprintf(out, cap, "%s", file);
    } else {
        snprintf(out, cap, "%s\\%s", g_langFolders[langIdx], file);
    }
}

static int parseColorArg(const char *s, COLORREF *out)
{
    int r, g, b;
    if (sscanf(s, "%d,%d,%d", &r, &g, &b) == 3) {
        *out = RGB(r & 255, g & 255, b & 255);
        return 1;
    }
    return 0;
}

static int loadSet(SlideSet *set, const char *file, int langIdx)
{
    char path[PATH_BUF];
    FILE *f;
    char line[MAX_LINE_LEN];
    int current = -1;
    int defaultDuration = 0;
    char curBg[260] = "";
    char curAudio[260] = "";
    COLORREF curFg = RGB(255, 255, 255);
    COLORREF curShadow = RGB(0, 0, 0);
    int curUseFg = 0;
    int curUseShadow = 0;
    int curLowThird = 0;
    int curMargin = 0;

    snprintf(path, sizeof(path), "%s\\%s\\%s",
        g_slidesDir, g_langFolders[langIdx], file);
    f = fopen(path, "r");
    if (f == NULL)
        return 0;

    strcpy(set->name, file);
    set->count = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        char *p = line;
        int len;
        while (*p == ' ' || *p == '\t')
            p++;
        len = (int)strlen(p);
        while (len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r')) {
            p[len - 1] = '\0';
            len--;
        }

        if (strncmp(p, "[TITLE ", 7) == 0) {
            char *end = strchr(p + 7, ']');
            if (end != NULL) {
                *end = '\0';
                snprintf(set->name, MAX_LINE_LEN, "%s", p + 7);
            }
            continue;
        }
        if (strncmp(p, "[SLIDE", 6) == 0) {
            char *num;
            Slide *s;
            if (set->count >= MAX_SLIDES)
                break;
            current = set->count;
            s = &set->slides[current];
            s->lineCount = 0;
            s->duration = 0;
            strncpy(s->bg, curBg, 259);
            s->bg[259] = '\0';
            s->img[0] = '\0';
            strncpy(s->audio, curAudio, 259);
            s->audio[259] = '\0';
            s->fg = curFg;
            s->shadow = curShadow;
            s->useFg = curUseFg;
            s->useShadow = curUseShadow;
            s->lowThird = curLowThird;
            s->margin = curMargin;
            num = p + 6;
            while (*num == ' ' || *num == '\t')
                num++;
            if (*num >= '0' && *num <= '9')
                s->duration = atoi(num);
            else if (defaultDuration > 0)
                s->duration = defaultDuration;
            set->count++;
            continue;
        }
        if (strncmp(p, "[DURATION ", 10) == 0) {
            defaultDuration = atoi(p + 10);
            continue;
        }
        if (strncmp(p, "[BACKGROUND ", 12) == 0) {
            char *end = strchr(p + 12, ']');
            if (end != NULL) {
                *end = '\0';
                resolveRel(curBg, sizeof(curBg), p + 12, langIdx);
                if (current >= 0)
                    strncpy(set->slides[current].bg, curBg, 259);
            }
            continue;
        }
        if (strncmp(p, "[IMAGE ", 7) == 0) {
            char *end = strchr(p + 7, ']');
            if (end != NULL && current >= 0) {
                *end = '\0';
                resolveRel(set->slides[current].img, 259, p + 7, langIdx);
            }
            continue;
        }
        if (strncmp(p, "[AUDIO ", 7) == 0) {
            char *end = strchr(p + 7, ']');
            if (end != NULL) {
                *end = '\0';
                if (strcmp(p + 7, "none") == 0 || strcmp(p + 7, "off") == 0)
                    curAudio[0] = '\0';
                else
                    resolveRel(curAudio, sizeof(curAudio), p + 7, langIdx);
                if (current >= 0)
                    strncpy(set->slides[current].audio, curAudio, 259);
            }
            continue;
        }
        if (strncmp(p, "[COLOR ", 7) == 0) {
            char *end = strchr(p + 7, ']');
            if (end != NULL) {
                *end = '\0';
                if (parseColorArg(p + 7, &curFg))
                    curUseFg = 1;
                if (current >= 0)
                    set->slides[current].fg = curFg;
            }
            continue;
        }
        if (strncmp(p, "[SHADOW ", 8) == 0) {
            char *end = strchr(p + 8, ']');
            if (end != NULL) {
                *end = '\0';
                if (parseColorArg(p + 8, &curShadow))
                    curUseShadow = 1;
                if (current >= 0)
                    set->slides[current].shadow = curShadow;
            }
            continue;
        }
        if (strcmp(p, "[LOWERTHIRD]") == 0) {
            curLowThird = 1;
            if (current >= 0)
                set->slides[current].lowThird = 1;
            continue;
        }
        if (strncmp(p, "[MARGIN ", 8) == 0) {
            int m = atoi(p + 8);
            if (m < 0)
                m = 0;
            if (m > 45)
                m = 45;
            curMargin = m;
            if (current >= 0)
                set->slides[current].margin = m;
            continue;
        }
        if (current >= 0) {
            Slide *s = &set->slides[current];
            if (s->lineCount < MAX_LINES)
                snprintf(s->lines[s->lineCount], MAX_LINE_LEN, "%s", p);
            s->lineCount++;
        }
    }
    fclose(f);
    return set->count > 0;
}

static void buildText(char *out, size_t cap)
{
    Slide *s = &g_set.slides[g_index];
    int i;
    out[0] = '\0';
    for (i = 0; i < s->lineCount; i++) {
        size_t room = cap - strlen(out) - 1;
        if (room == 0)
            break;
        strncat(out, s->lines[i], room);
        room = cap - strlen(out) - 1;
        if (i + 1 < s->lineCount && room > 0)
            strncat(out, "\n", room);
    }
    if (out[0] == '\0')
        strcpy(out, "(empty slide)");
}

static void playSlideAudio(void);
static void goTo(int n)
{
    if (g_set.count <= 0)
        return;
    g_index = n % g_set.count;
    if (g_index < 0)
        g_index += g_set.count;
    g_remaining = (g_auto && g_set.slides[g_index].duration > 0)
                      ? g_set.slides[g_index].duration : 0;
    playSlideAudio();
}

static void loadSetById(int id)
{
    int i;
    if (id < 0 || id >= g_setCounts[g_langIndex])
        return;
    if (!loadSet(&g_set, g_setFiles[g_langIndex][id], g_langIndex))
        return;
    strcpy(g_currentFile, g_setFiles[g_langIndex][id]);
    g_index = 0;
    g_remaining = (g_auto && g_set.slides[0].duration > 0)
                      ? g_set.slides[0].duration : 0;
    playSlideAudio();

    if (g_main) {
        HWND lb = GetDlgItem(g_main, IDC_SLIDES);
        SendMessage(lb, LB_RESETCONTENT, 0, 0);
        for (i = 0; i < g_set.count; i++) {
            char entry[160];
            snprintf(entry, sizeof(entry), "%d.  %s",
                     i + 1, g_set.slides[i].lines[0]);
            SendMessage(lb, LB_ADDSTRING, 0, (LPARAM)entry);
        }
        SendMessage(lb, LB_SETCURSEL, 0, 0);
    }
}

static void refreshAll(void);

static void repopulateSets(void)
{
    HWND lb;
    int i;
    if (!g_main)
        return;
    lb = GetDlgItem(g_main, IDC_SETS);
    SendMessage(lb, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_setCounts[g_langIndex]; i++)
        SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)g_setTitles[g_langIndex][i]);
    SendMessage(lb, LB_SETCURSEL, 0, 0);
    if (g_setCounts[g_langIndex] > 0) {
        loadSetById(0);
        refreshAll();
    }
}

static void tickTimer(void)
{
    Slide *s;
    if (g_alertActive) {
        g_alertElapsed += 100;
        if (g_alertTotal > 0 && g_alertElapsed >= g_alertTotal) {
            g_alertActive = 0;
            if (g_alertHwnd)
                SendMessage(g_alertHwnd, WM_COMMAND, IDAL_OK, 0);
        }
    }
    if (!g_auto || g_set.count <= 0)
        return;
    s = &g_set.slides[g_index];
    if (s->duration <= 0)
        return;
    if (g_remaining <= 0)
        g_remaining = s->duration;
    g_remaining--;
    if (g_remaining <= 0) {
        if (!g_loop && g_index + 1 >= g_set.count) {
            g_remaining = 0;
            return;
        }
        goTo(g_index + 1);
    }
}

static void stopAudio(void)
{
    if (g_currentAudio) {
        mciSendStringA("close slideaud", NULL, 0, 0);
        g_currentAudio = 0;
    }
}

static void playSlideAudio(void)
{
    char cmd[PATH_BUF + 64];
    const char *rel;
    if (g_set.count <= 0)
        return;
    stopAudio();
    rel = g_set.slides[g_index].audio;
    if (rel == NULL || rel[0] == '\0')
        return;
    snprintf(cmd, sizeof(cmd), "open \"%s\\%s\" type mpegvideo alias slideaud",
        g_slidesDir, rel);
    if (mciSendStringA(cmd, NULL, 0, 0) == 0) {
        if (mciSendStringA("play slideaud", NULL, 0, 0) == 0)
            g_currentAudio = 1;
    }
}

static IWICImagingFactory *g_wicFactory = NULL;

static const GUID wicFactoryClsid =
    { 0xcacaf262, 0x9370, 0x4615, { 0xa1, 0x3b, 0x9f, 0x55, 0x39, 0xda, 0x4c, 0x0a } };
static const GUID wicImagingFactoryIid =
    { 0xec5ec8a9, 0xc495, 0x4b17, { 0x81, 0x7c, 0x06, 0xeb, 0x0e, 0xda, 0x0d, 0xcc } };
static const GUID wicFormat32bppBGRA =
    { 0x6fddc324, 0x4e03, 0x4bfe, { 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x10 } };

static IWICImagingFactory *wicFactory(void)
{
    if (!g_wicFactory) {
        CoCreateInstance(&wicFactoryClsid, NULL, CLSCTX_INPROC_SERVER,
            &wicImagingFactoryIid, (void **)&g_wicFactory);
    }
    return g_wicFactory;
}

static ImgData *loadImageFile(const char *path)
{
    IWICImagingFactory *fac = wicFactory();
    IWICBitmapDecoder *dec = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *conv = NULL;
    WCHAR wp[MAX_PATH];
    UINT w = 0, h = 0, stride, sz;
    unsigned char *bits;
    ImgData *d;
    HRESULT hr;
    if (!fac)
        return NULL;
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wp, MAX_PATH))
        return NULL;
    if (FAILED(fac->lpVtbl->CreateDecoderFromFilename(fac, wp, NULL,
            GENERIC_READ, WICDecodeMetadataCacheOnLoad, &dec)))
        return NULL;
    if (FAILED(dec->lpVtbl->GetFrame(dec, 0, &frame))) {
        dec->lpVtbl->Release(dec);
        return NULL;
    }
    if (FAILED(fac->lpVtbl->CreateFormatConverter(fac, &conv))) {
        frame->lpVtbl->Release(frame);
        dec->lpVtbl->Release(dec);
        return NULL;
    }
    hr = conv->lpVtbl->Initialize(conv, (IWICBitmapSource *)frame,
        &wicFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0,
        WICBitmapPaletteTypeCustom);
    frame->lpVtbl->Release(frame);
    dec->lpVtbl->Release(dec);
    if (FAILED(hr)) {
        conv->lpVtbl->Release(conv);
        return NULL;
    }
    conv->lpVtbl->GetSize(conv, &w, &h);
    if (w == 0 || h == 0 || w > 10000 || h > 10000) {
        conv->lpVtbl->Release(conv);
        return NULL;
    }
    stride = w * 4;
    sz = stride * h;
    bits = malloc(sz);
    if (!bits || FAILED(conv->lpVtbl->CopyPixels(conv, NULL, stride, sz, bits))) {
        free(bits);
        conv->lpVtbl->Release(conv);
        return NULL;
    }
    conv->lpVtbl->Release(conv);
    d = &g_imgCache[g_imgHead];
    if (d->bits)
        free(d->bits);
    d->w = (int)w;
    d->h = (int)h;
    d->stride = (int)stride;
    d->bits = bits;
    snprintf(d->path, sizeof(d->path), "%s", path);
    g_imgHead = (g_imgHead + 1) % IMG_CACHE_SIZE;
    return d;
}

static ImgData *getImage(const char *path)
{
    int i;
    if (!path || !path[0])
        return NULL;
    for (i = 0; i < IMG_CACHE_SIZE; i++) {
        if (g_imgCache[i].bits && strcmp(g_imgCache[i].path, path) == 0)
            return &g_imgCache[i];
    }
    return loadImageFile(path);
}

static void stretchDib(HDC hdc, int dx, int dy, int dw, int dh,
    const ImgData *im)
{
    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = im->w;
    bi.bmiHeader.biHeight = -im->h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    SetStretchBltMode(hdc, HALFTONE);
    StretchDIBits(hdc, dx, dy, dw, dh, 0, 0, im->w, im->h, im->bits,
        &bi, DIB_RGB_COLORS, SRCCOPY);
}

static void drawImageCover(HDC hdc, const RECT *rc, const ImgData *im)
{
    int rw = rc->right - rc->left, rh = rc->bottom - rc->top;
    double scale;
    int dw, dh, dx, dy;
    if (!im || !im->bits)
        return;
    scale = ((double)rw / im->w > (double)rh / im->h)
                ? (double)rw / im->w : (double)rh / im->h;
    dw = (int)(im->w * scale);
    dh = (int)(im->h * scale);
    dx = rc->left + (rw - dw) / 2;
    dy = rc->top + (rh - dh) / 2;
    stretchDib(hdc, dx, dy, dw, dh, im);
}

static void drawImageFit(HDC hdc, const RECT *rc, const ImgData *im)
{
    int rw = rc->right - rc->left, rh = rc->bottom - rc->top;
    double scale;
    int dw, dh, dx, dy;
    if (!im || !im->bits)
        return;
    scale = ((double)rw / im->w < (double)rh / im->h)
                ? (double)rw / im->w : (double)rh / im->h;
    dw = (int)(im->w * scale);
    dh = (int)(im->h * scale);
    dx = rc->left + (rw - dw) / 2;
    dy = rc->top + (rh - dh) / 2;
    stretchDib(hdc, dx, dy, dw, dh, im);
}

static void updateStatus(void)
{
    char buf[512];
    int dur = g_set.slides[g_index].duration;
    if (!g_main)
        return;
    snprintf(buf, sizeof(buf), "Slide %d / %d   |   %s   |   %s   |   next in %ds",
        g_index + 1, g_set.count, g_set.name, g_langFolders[g_langIndex],
        (g_auto && dur > 0) ? g_remaining : 0);
    SetDlgItemTextA(g_main, IDC_STATUS, buf);
    SetDlgItemTextA(g_main, IDC_BTN_AUTO, g_auto ? "AUTO: ON" : "AUTO: OFF");

    {
        HWND lb = GetDlgItem(g_main, IDC_SLIDES);
        int cur = (int)SendMessage(lb, LB_GETCURSEL, 0, 0);
        if (cur != g_index)
            SendMessage(lb, LB_SETCURSEL, g_index, 0);
    }
}

static void refreshAll(void)
{
    if (g_preview)
        InvalidateRect(g_preview, NULL, TRUE);
    if (g_disp)
        InvalidateRect(g_disp, NULL, TRUE);
    updateStatus();
}

static void toggleFullscreen(void)
{
    static RECT saved;
    static int savedStyle = 0;
    static int savedExStyle = 0;
    if (!g_disp)
        return;

    if (!g_dispFullscreen) {
        GetWindowRect(g_disp, &saved);
        savedStyle = GetWindowLongA(g_disp, GWL_STYLE);
        savedExStyle = GetWindowLongA(g_disp, GWL_EXSTYLE);
        SetWindowLongA(g_disp, GWL_STYLE,
            savedStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX));
        SetWindowPos(g_disp, HWND_TOPMOST, 0, 0,
            GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        g_dispFullscreen = 1;
    } else {
        SetWindowLongA(g_disp, GWL_STYLE, savedStyle);
        SetWindowLongA(g_disp, GWL_EXSTYLE, savedExStyle);
        SetWindowPos(g_disp, HWND_NOTOPMOST, saved.left, saved.top,
            saved.right - saved.left, saved.bottom - saved.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        g_dispFullscreen = 0;
    }
    refreshAll();
}

static void makeDispFonts(int clientHeight)
{
    if (g_dispFontBig)
        DeleteObject(g_dispFontBig);
    if (g_dispFontSmall)
        DeleteObject(g_dispFontSmall);
    g_dispFontBig = CreateFontA(-clientHeight / 12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_dispFontSmall = CreateFontA(-clientHeight / 34, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
}

static void paintLiveText(HDC hdc, const RECT *rc, HFONT font, const char *text,
    COLORREF color, COLORREF shadow, int useShadow)
{
    RECT r = *rc;
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    if (useShadow) {
        RECT sr = *rc;
        OffsetRect(&sr, 3, 3);
        SetTextColor(hdc, shadow);
        DrawTextA(hdc, text, -1, &sr, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
    }
    SetTextColor(hdc, color);
    DrawTextA(hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(hdc, old);
}

/* Pick the largest font height (px) that fits "text" inside width x availH. */
static int fitFontSize(HDC hdc, const char *text, int width, int availH, int base)
{
    int fs = base;
    while (fs > 14) {
        HFONT f = CreateFontA(-fs, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, f);
        RECT r = { 0, 0, width, 100000 };
        DrawTextA(hdc, text, -1, &r, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(hdc, old);
        DeleteObject(f);
        if (r.bottom - r.top <= availH)
            break;
        fs -= 2;
    }
    return fs;
}

static void paintDisplay(HWND hwnd, HDC hdc, const RECT *rc)
{
    char header[512];
    char footer[512];
    char body[16384];
    char full[PATH_BUF];
    RECT r;
    int w, filled;
    HFONT old;
    Slide *s = &g_set.slides[g_index];
    ImgData *im;
    (void)hwnd;

    if (s->bg[0]) {
        snprintf(full, sizeof(full), "%s\\%s", g_slidesDir, s->bg);
        im = getImage(full);
        FillRect(hdc, rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        if (im)
            drawImageCover(hdc, rc, im);
    } else {
        HBRUSH bg = CreateSolidBrush(RGB(8, 8, 14));
        FillRect(hdc, rc, bg);
        DeleteObject(bg);
    }
    SetBkMode(hdc, TRANSPARENT);

    if (!g_dispFullscreen) {
        r = *rc;
        r.bottom = r.top + 30;
        snprintf(header, sizeof(header), "%s   Slide %d / %d   %s",
            g_set.name, g_index + 1, g_set.count, g_auto ? "[AUTO]" : "[MANUAL]");
        old = (HFONT)SelectObject(hdc, g_dispFontSmall);
        SetTextColor(hdc, RGB(170, 170, 170));
        DrawTextA(hdc, header, -1, &r, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

        r = *rc;
        r.top = rc->bottom - 26;
        snprintf(footer, sizeof(footer),
            "Space/N next   P prev   A auto   +/- speed   R restart   Esc/F11 fullscreen");
        SetTextColor(hdc, RGB(140, 140, 140));
        DrawTextA(hdc, footer, -1, &r, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

        if (g_auto && s->duration > 0) {
            char cd[128];
            snprintf(cd, sizeof(cd), "next in %d s", g_remaining);
            SetTextColor(hdc, RGB(140, 140, 140));
            DrawTextA(hdc, cd, -1, &r, DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            w = rc->right - rc->left;
            filled = (g_remaining * w) / s->duration;
            if (filled > w)
                filled = w;
            {
                HBRUSH hb = CreateSolidBrush(RGB(0, 190, 0));
                RECT bar = { 0, rc->bottom - 4, filled, rc->bottom };
                FillRect(hdc, &bar, hb);
                DeleteObject(hb);
            }
        }
        SelectObject(hdc, old);
    }

    r = *rc;
    if (!g_dispFullscreen) {
        r.top += 30;
        r.bottom -= 26;
    }
    if (s->margin > 0) {
        int mx = (r.right - r.left) * s->margin / 100;
        int my = (r.bottom - r.top) * s->margin / 100;
        r.left += mx;
        r.right -= mx;
        r.top += my;
        r.bottom -= my;
    }

    if (s->img[0]) {
        snprintf(full, sizeof(full), "%s\\%s", g_slidesDir, s->img);
        im = getImage(full);
        if (im)
            drawImageFit(hdc, &r, im);
    } else {
        buildText(body, sizeof(body));
        if (s->lowThird) {
            int rh = r.bottom - r.top;
            r.top = r.bottom - rh / 3;
        }
        {
            int availH = r.bottom - r.top;
            int base = availH / 10;
            int fs;
            HFONT bodyFont;
            if (base < 20)
                base = 20;
            if (base > 96)
                base = 96;
            fs = fitFontSize(hdc, body, r.right - r.left, availH, base);
            bodyFont = CreateFontA(-fs, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            paintLiveText(hdc, &r, bodyFont, body,
                s->useFg ? s->fg : RGB(255, 255, 255),
                s->useShadow ? s->shadow : RGB(0, 0, 0),
                s->useShadow);
            DeleteObject(bodyFont);
        }
    }

    if (g_alertActive && g_alert[0]) {
        RECT ar = *rc;
        ar.top = rc->bottom - 48;
        {
            HBRUSH hb = CreateSolidBrush(RGB(20, 20, 26));
            FillRect(hdc, &ar, hb);
            DeleteObject(hb);
        }
        {
            HBRUSH hb = CreateSolidBrush(RGB(220, 120, 0));
            RECT line = { ar.left, ar.top, ar.right, ar.top + 3 };
            FillRect(hdc, &line, hb);
            DeleteObject(hb);
        }
        old = (HFONT)SelectObject(hdc, g_dispFontSmall);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 200, 120));
        {
            SIZE sz;
            int total, x;
            GetTextExtentPoint32A(hdc, g_alert, (int)strlen(g_alert), &sz);
            total = (rc->right - rc->left) + sz.cx + 80;
            x = (rc->right - rc->left) - ((g_alertElapsed * 90) / 1000) % total;
            {
                RECT tr = { x, ar.top + 5, x + sz.cx + 30, ar.bottom };
                DrawTextA(hdc, g_alert, -1, &tr, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            }
        }
        SelectObject(hdc, old);
    }
}

static void paintPreview(HWND hwnd, HDC hdc, const RECT *rc)
{
    char body[16384];
    char info[256];
    char full[PATH_BUF];
    RECT r;
    HFONT old;
    Slide *s = &g_set.slides[g_index];
    ImgData *im;
    (void)hwnd;

    if (s->bg[0]) {
        snprintf(full, sizeof(full), "%s\\%s", g_slidesDir, s->bg);
        im = getImage(full);
        FillRect(hdc, rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        if (im)
            drawImageCover(hdc, rc, im);
    } else {
        HBRUSH bg = CreateSolidBrush(RGB(18, 18, 28));
        FillRect(hdc, rc, bg);
        DeleteObject(bg);
    }

    r = *rc;
    r.top += 8;
    r.bottom -= 30;
    if (s->img[0]) {
        snprintf(full, sizeof(full), "%s\\%s", g_slidesDir, s->img);
        im = getImage(full);
        if (im)
            drawImageFit(hdc, &r, im);
    } else {
        buildText(body, sizeof(body));
        if (s->lowThird) {
            int rh = r.bottom - r.top;
            r.top = r.bottom - rh / 3;
        }
        {
            int availH = r.bottom - r.top;
            int fs = fitFontSize(hdc, body, r.right - r.left, availH, 40);
            HFONT bodyFont = CreateFontA(-fs, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            paintLiveText(hdc, &r, bodyFont, body,
                s->useFg ? s->fg : RGB(255, 255, 255),
                s->useShadow ? s->shadow : RGB(0, 0, 0),
                s->useShadow);
            DeleteObject(bodyFont);
        }
    }

    r = *rc;
    r.top = rc->bottom - 22;
    snprintf(info, sizeof(info), "slide %d / %d   duration %ds",
        g_index + 1, g_set.count, s->duration);
    old = (HFONT)SelectObject(hdc, g_prevFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(120, 120, 120));
    DrawTextA(hdc, info, -1, &r, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
    SelectObject(hdc, old);
}

static LRESULT CALLBACK previewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT rc;
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        paintPreview(hwnd, hdc, &rc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK dispWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        makeDispFonts(720);
        SetTimer(hwnd, DISP_TIMER, 100, NULL);
        return 0;

    case WM_TIMER:
        if (g_disp)
            InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_KEYDOWN: {
        int key = tolower((int)wParam);
        Slide *s = &g_set.slides[g_index];
        switch (key) {
        case VK_SPACE:
        case VK_RIGHT:
        case VK_NEXT:
        case VK_DOWN:
        case 'n':
            goTo(g_index + 1);
            refreshAll();
            break;
        case VK_LEFT:
        case VK_PRIOR:
        case VK_UP:
        case 'p':
            goTo(g_index - 1);
            refreshAll();
            break;
        case 'a':
            g_auto = !g_auto;
            g_remaining = (g_auto && s->duration > 0) ? s->duration : 0;
            refreshAll();
            break;
        case VK_ADD:
        case VK_OEM_PLUS:
            g_defaultDur++;
            if (s->duration > 0) {
                s->duration++;
                g_remaining = s->duration;
            }
            refreshAll();
            break;
        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            if (g_defaultDur > 1)
                g_defaultDur--;
            if (s->duration > 1) {
                s->duration--;
                g_remaining = s->duration;
            }
            refreshAll();
            break;
        case 'r':
            goTo(0);
            refreshAll();
            break;
        case VK_F11:
        case VK_ESCAPE:
            toggleFullscreen();
            break;
        default:
            break;
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK:
        toggleFullscreen();
        return 0;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            makeDispFonts(HIWORD(lParam));
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT rc;
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        paintDisplay(hwnd, hdc, &rc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, DISP_TIMER);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void createMainControls(HWND hwnd)
{
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HINSTANCE inst = GetModuleHandle(NULL);
    HWND c;

    c = CreateWindowExA(0, "BUTTON", "Prev",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 10, 90, 34, hwnd, (HMENU)IDC_BTN_PREV, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Next",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 108, 10, 90, 34, hwnd, (HMENU)IDC_BTN_NEXT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "AUTO: ON",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 206, 10, 110, 34, hwnd, (HMENU)IDC_BTN_AUTO, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Restart",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 324, 10, 90, 34, hwnd, (HMENU)IDC_BTN_RESTART, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Fullscreen",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 422, 10, 110, 34, hwnd, (HMENU)IDC_BTN_FULLSCREEN, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_RIGHT, 540, 16, 410, 22, hwnd, (HMENU)IDC_STATUS, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);

    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_NUMBER, 540, 46, 64, 24, hwnd, (HMENU)IDC_GOTO, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Go",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 608, 44, 42, 26, hwnd, (HMENU)IDC_BTN_GOTO, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Loop playback",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 656, 46, 130, 22, hwnd, (HMENU)IDC_CHK_LOOP, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    SendMessage(c, BM_SETCHECK, g_loop ? BST_CHECKED : BST_UNCHECKED, 0);

    c = CreateWindowExA(0, "STATIC", "Slide Sets",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 72, 230, 16, hwnd, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_BORDER,
        10, 90, 230, 450, hwnd, (HMENU)IDC_SETS, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);

    c = CreateWindowExA(0, "STATIC", "Language:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 48, 64, 18, hwnd, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        78, 46, 162, 200, hwnd, (HMENU)IDC_LANG, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);

    c = CreateWindowExA(0, "STATIC", "Slides in Set",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 250, 48, 250, 16, hwnd, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_BORDER,
        250, 68, 250, 472, hwnd, (HMENU)IDC_SLIDES, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);

    c = CreateWindowExA(WS_EX_CLIENTEDGE, "SlidePreviewClass", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER, 510, 68, 440, 472, hwnd, (HMENU)IDC_PREVIEW, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    g_preview = c;

    c = CreateWindowExA(0, "STATIC",
        "Operate from here or from the live window. In the live window: Space/N next, P prev, A auto, +/- speed, R restart, Esc/F11 fullscreen. Slides are edited as .txt files in the slides folder.",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 548, 940, 60, hwnd, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
}

static LRESULT CALLBACK mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        createMainControls(hwnd);
        g_prevFont = CreateFontA(-40, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        SetTimer(hwnd, CTL_TIMER, 250, NULL);
        return 0;

    case WM_TIMER:
        if (wParam == CTL_TIMER) {
            tickTimer();
            refreshAll();
        }
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        Slide *s = &g_set.slides[g_index];
        if (code == BN_CLICKED) {
            if (id == IDC_BTN_PREV) {
                goTo(g_index - 1);
                refreshAll();
            } else if (id == IDC_BTN_NEXT) {
                goTo(g_index + 1);
                refreshAll();
            } else if (id == IDC_BTN_AUTO) {
                g_auto = !g_auto;
                g_remaining = (g_auto && s->duration > 0) ? s->duration : 0;
                refreshAll();
            } else if (id == IDC_BTN_RESTART) {
                goTo(0);
                refreshAll();
            } else if (id == IDC_BTN_FULLSCREEN) {
                toggleFullscreen();
            } else if (id == IDC_BTN_GOTO) {
                char buf[32];
                int n;
                GetDlgItemTextA(hwnd, IDC_GOTO, buf, sizeof(buf));
                n = atoi(buf);
                if (n >= 1 && n <= g_set.count) {
                    goTo(n - 1);
                    refreshAll();
                }
            } else if (id == IDC_CHK_LOOP) {
                g_loop = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
            }
        } else if (code == LBN_SELCHANGE) {
            if (id == IDC_SETS) {
                int sel = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < g_setCounts[g_langIndex]) {
                    loadSetById(sel);
                    refreshAll();
                }
            } else if (id == IDC_SLIDES) {
                int sel = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    goTo(sel);
                    refreshAll();
                }
            }
        } else if (code == CBN_SELCHANGE && id == IDC_LANG) {
            int sel = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_langCount && sel != g_langIndex) {
                g_langIndex = sel;
                repopulateSets();
            }
        } else if (code == 0 && id == IDM_TOOLS_DOWNLOAD) {
            openDownloadDialog(hwnd);
        } else if (code == 0 && id == IDM_TOOLS_REFRESH) {
            refreshLanguagesUI();
        } else if (code == 0 && id == IDM_TOOLS_ALERT) {
            openAlertDialog(hwnd);
        }
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, CTL_TIMER);
        if (g_prevFont)
            DeleteObject(g_prevFont);
        if (g_dispFontBig)
            DeleteObject(g_dispFontBig);
        if (g_dispFontSmall)
            DeleteObject(g_dispFontSmall);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ====================================================================
   Library download support: JSON helpers, WinINet HTTP, Bible parsing,
   bundled song books, and the Download Library dialog.
   ==================================================================== */

static void skipWs(const char **pp)
{
    while (**pp == ' ' || **pp == '\t' || **pp == '\n' || **pp == '\r')
        (*pp)++;
}

static int hexVal(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int utf8Write(unsigned cp, char *out)
{
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
}

static int parseJsonString(const char **pp, char *out, int cap)
{
    const char *p = *pp;
    int oi = 0;
    if (*p != '"')
        return 0;
    p++;
    while (*p && *p != '"') {
        if (*p == '\\') {
            p++;
            if (*p == '\0')
                break;
            switch (*p) {
            case '"':  if (oi < cap - 1) out[oi++] = '"';  p++; break;
            case '\\': if (oi < cap - 1) out[oi++] = '\\'; p++; break;
            case '/':  if (oi < cap - 1) out[oi++] = '/';  p++; break;
            case 'b':  if (oi < cap - 1) out[oi++] = '\b'; p++; break;
            case 'f':  if (oi < cap - 1) out[oi++] = '\f'; p++; break;
            case 'n':  if (oi < cap - 1) out[oi++] = '\n'; p++; break;
            case 'r':  if (oi < cap - 1) out[oi++] = '\r'; p++; break;
            case 't':  if (oi < cap - 1) out[oi++] = '\t'; p++; break;
            case 'u': {
                unsigned cp = 0;
                int i, ok = 1;
                for (i = 1; i <= 4 && ok; i++) {
                    int hv = hexVal(p[i]);
                    if (hv < 0) ok = 0;
                    else cp = (cp << 4) | (unsigned)hv;
                }
                if (!ok)
                    break;
                p += 4;
                if (cp >= 0xD800 && cp <= 0xDBFF && p[1] == '\\' && p[2] == 'u') {
                    unsigned lo = 0;
                    ok = 1;
                    for (i = 1; i <= 4 && ok; i++) {
                        int hv = hexVal(p[2 + i]);
                        if (hv < 0) ok = 0;
                        else lo = (lo << 4) | (unsigned)hv;
                    }
                    if (ok && lo >= 0xDC00 && lo <= 0xDFFF) {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        p += 6;
                    }
                }
                if (oi + 4 < cap)
                    oi += utf8Write(cp, out + oi);
                p++;
                break;
            }
            default:
                p++;
                break;
            }
        } else {
            if (oi < cap - 1)
                out[oi++] = *p;
            p++;
        }
    }
    out[oi] = '\0';
    if (*p == '"')
        p++;
    *pp = p;
    return 1;
}

static int parseJsonInt(const char **pp, long *out)
{
    const char *p = *pp;
    long v = 0;
    int neg = 0;
    skipWs(&p);
    if (*p == '-') { neg = 1; p++; }
    if (*p < '0' || *p > '9')
        return 0;
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        p++;
    }
    *pp = p;
    *out = neg ? -v : v;
    return 1;
}

static void skipValue(const char **pp)
{
    const char *p = *pp;
    skipWs(&p);
    if (*p == '{' || *p == '[') {
        int depth = 1;
        p++;
        while (*p && depth) {
            if (*p == '{' || *p == '[') {
                depth++;
                p++;
            } else if (*p == '}' || *p == ']') {
                depth--;
                p++;
            } else if (*p == '"') {
                p++;
                while (*p && *p != '"') {
                    if (*p == '\\') p++;
                    p++;
                }
                if (*p) p++;
            } else {
                p++;
            }
        }
    } else if (*p == '"') {
        p++;
        while (*p && *p != '"') {
            if (*p == '\\') p++;
            p++;
        }
        if (*p) p++;
    } else {
        while (*p && *p != ',' && *p != '}' && *p != ']' &&
               *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
            p++;
    }
    *pp = p;
}

/* Walks one key/value pair inside an object. Call repeatedly with *pp
   positioned after '{' or after ','. Returns 0 at the closing '}'.
   On success *valueStart points at the value token and *pp is left
   after the value (and any following ','). */
static int objectNext(const char **pp, char *key, int cap, const char **valueStart)
{
    const char *p = *pp;
    for (;;) {
        skipWs(&p);
        if (*p == '}')
            break;
        if (*p == ',')
            p++;
        skipWs(&p);
        if (*p == '}')
            break;
        if (!parseJsonString(&p, key, cap)) {
            if (*p == '}') { p++; *pp = p; return 0; }
            skipValue(&p);
            continue;
        }
        skipWs(&p);
        if (*p != ':') {
            skipValue(&p);
            continue;
        }
        p++;
        skipWs(&p);
        *valueStart = p;
        skipValue(&p);
        skipWs(&p);
        if (*p == ',')
            p++;
        *pp = p;
        return 1;
    }
    if (*p == '}') p++;
    *pp = p;
    return 0;
}

static char *httpGet(const char *url, long *outLen, int *outStatus)
{
    char *buf = NULL;
    HINTERNET net, req;
    DWORD sc = 0, scsz = sizeof(sc);
    DWORD cap = 1 << 20, used = 0, rd;
    DWORD to = 30000;
    char *nb;

    *outStatus = 0;
    if (outLen)
        *outLen = 0;
    net = InternetOpenA("DomaineChurchPresenter/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!net)
        return NULL;
    InternetSetOptionA(net, INTERNET_OPTION_CONNECT_TIMEOUT, &to, sizeof(to));
    InternetSetOptionA(net, INTERNET_OPTION_RECEIVE_TIMEOUT, &to, sizeof(to));
    InternetSetOptionA(net, INTERNET_OPTION_SEND_TIMEOUT, &to, sizeof(to));
    req = InternetOpenUrlA(net, url, NULL, 0,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!req) {
        InternetCloseHandle(net);
        return NULL;
    }
    HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &sc, &scsz, NULL);
    *outStatus = (int)sc;
    if (sc != 200) {
        InternetCloseHandle(req);
        InternetCloseHandle(net);
        return NULL;
    }
    buf = malloc(cap);
    if (!buf) {
        InternetCloseHandle(req);
        InternetCloseHandle(net);
        return NULL;
    }
    for (;;) {
        rd = 0;
        if (!InternetReadFile(req, buf + used, cap - used, &rd) || rd == 0)
            break;
        used += rd;
        if (used + (1 << 16) >= cap) {
            cap <<= 1;
            nb = realloc(buf, cap);
            if (!nb)
                break;
            buf = nb;
        }
    }
    InternetCloseHandle(req);
    InternetCloseHandle(net);
    buf[used] = '\0';
    if (outLen)
        *outLen = (long)used;
    return buf;
}

/* Parse https://api.getbible.net/v2/translations.json into DlItem list. */
static int parseBibleList(const char *json, DlItem *items, int maxItems)
{
    const char *p = json;
    int n = 0;
    p = strchr(p, '{');
    if (!p)
        return 0;
    p++;
    for (;;) {
        char key[64];
        char title[DL_LEN] = "", langCode[24] = "", langName[DL_LEN] = "";
        const char *vs;
        if (!objectNext(&p, key, sizeof(key), &vs))
            break;
        if (*vs == '{') {
            const char *q = vs + 1;
            for (;;) {
                char k2[64];
                const char *v2;
                if (!objectNext(&q, k2, sizeof(k2), &v2))
                    break;
                if (strcmp(k2, "translation") == 0)
                    parseJsonString(&v2, title, sizeof(title));
                else if (strcmp(k2, "lang") == 0)
                    parseJsonString(&v2, langCode, sizeof(langCode));
                else if (strcmp(k2, "language") == 0)
                    parseJsonString(&v2, langName, sizeof(langName));
            }
        }
        if (n < maxItems && title[0] != '\0') {
            items[n].isSongbook = 0;
            items[n].content = NULL;
            snprintf(items[n].code, sizeof(items[n].code), "%s", key);
            snprintf(items[n].title, sizeof(items[n].title), "%s", title);
            snprintf(items[n].langCode, sizeof(items[n].langCode), "%s", langCode);
            snprintf(items[n].langName, sizeof(items[n].langName),
                langName[0] ? "%s" : "Unknown", langName);
            n++;
        }
    }
    return n;
}

static void addSlideLine(Slide *s, const char *text)
{
    if (s->lineCount < MAX_LINES)
        snprintf(s->lines[s->lineCount], MAX_LINE_LEN, "%s", text);
    s->lineCount++;
}

static void addChapterSlides(SlideSet *out, const char *bookName,
    long chNum, const char *chName, const char *versesArr)
{
    struct V { long v; char text[320]; } verses[300];
    int vc = 0, start, i;
    const char *q = versesArr + 1;
    (void)chName;

    for (;;) {
        char k[64];
        long vn = 0;
        char text[320] = "";
        const char *v;
        skipWs(&q);
        if (*q == ']')
            break;
        if (*q != '{') { q++; continue; }
        q++;
        while (objectNext(&q, k, sizeof(k), &v)) {
            if (strcmp(k, "verse") == 0) {
                long x;
                parseJsonInt(&v, &x);
                vn = x;
            } else if (strcmp(k, "text") == 0) {
                parseJsonString(&v, text, sizeof(text));
            }
        }
        if (vc < 300 && vn > 0) {
            verses[vc].v = vn;
            snprintf(verses[vc].text, sizeof(verses[vc].text), "%s", text);
            vc++;
        }
    }
    if (vc == 0)
        return;

    start = 0;
    while (start < vc) {
        int end = start + VERSES_PER_SLIDE;
        Slide *s;
        char hdr[180];
        if (end > vc)
            end = vc;
        if (out->count >= MAX_SLIDES)
            return;
        s = &out->slides[out->count];
        memset(s, 0, sizeof(*s));
        s->duration = 8;
        if (start == 0 && end == vc)
            snprintf(hdr, sizeof(hdr), "%s %ld", bookName, chNum);
        else
            snprintf(hdr, sizeof(hdr), "%s %ld:%ld-%ld",
                bookName, chNum, verses[start].v, verses[end - 1].v);
        addSlideLine(s, hdr);
        for (i = start; i < end; i++) {
            char line[340];
            snprintf(line, sizeof(line), "%ld %s", verses[i].v, verses[i].text);
            addSlideLine(s, line);
        }
        out->count++;
        start = end;
    }
}

/* Parse a whole-translation JSON and build a chapter-per-slide set. */
static int buildBibleSlides(const char *json, SlideSet *out)
{
    const char *p = json;
    strcpy(out->name, "Bible");
    out->count = 0;
    p = strstr(p, "\"books\"");
    if (!p)
        return 0;
    while (*p && *p != ':')
        p++;
    p++;
    skipWs(&p);
    if (*p != '[')
        return 0;
    p++;
    for (;;) {
        skipWs(&p);
        if (*p == ']')
            break;
        if (*p != '{') { p++; continue; }
        p++;
        {
            char bookName[96] = "";
            char key[64];
            const char *vs;
            while (objectNext(&p, key, sizeof(key), &vs)) {
                if (strcmp(key, "name") == 0) {
                    parseJsonString(&vs, bookName, sizeof(bookName));
                } else if (strcmp(key, "chapters") == 0 && *vs == '[') {
                    const char *q = vs + 1;
                    for (;;) {
                        char k2[64];
                        long chNum = 0;
                        char chName[128] = "";
                        const char *versesStart = NULL;
                        const char *v2;
                        skipWs(&q);
                        if (*q == ']')
                            break;
                        if (*q != '{') { q++; continue; }
                        q++;
                        while (objectNext(&q, k2, sizeof(k2), &v2)) {
                            if (strcmp(k2, "chapter") == 0) {
                                long x;
                                parseJsonInt(&v2, &x);
                                chNum = x;
                            } else if (strcmp(k2, "name") == 0) {
                                parseJsonString(&v2, chName, sizeof(chName));
                            } else if (strcmp(k2, "verses") == 0 && *v2 == '[') {
                                versesStart = v2;
                            }
                        }
                        if (versesStart)
                            addChapterSlides(out, bookName, chNum, chName, versesStart);
                    }
                }
            }
        }
    }
    return out->count > 0;
}

static void langFolderForItem(const DlItem *it, char *folder, int cap)
{
    if (strcmp(it->langCode, "sw") == 0) {
        snprintf(folder, cap, "%s\\Kiswahili", g_slidesDir);
    } else if (strcmp(it->langCode, "en") == 0) {
        snprintf(folder, cap, "%s\\English", g_slidesDir);
    } else {
        char name[DL_LEN];
        char *c;
        snprintf(name, sizeof(name), "%s",
            it->langName[0] != '\0' && strcmp(it->langName, "Unknown") != 0
                ? it->langName : it->langCode);
        for (c = name; *c; c++) {
            if (strchr("<>:\"/\\|?*", *c) != NULL || (unsigned char)*c >= 0x80)
                *c = '_';
        }
        snprintf(folder, cap, "%s\\%s", g_slidesDir, name);
    }
}

static int writeSlideSetFile(const char *folder, const char *fileName,
    const SlideSet *set, const char *title)
{
    char path[PATH_BUF];
    FILE *f;
    int i, j;
    snprintf(path, sizeof(path), "%s\\%s", folder, fileName);
    f = fopen(path, "w");
    if (!f)
        return 0;
    fprintf(f, "[DURATION 8]\n");
    fprintf(f, "[TITLE %s]\n", title);
    for (i = 0; i < set->count; i++) {
        const Slide *s = &set->slides[i];
        fprintf(f, "[SLIDE %d]\n", s->duration > 0 ? s->duration : 8);
        for (j = 0; j < s->lineCount; j++)
            fprintf(f, "%s\n", s->lines[j]);
    }
    fclose(f);
    return 1;
}

static int downloadBibleItem(int itemIdx)
{
    DlItem *it = &g_dlItems[itemIdx];
    char folder[PATH_BUF], url[512];
    char *json;
    SlideSet *set;
    int status = 0;
    langFolderForItem(it, folder, sizeof(folder));
    if (!dirExists(folder))
        CreateDirectoryA(folder, NULL);
    snprintf(url, sizeof(url), "https://api.getbible.net/v2/%s.json", it->code);
    json = httpGet(url, NULL, &status);
    if (!json)
        return 0;
    set = calloc(1, sizeof(SlideSet));
    if (!set) {
        free(json);
        return 0;
    }
    if (!buildBibleSlides(json, set)) {
        free(set);
        free(json);
        return 0;
    }
    free(json);
    if (!writeSlideSetFile(folder, it->code, set, it->title)) {
        free(set);
        return 0;
    }
    free(set);
    return 1;
}

static int installSongbookItem(int itemIdx)
{
    DlItem *it = &g_dlItems[itemIdx];
    char folder[PATH_BUF], path[PATH_BUF];
    FILE *f;
    langFolderForItem(it, folder, sizeof(folder));
    if (!dirExists(folder))
        CreateDirectoryA(folder, NULL);
    snprintf(path, sizeof(path), "%s\\%s.txt", folder, it->code);
    f = fopen(path, "w");
    if (!f)
        return 0;
    if (it->content)
        fputs(it->content, f);
    fclose(f);
    return 1;
}

static void pumpUi(void)
{
    MSG m;
    while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
        if (m.message == WM_QUIT) {
            PostQuitMessage((int)m.wParam);
            return;
        }
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
}

static void setDlStatus(const char *text)
{
    if (g_dlHwnd)
        SetDlgItemTextA(g_dlHwnd, IDDL_STATUS, text);
}

static void setDlProgress(int pct)
{
    if (g_dlHwnd)
        SendMessageA(GetDlgItem(g_dlHwnd, IDDL_PROGRESS), PBM_SETPOS, pct, 0);
}

static void enumerateLanguages(void)
{
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pattern[PATH_BUF];
    int i, j;
    g_langCount = 0;
    if (g_slidesDir[0] == '\0')
        return;
    snprintf(pattern, sizeof(pattern), "%s\\*", g_slidesDir);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0 &&
            g_langCount < MAX_LANGS) {
            strncpy(g_langFolders[g_langCount], fd.cFileName, 259);
            g_langFolders[g_langCount][259] = '\0';
            g_langCount++;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    for (i = 0; i < g_langCount; i++) {
        for (j = i + 1; j < g_langCount; j++) {
            if (_stricmp(g_langFolders[j], g_langFolders[i]) < 0) {
                char t[260];
                strcpy(t, g_langFolders[i]);
                strcpy(g_langFolders[i], g_langFolders[j]);
                strcpy(g_langFolders[j], t);
            }
        }
    }
}

static void loadAllTitles(void)
{
    int i, j;
    for (i = 0; i < g_langCount; i++) {
        for (j = 0; j < g_setCounts[i]; j++) {
            SlideSet *t = calloc(1, sizeof(SlideSet));
            if (!t)
                continue;
            if (loadSet(t, g_setFiles[i][j], i))
                snprintf(g_setTitles[i][j], 260, "%s", t->name);
            else
                snprintf(g_setTitles[i][j], 260, "%s", g_setFiles[i][j]);
            free(t);
        }
    }
}

static void refreshLanguagesUI(void)
{
    int i;
    enumerateLanguages();
    if (g_langCount <= 0)
        return;
    for (i = 0; i < g_langCount; i++)
        enumerateSets(i);
    if (g_langIndex >= g_langCount || g_setCounts[g_langIndex] <= 0) {
        g_langIndex = 0;
        for (i = 0; i < g_langCount; i++) {
            if (g_setCounts[i] > 0) {
                g_langIndex = i;
                break;
            }
        }
    }
    loadAllTitles();
    if (g_main) {
        HWND cb = GetDlgItem(g_main, IDC_LANG);
        SendMessageA(cb, CB_RESETCONTENT, 0, 0);
        for (i = 0; i < g_langCount; i++)
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)g_langFolders[i]);
        SendMessageA(cb, CB_SETCURSEL, g_langIndex, 0);
    }
    repopulateSets();
    refreshAll();
}

static void fillDlList(HWND dlg)
{
    HWND lb = GetDlgItem(dlg, IDDL_LIST);
    int i;
    g_dlMapCount = 0;
    SendMessageA(lb, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_dlCount; i++) {
        char entry[300];
        if (g_dlItems[i].isSongbook && g_dlShowBibles)
            continue;
        if (!g_dlItems[i].isSongbook && !g_dlShowBibles)
            continue;
        if (g_dlItems[i].isSongbook)
            snprintf(entry, sizeof(entry), "%s", g_dlItems[i].title);
        else
            snprintf(entry, sizeof(entry), "%s  -  %s  [%s]",
                g_dlItems[i].title, g_dlItems[i].langName, g_dlItems[i].code);
        SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)entry);
        g_dlMap[g_dlMapCount++] = i;
    }
}

static int fetchDlItems(void)
{
    char *json;
    int status = 0;
    g_dlCount = 0;
    setDlStatus("Fetching Bible list from getbible.net ...");
    pumpUi();
    json = httpGet("https://api.getbible.net/v2/translations.json", NULL, &status);
    if (json) {
        g_dlCount = parseBibleList(json, g_dlItems, MAX_DL_ITEMS);
        free(json);
    }
    if (g_dlCount > 0)
        setDlStatus("Bible list fetched. Bundled song books are available below.");
    else
        setDlStatus("Could not reach getbible.net. Bundled song books are still available.");
    pumpUi();
    return g_dlCount;
}

static const char songbookHymns1[] =
"[TITLE Classic Hymns Vol 1]\n"
"[DURATION 8]\n"
"[SLIDE 8]\n"
"Amazing Grace\n"
"Amazing grace! how sweet the sound,\n"
"That saved a wretch like me!\n"
"I once was lost, but now am found,\n"
"Was blind, but now I see.\n"
"[SLIDE 8]\n"
"Twas grace that taught my heart to fear,\n"
"And grace my fears relieved;\n"
"How precious did that grace appear\n"
"The hour I first believed.\n"
"[SLIDE 8]\n"
"Through many dangers, toils, and snares,\n"
"I have already come;\n"
"Tis grace hath brought me safe thus far,\n"
"And grace will lead me home.\n"
"[SLIDE 8]\n"
"When weve been there ten thousand years,\n"
"Bright shining as the sun,\n"
"Weve no less days to sing Gods praise\n"
"Than when we first begun.\n"
"[SLIDE 8]\n"
"Holy, Holy, Holy\n"
"Holy, holy, holy! Lord God Almighty!\n"
"Early in the morning our song shall rise to thee;\n"
"Holy, holy, holy, merciful and mighty!\n"
"God in three persons, blessed Trinity!\n"
"[SLIDE 8]\n"
"Holy, holy, holy! all the saints adore thee,\n"
"Casting down their golden crowns around the glassy sea;\n"
"Cherubim and seraphim falling down before thee,\n"
"Which wert and art and evermore shalt be.\n"
"[SLIDE 8]\n"
"Holy, holy, holy! though the darkness hide thee,\n"
"Though the eye of sinful man thy glory may not see;\n"
"Only thou art holy; there is none beside thee,\n"
"Perfect in power, in love, and purity.\n"
"[SLIDE 8]\n"
"Holy, holy, holy! Lord God Almighty!\n"
"All thy works shall praise thy name, in earth, and sky, and sea;\n"
"Holy, holy, holy, merciful and mighty!\n"
"God in three persons, blessed Trinity!\n"
"[SLIDE 8]\n"
"Blessed Assurance\n"
"Blessed assurance, Jesus is mine!\n"
"O what a foretaste of glory divine!\n"
"Heir of salvation, purchase of God,\n"
"Born of his Spirit, washed in his blood.\n"
"[SLIDE 8]\n"
"This is my story, this is my song,\n"
"Praising my Savior all the day long;\n"
"This is my story, this is my song,\n"
"Praising my Savior all the day long.\n"
"[SLIDE 8]\n"
"Perfect submission, perfect delight,\n"
"Visions of rapture now burst on my sight;\n"
"Angels descending, bring from above\n"
"Echoes of mercy, whispers of love.\n"
"[SLIDE 8]\n"
"This is my story, this is my song,\n"
"Praising my Savior all the day long;\n"
"This is my story, this is my song,\n"
"Praising my Savior all the day long.\n"
"[SLIDE 8]\n"
"Perfect submission, all is at rest;\n"
"I in my Savior am happy and blest,\n"
"Watching and waiting, looking above,\n"
"Filled with his goodness, lost in his love.\n"
"[SLIDE 8]\n"
"This is my story, this is my song,\n"
"Praising my Savior all the day long;\n"
"This is my story, this is my song,\n"
"Praising my Savior all the day long.\n"
"[SLIDE 8]\n"
"Rock of Ages\n"
"Rock of Ages, cleft for me,\n"
"Let me hide myself in thee;\n"
"Let the water and the blood,\n"
"From thy wounded side which flowed,\n"
"Be of sin the double cure,\n"
"Save from wrath and make me pure.\n"
"[SLIDE 8]\n"
"Not the labors of my hands\n"
"Can fulfill thy laws demands;\n"
"Could my zeal no respite know,\n"
"Could my tears forever flow,\n"
"All for sin could not atone;\n"
"Thou must save, and thou alone.\n"
"[SLIDE 8]\n"
"While I draw this fleeting breath,\n"
"When my eyes shall close in death,\n"
"When I rise to worlds unknown,\n"
"And behold thee on thy throne,\n"
"Rock of Ages, cleft for me,\n"
"Let me hide myself in thee.\n"
"[SLIDE 8]\n"
"It Is Well\n"
"When peace, like a river, attendeth my way,\n"
"When sorrows like sea billows roll;\n"
"Whatever my lot, thou hast taught me to say,\n"
"It is well, it is well with my soul.\n"
"[SLIDE 8]\n"
"It is well, with my soul,\n"
"It is well, with my soul,\n"
"It is well, it is well, with my soul.\n"
"[SLIDE 8]\n"
"Though Satan should buffet, though trials should come,\n"
"Let this blest assurance control,\n"
"That Christ hath regarded my helpless estate,\n"
"And hath shed his own blood for my soul.\n"
"[SLIDE 8]\n"
"It is well, with my soul,\n"
"It is well, with my soul,\n"
"It is well, it is well, with my soul.\n"
"[SLIDE 8]\n"
"My sin, oh, the bliss of this glorious thought!\n"
"My sin, not in part but the whole,\n"
"Is nailed to the cross, and I bear it no more,\n"
"Praise the Lord, praise the Lord, O my soul!\n"
"[SLIDE 8]\n"
"It is well, with my soul,\n"
"It is well, with my soul,\n"
"It is well, it is well, with my soul.\n"
"[SLIDE 8]\n"
"All Hail the Power\n"
"All hail the power of Jesus name!\n"
"Let angels prostrate fall;\n"
"Bring forth the royal diadem,\n"
"And crown him Lord of all.\n"
"Bring forth the royal diadem,\n"
"And crown him Lord of all.\n"
"[SLIDE 8]\n"
"Ye chosen seed of Israels race,\n"
"Ye ransomed from the fall,\n"
"Hail him who saves you by his grace,\n"
"And crown him Lord of all.\n"
"Hail him who saves you by his grace,\n"
"And crown him Lord of all.\n"
"[SLIDE 8]\n"
"When I Survey\n"
"When I survey the wondrous cross\n"
"On which the Prince of glory died,\n"
"My richest gain I count but loss,\n"
"And pour contempt on all my pride.\n"
"[SLIDE 8]\n"
"Forbid it, Lord, that I should boast,\n"
"Save in the death of Christ my God;\n"
"All the vain things that charm me most,\n"
"I sacrifice them to his blood.\n"
"[SLIDE 8]\n"
"See, from his head, his hands, his feet,\n"
"Sorrow and love flow mingled down!\n"
"Did eer such love and sorrow meet,\n"
"Or thorns compose so rich a crown?\n"
"[SLIDE 8]\n"
"Were the whole realm of nature mine,\n"
"That were a present far too small;\n"
"Love so amazing, so divine,\n"
"Demands my soul, my life, my all.\n"
"[SLIDE 8]\n"
"A Mighty Fortress\n"
"A mighty fortress is our God,\n"
"A bulwark never failing;\n"
"Our helper he, amid the flood\n"
"Of mortal ills prevailing.\n"
"For still our ancient foe\n"
"Doth seek to work us woe;\n"
"His craft and power are great,\n"
"And armed with cruel hate,\n"
"On earth is not his equal.\n"
"[SLIDE 8]\n"
"Did we in our own strength confide,\n"
"Our striving would be losing;\n"
"Were not the right Man on our side,\n"
"The Man of Gods own choosing.\n"
"Dost ask who that may be?\n"
"Christ Jesus, it is he;\n"
"Lord Sabaoth his name,\n"
"From age to age the same,\n"
"And he must win the battle.\n";

static const char songbookHymns2[] =
"[TITLE Classic Hymns Vol 2]\n"
"[DURATION 8]\n"
"[SLIDE 8]\n"
"What a Friend We Have in Jesus\n"
"What a friend we have in Jesus,\n"
"All our sins and griefs to bear!\n"
"What a privilege to carry\n"
"Everything to God in prayer!\n"
"O what peace we often forfeit,\n"
"O what needless pain we bear,\n"
"All because we do not carry\n"
"Everything to God in prayer.\n"
"[SLIDE 8]\n"
"Have we trials and temptations?\n"
"Is there trouble anywhere?\n"
"We should never be discouraged,\n"
"Take it to the Lord in prayer.\n"
"Can we find a friend so faithful\n"
"Who will all our sorrows share?\n"
"Jesus knows our every weakness,\n"
"Take it to the Lord in prayer.\n"
"[SLIDE 8]\n"
"How Firm a Foundation\n"
"How firm a foundation, ye saints of the Lord,\n"
"Is laid for your faith in his excellent word!\n"
"What more can he say than to you he hath said,\n"
"You who unto Jesus for refuge have fled?\n"
"[SLIDE 8]\n"
"Fear not, I am with thee, O be not dismayed,\n"
"For I am thy God and will still give thee aid;\n"
"Ill strengthen thee, help thee, and cause thee to stand,\n"
"Upheld by my righteous, omnipotent hand.\n"
"[SLIDE 8]\n"
"Jesus Loves Me\n"
"Jesus loves me! this I know,\n"
"For the Bible tells me so;\n"
"Little ones to him belong,\n"
"They are weak, but he is strong.\n"
"[SLIDE 8]\n"
"Yes, Jesus loves me!\n"
"Yes, Jesus loves me!\n"
"Yes, Jesus loves me!\n"
"The Bible tells me so.\n"
"[SLIDE 8]\n"
"The Old Rugged Cross\n"
"On a hill far away stood an old rugged cross,\n"
"The emblem of suffering and shame;\n"
"And I love that old cross where the dearest and best\n"
"For a world of lost sinners was slain.\n"
"[SLIDE 8]\n"
"So Ill cherish the old rugged cross,\n"
"Till my trophies at last I lay down;\n"
"I will cling to the old rugged cross,\n"
"And exchange it some day for a crown.\n"
"[SLIDE 8]\n"
"Sweet Hour of Prayer\n"
"Sweet hour of prayer! sweet hour of prayer!\n"
"That calls me from a world of care,\n"
"And bids me at my Fathers throne\n"
"Make all my wants and wishes known.\n"
"In seasons of distress and grief,\n"
"My soul has often found relief,\n"
"And oft escaped the tempters snare\n"
"By thy return, sweet hour of prayer!\n"
"[SLIDE 8]\n"
"Come Thou Fount\n"
"Come, thou Fount of every blessing,\n"
"Tune my heart to sing thy grace;\n"
"Streams of mercy, never ceasing,\n"
"Call for songs of loudest praise.\n"
"Teach me some melodious sonnet,\n"
"Sung by flaming tongues above;\n"
"Praise the mount! Im fixed upon it,\n"
"Mount of thy redeeming love.\n"
"[SLIDE 8]\n"
"Nearer, My God, to Thee\n"
"Nearer, my God, to thee, nearer to thee!\n"
"Een though it be a cross that raiseth me,\n"
"Still all my song shall be, nearer, my God, to thee;\n"
"Nearer, my God, to thee, nearer to thee!\n"
"[SLIDE 8]\n"
"Guide Me, O Thou Great Jehovah\n"
"Guide me, O thou great Jehovah,\n"
"Pilgrim through this barren land;\n"
"I am weak, but thou art mighty,\n"
"Hold me with thy powerful hand;\n"
"Bread of heaven, bread of heaven,\n"
"Feed me till I want no more.\n"
"[SLIDE 8]\n"
"Open now the crystal fountain,\n"
"Whence the healing stream doth flow;\n"
"Let the fiery, cloudy pillar\n"
"Lead me all my journey through;\n"
"Strong Deliverer, strong Deliverer,\n"
"Be thou still my strength and shield.\n";

static const char songbookSwahili[] =
"[TITLE Nyimbo za Kikristo (Kiswahili)]\n"
"[DURATION 8]\n"
"[SLIDE 8]\n"
"Bwana Ni Mchungaji Wangu (Zaburi 23)\n"
"Bwana ni mchungaji wangu,\n"
"sitaona upungufu;\n"
"anilaza kwenye malisho mabichi,\n"
"huniongoza kando ya maji yaliyotulia.\n"
"[SLIDE 8]\n"
"Huirudisha nafsi yangu;\n"
"huniongoza katika njia za haki,\n"
"kwa ajili ya jina lake.\n"
"[SLIDE 8]\n"
"Naam, hata nikipita katikati ya bonde la uvuli wa mauti,\n"
"sitaogopa mabaya;\n"
"kwa kuwa wewe u pamoja nami;\n"
"fimbo yako na mkongojo wako\n"
"ndivyo vinifariji.\n"
"[SLIDE 8]\n"
"Waandaa meza mbele yangu\n"
"mbele ya adui zangu;\n"
"wapaka mafuta kichwa changu,\n"
"kikombe changu kinajaa mno.\n"
"[SLIDE 8]\n"
"Hakika wema na fadhili vitanifuata\n"
"siku zote za maisha yangu;\n"
"nami nitakaa katika nyumba ya Bwana siku zote.\n"
"[SLIDE 8]\n"
"Neema Ya Ajabu\n"
"Neema ya ajabu! tamu sauti yake,\n"
"iliyoniokoa mimi mwovu.\n"
"Nilipotea, lakini sasa nimepatikana,\n"
"nilikuwa kipofu, lakini sasa naona.\n"
"[SLIDE 8]\n"
"Ni neema iliyofundisha moyo wangu kuogopa,\n"
"na neema ikaniondoa hofu yangu;\n"
"neema ile ilionekana tamu sana\n"
"saa ile niliyoiamini kwanza.\n"
"[SLIDE 8]\n"
"Kupitia hatari, taabu, na mitego,\n"
"nimeshapita mpaka sasa;\n"
"ni neema iliyonileta salama hadi hapa,\n"
"na neema itaniongoza hadi nyumbani.\n"
"[SLIDE 8]\n"
"Mungu Ni Upendo\n"
"Mungu ni upendo, Mungu ni upendo,\n"
"Mungu ni upendo, ndiye kwanza kutupenda.\n"
"Na sisi tuwapende wengine,\n"
"kwa kuwa Mungu ni upendo.\n"
"[SLIDE 8]\n"
"Zaburi 100\n"
"Mlizeni Bwana kwa furaha,\n"
"wajini mbele zake kwa nyimbo za shangwe;\n"
"Mtumikieni Bwana kwa furaha,\n"
"ingieni mbele zake kwa wimbo wa shukrani.\n"
"[SLIDE 8]\n"
"Mjue ya kuwa Bwana ndiye Mungu;\n"
"ndiye aliyetufanya, na sisi tu wake;\n"
"tunapaswa kumshukuru,\n"
"na kumwabariki jina lake.\n";

typedef struct {
    const char *code;
    const char *title;
    const char *lang;
    const char *content;
} BundledSongbook;

static const BundledSongbook g_bundledSongbooks[] = {
    { "classic-hymns-1", "Classic Hymns Vol 1", "en", songbookHymns1 },
    { "classic-hymns-2", "Classic Hymns Vol 2", "en", songbookHymns2 },
    { "nyimbo-za-kikristo", "Nyimbo za Kikristo (Kiswahili)", "sw", songbookSwahili },
};

static const int g_bundledSongbookCount =
    (int)(sizeof(g_bundledSongbooks) / sizeof(g_bundledSongbooks[0]));

static void addSongbookItems(void)
{
    extern const BundledSongbook g_bundledSongbooks[];
    extern const int g_bundledSongbookCount;
    int i;
    for (i = 0; i < g_bundledSongbookCount && g_dlCount < MAX_DL_ITEMS; i++) {
        DlItem *it = &g_dlItems[g_dlCount];
        it->isSongbook = 1;
        snprintf(it->code, sizeof(it->code), "%s", g_bundledSongbooks[i].code);
        snprintf(it->title, sizeof(it->title), "%s",
            g_bundledSongbooks[i].title);
        snprintf(it->langCode, sizeof(it->langCode), "%s",
            g_bundledSongbooks[i].lang);
        snprintf(it->langName, sizeof(it->langName), "%s",
            strcmp(g_bundledSongbooks[i].lang, "sw") == 0
                ? "Kiswahili" : "English");
        it->content = g_bundledSongbooks[i].content;
        g_dlCount++;
    }
}

static void doDownloads(HWND dlg)
{
    HWND lb = GetDlgItem(dlg, IDDL_LIST);
    int selCount, i, ok, fail;
    int *sels;
    selCount = (int)SendMessageA(lb, LB_GETSELCOUNT, 0, 0);
    if (selCount <= 0) {
        setDlStatus("Select at least one item, then press Download Selected.");
        return;
    }
    sels = malloc(sizeof(int) * selCount);
    if (!sels)
        return;
    SendMessageA(lb, LB_GETSELITEMS, selCount, (LPARAM)sels);
    EnableWindow(GetDlgItem(dlg, IDDL_DOWNLOAD), FALSE);
    g_dlBusy = 1;
    ok = 0;
    fail = 0;
    for (i = 0; i < selCount; i++) {
        int idx = (i < g_dlMapCount) ? g_dlMap[sels[i]] : -1;
        char msg[DL_LEN + 40];
        if (idx < 0 || idx >= g_dlCount)
            continue;
        snprintf(msg, sizeof(msg), "Downloading  %s ...", g_dlItems[idx].title);
        setDlStatus(msg);
        setDlProgress(30);
        pumpUi();
        if (g_dlItems[idx].isSongbook) {
            if (installSongbookItem(idx))
                ok++;
            else
                fail++;
        } else {
            if (downloadBibleItem(idx))
                ok++;
            else
                fail++;
        }
        setDlProgress(100);
        pumpUi();
    }
    free(sels);
    setDlProgress(0);
    refreshLanguagesUI();
    {
        char done[200];
        snprintf(done, sizeof(done), "Done: %d installed, %d failed. Library refreshed.",
            ok, fail);
        setDlStatus(done);
    }
    g_dlBusy = 0;
    EnableWindow(GetDlgItem(dlg, IDDL_DOWNLOAD), TRUE);
}

static LRESULT CALLBACK downloadDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HINSTANCE inst = GetModuleHandle(NULL);
        HWND c;
        c = CreateWindowExA(0, "BUTTON", "Bibles",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
            12, 10, 90, 22, hwnd, (HMENU)IDDL_BIBLE, inst, NULL);
        SendMessageA(c, WM_SETFONT, (WPARAM)f, TRUE);
        c = CreateWindowExA(0, "BUTTON", "Song Books",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            108, 10, 110, 22, hwnd, (HMENU)IDDL_SONGBOOK, inst, NULL);
        SendMessageA(c, WM_SETFONT, (WPARAM)f, TRUE);
        CheckRadioButton(hwnd, IDDL_BIBLE, IDDL_SONGBOOK,
            g_dlShowBibles ? IDDL_BIBLE : IDDL_SONGBOOK);
        c = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_EXTENDEDSEL | WS_BORDER,
            12, 38, 500, 240, hwnd, (HMENU)IDDL_LIST, inst, NULL);
        SendMessageA(c, WM_SETFONT, (WPARAM)f, TRUE);
        c = CreateWindowExA(0, "msctls_progress32", "",
            WS_CHILD | WS_VISIBLE,
            12, 286, 500, 18, hwnd, (HMENU)IDDL_PROGRESS, inst, NULL);
        SendMessageA(c, PBM_SETRANGE32, 0, 100);
        c = CreateWindowExA(0, "STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            12, 312, 500, 60, hwnd, (HMENU)IDDL_STATUS, inst, NULL);
        SendMessageA(c, WM_SETFONT, (WPARAM)f, TRUE);
        c = CreateWindowExA(0, "BUTTON", "Download Selected",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            12, 378, 170, 34, hwnd, (HMENU)IDDL_DOWNLOAD, inst, NULL);
        SendMessageA(c, WM_SETFONT, (WPARAM)f, TRUE);
        c = CreateWindowExA(0, "BUTTON", "Close",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            192, 378, 100, 34, hwnd, (HMENU)IDDL_CLOSE, inst, NULL);
        SendMessageA(c, WM_SETFONT, (WPARAM)f, TRUE);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (code == BN_CLICKED) {
            if (id == IDDL_BIBLE || id == IDDL_SONGBOOK) {
                g_dlShowBibles = (id == IDDL_BIBLE);
                CheckRadioButton(hwnd, IDDL_BIBLE, IDDL_SONGBOOK,
                    g_dlShowBibles ? IDDL_BIBLE : IDDL_SONGBOOK);
                fillDlList(hwnd);
                return 0;
            }
            if (id == IDDL_DOWNLOAD) {
                if (!g_dlBusy)
                    doDownloads(hwnd);
                return 0;
            }
            if (id == IDDL_CLOSE) {
                if (!g_dlBusy)
                    DestroyWindow(hwnd);
                return 0;
            }
        }
        return 0;
    }

    case WM_DESTROY:
        g_dlHwnd = NULL;
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openDownloadDialog(HWND parent)
{
    HWND dlg;
    if (g_dlHwnd) {
        SetForegroundWindow(g_dlHwnd);
        return;
    }
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "DownloadDlgClass",
        "Download Library - Domaine Church Presenter",
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        140, 90, 526, 452, parent, NULL, GetModuleHandle(NULL), NULL);
    if (!dlg)
        return;
    g_dlHwnd = dlg;
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
    fetchDlItems();
    addSongbookItems();
    fillDlList(dlg);
    setDlStatus("Select one or more Bibles or song books, then press Download Selected.");
}

static LRESULT CALLBACK alertDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDAL_OK) {
            char buf[sizeof(g_alert)];
            GetDlgItemTextA(hwnd, IDAL_TEXT, buf, sizeof(buf));
            snprintf(g_alert, sizeof(g_alert), "%s", buf);
            g_alertActive = g_alert[0] ? 1 : 0;
            g_alertElapsed = 0;
            g_alertTotal = 12000;
            g_alertHwnd = NULL;
            DestroyWindow(hwnd);
            refreshAll();
        } else if (id == IDAL_CANCEL) {
            g_alertHwnd = NULL;
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        g_alertHwnd = NULL;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openAlertDialog(HWND parent)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    if (g_alertHwnd) {
        SetForegroundWindow(g_alertHwnd);
        return;
    }
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "AlertDlgClass",
        "Alert Message - Domaine Church Presenter",
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        200, 140, 496, 200, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    c = CreateWindowExA(0, "STATIC",
        "Show a scrolling message at the bottom of the live output for 12 seconds.",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 12, 10, 460, 20, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN,
        12, 34, 460, 96, dlg, (HMENU)IDAL_TEXT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Show Alert",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 296, 142, 84, 28, dlg, (HMENU)IDAL_OK, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE, 388, 142, 84, 28, dlg, (HMENU)IDAL_CANCEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    g_alertHwnd = dlg;
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    WNDCLASSA wc;
    HWND mainWnd, dispWnd;
    MSG msg;
    char modulePath[PATH_BUF];
    int i;

    (void)hPrev;
    (void)lpCmd;

    GetModuleFileNameA(NULL, modulePath, PATH_BUF);
    getExeDir(modulePath);
    findSlidesDir();

    {
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icc);
    }

    enumerateLanguages();
    refreshLanguagesUI();
    {
        int any = 0;
        for (i = 0; i < g_langCount; i++)
            any += g_setCounts[i];
        if (any <= 0) {
            MessageBoxA(NULL,
                "No .txt slide files found.\n\n"
                "Create language folders such as 'slides\\English' and "
                "'slides\\Kiswahili' and put .txt files inside using "
                "[TITLE ...] and [SLIDE n] lines.",
                "Domaine Church Presenter", MB_OK | MB_ICONERROR);
            return 1;
        }
    }
    loadSetById(0);

    memset(&wc, 0, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = mainWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ChurchMainClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = dispWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ChurchDispClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = previewProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "SlidePreviewClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = downloadDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "DownloadDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = alertDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "AlertDlgClass";
    RegisterClassA(&wc);

    mainWnd = CreateWindowExA(0, "ChurchMainClass", "Domaine Church Presenter - Controller",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        60, 40, 976, 650, NULL, NULL, hInst, NULL);
    g_main = mainWnd;

    {
        HMENU bar = CreateMenu();
        HMENU tools = CreateMenu();
        AppendMenuA(tools, MF_STRING, IDM_TOOLS_DOWNLOAD, "Download Library...");
        AppendMenuA(tools, MF_STRING, IDM_TOOLS_REFRESH, "Refresh Library");
        AppendMenuA(tools, MF_STRING, IDM_TOOLS_ALERT, "Alert Message...");
        AppendMenuA(bar, MF_POPUP, (UINT_PTR)tools, "Tools");
        SetMenu(mainWnd, bar);
    }

    dispWnd = CreateWindowExA(0, "ChurchDispClass", "Live Output - Domaine Church Presenter",
        WS_OVERLAPPEDWINDOW, 300, 120, 1280, 720, NULL, NULL, hInst, NULL);
    g_disp = dispWnd;

    if (!mainWnd || !dispWnd) {
        MessageBoxA(NULL, "Could not create a window.", "Domaine Church Presenter", MB_OK | MB_ICONERROR);
        return 1;
    }

    {
        HWND cb = GetDlgItem(mainWnd, IDC_LANG);
        for (i = 0; i < g_langCount; i++)
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)g_langFolders[i]);
        SendMessage(cb, CB_SETCURSEL, g_langIndex, 0);
    }
    repopulateSets();
    refreshAll();

    ShowWindow(mainWnd, nShow);
    ShowWindow(dispWnd, SW_SHOWNORMAL);
    UpdateWindow(mainWnd);
    UpdateWindow(dispWnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_dispFontBig)
        DeleteObject(g_dispFontBig);
    if (g_dispFontSmall)
        DeleteObject(g_dispFontSmall);
    if (g_prevFont)
        DeleteObject(g_prevFont);
    return (int)msg.wParam;
}
