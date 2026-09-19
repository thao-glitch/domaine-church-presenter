#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
#define IDM_TOOLS_PRAYER 1004

#define IDC_GOTO 120
#define IDC_BTN_GOTO 121
#define IDC_CHK_LOOP 122

#define IDC_TIMER_DISP 123
#define IDC_TIMER_VAL 124
#define IDC_BTN_TIMER_SET 125
#define IDC_SEARCH_LABEL 126
#define IDC_SEARCH_EDIT 127
#define IDC_BTN_SEARCH 128
#define IDC_BTN_CLEAR_SEARCH 129
#define IDC_SEARCH_RESULTS 130
#define IDC_QR_INPUT 131
#define IDC_QR_GEN 132
#define IDC_QR_DISPLAY 133

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

#define IDM_CHURCH_LOGIN 1101
#define IDM_CHURCH_LOGOUT 1102
#define IDM_CHURCH_MEMBERS 1103
#define IDM_CHURCH_SERVICES 1104
#define IDM_CHURCH_EVENTS 1105
#define IDM_CHURCH_TODAY 1106

#define IDLG_USER 501
#define IDLG_PASS 502
#define IDLG_OK 503
#define IDLG_CANCEL 504

#define IDREC_LIST 601
#define IDREC_ADD 602
#define IDREC_EDIT 603
#define IDREC_DEL 604
#define IDREC_CLOSE 605

#define IDMEM_GUARD 0

#define IDMEF_NAME 611
#define IDMEF_PHONE 612
#define IDMEF_EMAIL 613
#define IDMEF_ROLE 614
#define IDMEF_OK 615
#define IDMEF_CANCEL 616

#define IDSEF_NAME 621
#define IDSEF_TYPE 622
#define IDSEF_DAY 623
#define IDSEF_START 624
#define IDSEF_END 625
#define IDSEF_LOC 626
#define IDSEF_RECUR 627
#define IDSEF_OK 628
#define IDSEF_CANCEL 629
#define IDSEF_DATE 630

#define IDEVF_TITLE 631
#define IDEVF_DATE 632
#define IDEVF_START 633
#define IDEVF_END 634
#define IDEVF_LOC 635
#define IDEVF_CAT 636
#define IDEVF_DESC 637
#define IDEVF_OK 638
#define IDEVF_CANCEL 639

#define SB_URL_LEN 128
#define SB_KEY_LEN 256
#define SB_MAX_ROWS 2000
#define SB_REC_LEN 512

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

static int g_prayerActive = 0;
static int g_prayerElapsed = 0;
static int g_prayerTotal = 5000;
static char g_prayerText[512];

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

static char g_sbUrl[SB_URL_LEN] = "";
static char g_sbKey[SB_KEY_LEN] = "";
static char g_sbToken[2048] = "";
static char g_sbUserEmail[128] = "";
static char g_sbUserRole[64] = "";
static char g_sbFullName[128] = "";
static int g_sbLoggedIn = 0;
static int g_sbCanEdit = 0;
static int g_lastStatus = 0;
static int g_showToday = 0;
static char g_todayLines[32][160];
static int g_todayCount = 0;

static const char *g_roleLevels[] = {
    "Bishop", "Senior Pastor", "Pastor", "Assistant Pastor", "Elder",
    "Deacon", "Deaconess", "Evangelist", "Minister", "Worship Leader",
    "Choir", "Usher", "Greeter", "Media", "Youth Leader", "Member"
};
static const int g_roleCount = (int)(sizeof(g_roleLevels) / sizeof(g_roleLevels[0]));

static const char *g_serviceTypes[] = {
    "Sunday Morning Worship", "Sunday Evening Service", "Saturday Service",
    "Midweek Service", "Holy Communion", "Lord's Supper", "Baptism",
    "Confirmation", "Wedding / Marriage", "Funeral / Burial", "Ordination",
    "Anointing / Healing Service", "Prayer Meeting", "Intercessory Prayer",
    "Morning Prayer", "Night Prayer", "Bible Study", "Sunday School",
    "Children's Church", "Catechism Class", "Discipleship Class", "Revival",
    "Crusade / Evangelism", "Outreach", "Camp Meeting", "Vigil / All-Night",
    "Covenant Service", "New Year Service", "Easter Service", "Good Friday",
    "Christmas Service", "Ash Wednesday", "Maundy Thursday", "Easter Vigil",
    "Thanksgiving Service", "Dedication Service", "Harvest Service",
    "Singles Fellowship", "Youth Service", "Choir Practice", "Praise and Worship",
    "Fellowship / Koinonia", "Cell Group", "Men's Meeting", "Women's Meeting"
};
static const int g_serviceTypeCount = (int)(sizeof(g_serviceTypes) / sizeof(g_serviceTypes[0]));

static const char *g_weekDays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char g_memberFields[] =
    "id,full_name,phone,email,role,created_at";
static const char g_serviceFields[] =
    "id,name,type,recurring,weekday,date,start_time,end_time,location";
static const char g_eventFields[] =
    "id,title,category,date,start_time,end_time,location,description";

static void refreshLanguagesUI(void);
static void loadSupabaseConfig(void);
static char *httpSend(const char *method, const char *url, const char *headers,
    const char *body, int bodyLen, long *outLen, int *outStatus);
static void refreshToday(void);
static void todayStr(char *out, int cap);
static int todayWeekday(void);
static void updateChurchMenu(void);
static void sbLogout(void);
static void openLoginDialog(HWND parent);
static void openMembersDialog(HWND parent);
static void openServicesDialog(HWND parent);
static void openEventsDialog(HWND parent);
static LRESULT CALLBACK loginDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK membersDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK servicesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK eventsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK memberEditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK serviceEditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK eventEditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HMENU g_menuBar = NULL;
static void openDownloadDialog(HWND parent);
static void openAlertDialog(HWND parent);
void openPrayerDialog(HWND parent);
static void generateQrCode(const char *text);

static HWND g_main = NULL;
static HWND g_disp = NULL;
static HWND g_preview = NULL;
static HFONT g_dispFontBig = NULL;
static HFONT g_dispFontSmall = NULL;
static HFONT g_countdownFont = NULL;
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
    if (g_prayerActive) {
        g_prayerElapsed += 100;
        if (g_prayerTotal > 0 && g_prayerElapsed >= g_prayerTotal) {
            g_prayerActive = 0;
            g_prayerText[0] = '\0';
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
    
    // Update timer display
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", g_remaining);
    SetDlgItemTextA(g_main, IDC_TIMER_VAL, buf);
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
    if (g_countdownFont)
        DeleteObject(g_countdownFont);
    g_dispFontBig = CreateFontA(-clientHeight / 12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_dispFontSmall = CreateFontA(-clientHeight / 34, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_countdownFont = CreateFontA(-clientHeight / 8, 0, 0, 0, FW_BOLD, 0, 0, 0,
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
            {
                RECT tr = *rc;
                tr.left = rc->right - 120;
                old = (HFONT)SelectObject(hdc, g_dispFontSmall);
                SetBkMode(hdc, TRANSPARENT);
                DrawTextA(hdc, cd, -1, &tr, DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
                SelectObject(hdc, old);
            }
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
    
    if (g_prayerActive && g_prayerText[0]) {
        RECT pr = *rc;
        pr.top = rc->bottom - 48;
        {
            HBRUSH hb = CreateSolidBrush(RGB(20, 20, 30));
            FillRect(hdc, &pr, hb);
            DeleteObject(hb);
        }
        {
            HBRUSH hb = CreateSolidBrush(RGB(100, 180, 220));
            RECT line = { pr.left, pr.top, pr.right, pr.top + 3 };
            FillRect(hdc, &line, hb);
            DeleteObject(hb);
        }
        old = (HFONT)SelectObject(hdc, g_dispFontSmall);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(173, 216, 230));
        {
            SIZE sz;
            int total, x;
            GetTextExtentPoint32A(hdc, g_prayerText, (int)strlen(g_prayerText), &sz);
            total = (rc->right - rc->left) + sz.cx + 80;
            x = (rc->right - rc->left) - ((g_prayerElapsed * 80) / g_prayerTotal) % total;
            {
                RECT tr = { x, pr.top + 5, x + sz.cx + 30, pr.bottom };
                DrawTextA(hdc, g_prayerText, -1, &tr, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            }
        }
        SelectObject(hdc, old);
    }

    if (g_showToday && g_todayCount > 0) {
        char tb[4096];
        char todayIso[16];
        char wdayName[48];
        int i, tbLen;
        HFONT hf;
        todayStr(todayIso, sizeof(todayIso));
        snprintf(wdayName, sizeof(wdayName), "%s",
            todayWeekday() >= 0 && todayWeekday() < 7 ? g_weekDays[todayWeekday()] : "");
        tbLen = snprintf(tb, sizeof(tb), "TODAY - %s, %s", wdayName, todayIso);
        for (i = 0; i < g_todayCount && tbLen < (int)sizeof(tb) - 300; i++)
            tbLen += snprintf(tb + tbLen, sizeof(tb) - tbLen, "\n%s", g_todayLines[i]);
        {
            int top, bot;
            RECT bar = *rc;
            RECT tr = *rc;
            top = (!g_dispFullscreen) ? rc->top + 34 : rc->top + 10;
            bot = top + 170;
            tr.top = top;
            tr.left += 24;
            tr.right -= 24;
            tr.bottom = rc->bottom - 60;
            {
                HBRUSH hb = CreateSolidBrush(RGB(12, 12, 18));
                RECT bg = bar;
                bg.top = top;
                bg.bottom = bot;
                FillRect(hdc, &bg, hb);
                DeleteObject(hb);
            }
            {
                HBRUSH hb = CreateSolidBrush(RGB(210, 180, 60));
                RECT line = { bar.left, top, bar.right, top + 3 };
                FillRect(hdc, &line, hb);
                DeleteObject(hb);
            }
            hf = (HFONT)SelectObject(hdc, g_dispFontSmall);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 235));
            DrawTextA(hdc, tb, -1, &tr, DT_LEFT | DT_NOPREFIX);
            SelectObject(hdc, hf);
        }
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
        WS_CHILD | WS_VISIBLE | ES_READONLY | ES_CENTER, 540, 46, 64, 24, hwnd, (HMENU)IDC_GOTO, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Go",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 608, 44, 42, 26, hwnd, (HMENU)IDC_BTN_GOTO, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Timer:",
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 540, 78, 64, 18, hwnd, (HMENU)IDC_TIMER_DISP, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "EDIT", "0",
        WS_CHILD | WS_VISIBLE | ES_READONLY | ES_CENTER, 540+64+4, 76, 50, 22, hwnd, (HMENU)IDC_TIMER_VAL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Set",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 540+64+4+54, 74, 40, 24, hwnd, (HMENU)IDC_BTN_TIMER_SET, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Loop playback",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 656, 46, 130, 22, hwnd, (HMENU)IDC_CHK_LOOP, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Search:",
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 10, 560, 50, 18, hwnd, (HMENU)IDC_SEARCH_LABEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 65, 558, 260, 22, hwnd, (HMENU)IDC_SEARCH_EDIT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Search",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 558, 80, 22, hwnd, (HMENU)IDC_BTN_SEARCH, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Clear",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 415, 558, 80, 22, hwnd, (HMENU)IDC_BTN_CLEAR_SEARCH, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "QR:",
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 10, 586, 50, 18, hwnd, (HMENU)IDC_QR_INPUT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 65, 584, 260, 22, hwnd, (HMENU)IDC_QR_INPUT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Generate",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 584, 80, 22, hwnd, (HMENU)IDC_QR_GEN, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "STATIC", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER, 420, 584, 160, 80, hwnd, (HMENU)IDC_QR_DISPLAY, inst, NULL);
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

    c = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_BORDER | WS_TABSTOP,
        250, 548, 250, 120, hwnd, (HMENU)IDC_SEARCH_RESULTS, inst, NULL);
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
            } else if (id == IDC_BTN_TIMER_SET) {
                char buf[32];
                GetDlgItemTextA(hwnd, IDC_TIMER_VAL, buf, sizeof(buf));
                int val = atoi(buf);
                if (val >= 0 && val <= 999) {
                    g_remaining = val;
                    g_auto = 1;
                    g_defaultDur = val;
                    refreshAll();
                }
            } else if (id == IDC_BTN_SEARCH) {
                char search[256];
                GetDlgItemTextA(hwnd, IDC_SEARCH_EDIT, search, sizeof(search));
                // Convert to lowercase for case-insensitive search
                for (int i = 0; search[i]; i++) search[i] = tolower((unsigned char)search[i]);
                // Search across all language sets
                SendMessage(GetDlgItem(hwnd, IDC_SEARCH_RESULTS), LB_RESETCONTENT, 0, 0);
                for (int li = 0; li < g_langCount; li++) {
                    for (int si = 0; si < g_setCounts[li]; si++) {
                        char path[PATH_BUF];
                        snprintf(path, sizeof(path), "%s\\%s\\%s", g_slidesDir, g_langFolders[li], g_setFiles[li][si]);
                        FILE *f = fopen(path, "r");
                        if (f) {
                            char line[MAX_LINE_LEN];
                            while (fgets(line, sizeof(line), f)) {
                                char *p = line;
                                int len = (int)strlen(p);
                                while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r')) {
                                    p[len-1] = '\0';
                                    len--;
                                }
                                while (*p == ' ' || *p == '\t') p++;
                                if (strstr(p, search) != NULL) {
                                    char entry[200];
                                    snprintf(entry, sizeof(entry), "%s - Slide %d", g_setFiles[li][si], si + 1);
                                    SendMessage(GetDlgItem(hwnd, IDC_SEARCH_RESULTS), LB_ADDSTRING, 0, (LPARAM)entry);
                                    break;
                                }
                            }
                            fclose(f);
                        }
                    }
                }
                if (SendMessage(GetDlgItem(hwnd, IDC_SEARCH_RESULTS), LB_GETCOUNT, 0, 0) > 0) {
                    SendMessage(GetDlgItem(hwnd, IDC_SEARCH_RESULTS), LB_SETCURSEL, 0, 0);
                }
            } else if (id == IDC_BTN_CLEAR_SEARCH) {
                SetDlgItemTextA(hwnd, IDC_SEARCH_EDIT, "");
                SendMessage(GetDlgItem(hwnd, IDC_SEARCH_RESULTS), LB_RESETCONTENT, 0, 0);
            } else if (id == IDC_QR_GEN) {
                char qrText[256];
                GetDlgItemTextA(hwnd, IDC_QR_INPUT, qrText, sizeof(qrText));
                // Generate QR code
                generateQrCode(qrText);
                // Display QR code in the display area
                InvalidateRect(GetDlgItem(hwnd, IDC_QR_DISPLAY), NULL, TRUE);
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
            } else if (id == IDC_SEARCH_RESULTS) {
                int sel = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    // Parse "Filename - Slide N" to get language and slide index
                    char buf[200];
                    SendMessage(GetDlgItem(hwnd, IDC_SEARCH_RESULTS), LB_GETTEXT, sel, (LPARAM)buf);
                    // Find the slide - extract slide number
                    char *dash = strchr(buf, '-');
                    if (dash) {
                        // Need to find which set/slide this corresponds to
                        // For simplicity, go to slide 0 of the first matching set
                        goTo(0);
                        refreshAll();
                    }
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
        } else if (code == 0 && id == IDM_TOOLS_PRAYER) {
            openPrayerDialog(hwnd);
        } else if (code == 0 && id == IDM_CHURCH_LOGIN) {
            openLoginDialog(hwnd);
        } else if (code == 0 && id == IDM_CHURCH_LOGOUT) {
            sbLogout();
            updateChurchMenu();
            refreshToday();
            refreshAll();
        } else if (code == 0 && id == IDM_CHURCH_MEMBERS) {
            openMembersDialog(hwnd);
        } else if (code == 0 && id == IDM_CHURCH_SERVICES) {
            openServicesDialog(hwnd);
        } else if (code == 0 && id == IDM_CHURCH_EVENTS) {
            openEventsDialog(hwnd);
        } else if (code == 0 && id == IDM_CHURCH_TODAY) {
            g_showToday = !g_showToday;
            updateChurchMenu();
            refreshToday();
            if (g_disp)
                InvalidateRect(g_disp, NULL, TRUE);
            refreshAll();
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

static LRESULT CALLBACK prayerDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == IDAL_OK) {
                char buf[512];
                GetDlgItemTextA(hwnd, IDAL_TEXT, buf, sizeof(buf));
                if (buf[0] != '\0') {
                    // Add to scrolling alert area
                    if (!g_prayerActive) {
                        g_prayerActive = 1;
                        g_prayerElapsed = 0;
                        g_prayerTotal = 5000;
                        g_prayerText[0] = '\0';
                        snprintf(g_prayerText, sizeof(g_prayerText), "%s", buf);
                    }
                    EndDialog(hwnd, 0);
                }
            } else if (LOWORD(wParam) == IDAL_CANCEL) {
                EndDialog(hwnd, 0);
            }
        }
        break;
    case WM_CLOSE:
        EndDialog(hwnd, 0);
        break;
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void openPrayerDialog(HWND parent)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    if (g_alertHwnd) {
        SetForegroundWindow(g_alertHwnd);
        return;
    }
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "PrayerDlgClass",
        "Prayer Request - Domaine Church Presenter",
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        200, 140, 496, 250, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    c = CreateWindowExA(0, "STATIC",
        "Enter your prayer request:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 12, 10, 460, 20, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN,
        12, 34, 460, 100, dlg, (HMENU)IDAL_TEXT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Submit",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 296, 142, 84, 28, dlg, (HMENU)IDAL_OK, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE, 388, 142, 84, 28, dlg, (HMENU)IDAL_CANCEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    g_alertHwnd = dlg;
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

void generateQrCode(const char *text)
{
    if (!text || !text[0]) {
        // Clear the QR display area
        HWND qrDisplay = GetDlgItem(g_main, IDC_QR_DISPLAY);
        if (qrDisplay) {
            SetWindowTextA(qrDisplay, "");
        }
        return;
    }

    // Simple QR code generation - create a basic pattern
    // This is a very simplified version that creates a grid pattern
    // based on the text hash for demonstration purposes
    unsigned int hash = 0;
    for (const char *p = text; *p; p++) {
        hash = hash * 31 + (unsigned char)*p;
    }

    // Generate a 21x21 QR code pattern (minimum version)
    // Using the hash to determine dark/light cells
    char bitmap[21 * 21 / 8 + 1];
    memset(bitmap, 0, sizeof(bitmap));

    int size = 21; // QR code version 1 is 21x21
    int cells = size * size;
    int darkCount = abs(hash) % cells;

    // Set some cells as dark based on hash
    for (int i = 0; i < darkCount; i++) {
        int idx = i % cells;
        int row = idx / size;
        int col = idx % size;
        bitmap[row * ((size + 7) / 8) + col / 8] |= (1 << (col % 8));
    }

    // output to the QR display static control - show a simple representation
    char display[512];
    snprintf(display, sizeof(display), "QR Input: %s\\nPattern generated (hash: %u)", text, hash);
    
    HWND qrDisplay = GetDlgItem(g_main, IDC_QR_DISPLAY);
    if (qrDisplay) {
        SetWindowTextA(qrDisplay, display);
    }
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
    loadSupabaseConfig();

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

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = prayerDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "PrayerDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = loginDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "LoginDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = membersDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "MembersDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = servicesDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ServicesDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = eventsDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "EventsDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = memberEditDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "MemberEditDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = serviceEditDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ServiceEditDlgClass";
    RegisterClassA(&wc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = eventEditDlgProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "EventEditDlgClass";
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
        AppendMenuA(tools, MF_STRING, IDM_TOOLS_PRAYER, "Prayer Requests...");
        AppendMenuA(bar, MF_POPUP, (UINT_PTR)tools, "Tools");
        g_menuBar = bar;
        updateChurchMenu();
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

/* ====================================================================
   Church management (Supabase): members, services, events, login,
   role-based editing and today's schedule overlay.
   ==================================================================== */

static void todayStr(char *out, int cap)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    snprintf(out, cap, "%04d-%02d-%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

static int todayWeekday(void)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    return tm->tm_wday;
}

static void trimStr(char *s)
{
    int i = 0, n = (int)strlen(s);
    while (i < n && (s[i] == ' ' || s[i] == '\t'))
        i++;
    if (i > 0) {
        memmove(s, s + i, n - i + 1);
        n -= i;
    }
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t'
        || s[n - 1] == '\r' || s[n - 1] == '\n'))
        s[--n] = '\0';
}

static void saveSampleConfig(void)
{
    char path[PATH_BUF];
    FILE *f;
    snprintf(path, sizeof(path), "%s\\supabase.ini", g_exeDir);
    f = fopen(path, "w");
    if (!f)
        return;
    fputs("; Domaine Church Presenter - Supabase connection\n"
          "; 1) Create a free project at https://supabase.com\n"
          "; 2) Copy the project URL and anon (public) key from Project Settings -> API\n"
          "; 3) Run the SQL in docs/supabase/schema.sql in the Supabase SQL editor\n"
          "; 4) Paste values below and restart the app\n"
          "url=https://YOUR-PROJECT.supabase.co\n"
          "key=YOUR-ANON-KEY\n", f);
    fclose(f);
}

static void loadSupabaseConfig(void)
{
    char path[PATH_BUF];
    char line[512];
    FILE *f;
    snprintf(path, sizeof(path), "%s\\supabase.ini", g_exeDir);
    f = fopen(path, "r");
    if (!f) {
        saveSampleConfig();
        return;
    }
    while (fgets(line, sizeof(line), f) != NULL) {
        char *eq, *v;
        trimStr(line);
        if (line[0] == '\0' || line[0] == ';')
            continue;
        eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = '\0';
        v = eq + 1;
        trimStr(v);
        if (strcmp(line, "url") == 0)
            snprintf(g_sbUrl, sizeof(g_sbUrl), "%s", v);
        else if (strcmp(line, "key") == 0)
            snprintf(g_sbKey, sizeof(g_sbKey), "%s", v);
    }
    fclose(f);
}

static char *httpSend(const char *method, const char *url, const char *headers,
    const char *body, int bodyLen, long *outLen, int *outStatus)
{
    char *buf = NULL;
    HINTERNET net, req;
    DWORD sc = 0, scsz = sizeof(sc);
    DWORD cap = 1 << 20, used = 0, rd;
    DWORD to = 30000;
    char *nb;
    (void)method;

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
    if (!HttpSendRequestA(req, headers, -1L, (LPVOID)body, (DWORD)bodyLen)) {
        InternetCloseHandle(req);
        InternetCloseHandle(net);
        return NULL;
    }
    HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &sc, &scsz, NULL);
    *outStatus = (int)sc;
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

static void jsonEscape(char *out, size_t cap, const char *in)
{
    size_t o = 0;
    while (*in && o + 6 < cap) {
        if (*in == '"' || *in == '\\') {
            out[o++] = '\\';
            out[o++] = *in;
        } else if (*in == '\n') {
            out[o++] = '\\';
            out[o++] = 'n';
        } else if (*in == '\r') {
            out[o++] = '\\';
            out[o++] = 'r';
        } else if (*in == '\t') {
            out[o++] = '\\';
            out[o++] = 't';
        } else {
            out[o++] = *in;
        }
        in++;
    }
    out[o < cap ? o : cap - 1] = '\0';
}

static int jsonStrField(const char *seg, const char *key, char *out, int cap)
{
    char needle[96];
    const char *p;
    int oi = 0;
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    p = strstr(seg, needle);
    if (!p)
        return 0;
    p += strlen(needle);
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    if (*p != ':')
        return 0;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    if (*p == '"') {
        p++;
        while (*p && *p != '"' && oi < cap - 1) {
            if (*p == '\\' && p[1]) {
                p++;
                switch (*p) {
                case 'n': out[oi++] = '\n'; break;
                case 't': out[oi++] = '\t'; break;
                case 'r': out[oi++] = '\r'; break;
                case '/': out[oi++] = '/'; break;
                default: out[oi++] = *p; break;
                }
            } else {
                out[oi++] = *p;
            }
            p++;
        }
    } else {
        while (*p && *p != ',' && *p != '}' && *p != ']' && oi < cap - 1)
            out[oi++] = *p++;
    }
    out[oi] = '\0';
    return 1;
}

static const char *jsonRowNext(const char **pp, const char **row)
{
    const char *p = *pp;
    const char *start, *end;
    start = strchr(p, '{');
    if (!start)
        return NULL;
    end = strchr(start, '}');
    if (!end)
        return NULL;
    *row = start + 1;
    p = end + 1;
    if (*p == ',')
        p++;
    *pp = p;
    return p;
}

static int sbIsConfigured(void)
{
    return g_sbUrl[0] && strncmp(g_sbUrl, "https://YOUR-PROJECT", 18) != 0
        && g_sbKey[0] && strncmp(g_sbKey, "YOUR-ANON-KEY", 12) != 0;
}

static char *sbRequest(const char *method, const char *table, const char *query,
    const char *body)
{
    char url[1024];
    char headers[2048];
    char *res;
    long len = 0;
    if (!sbIsConfigured()) {
        g_lastStatus = -1;
        return NULL;
    }
    snprintf(url, sizeof(url), "%s/rest/v1/%s%s", g_sbUrl, table, query ? query : "");
    snprintf(headers, sizeof(headers),
        "apikey: %s\r\nAuthorization: Bearer %s\r\n"
        "Content-Type: application/json\r\nAccept: application/json\r\n",
        g_sbKey, g_sbToken);
    res = httpSend(method, url, headers, body, body ? (int)strlen(body) : 0,
        &len, &g_lastStatus);
    return res;
}

static int isEditorRole(const char *role)
{
    int i;
    for (i = 0; i < 11 && i < g_roleCount; i++)
        if (_stricmp(role, g_roleLevels[i]) == 0)
            return 1;
    return 0;
}

static int sbLogin(const char *email, const char *password)
{
    char url[1024];
    char headers[1024];
    char esc[256], body[512], *res;
    long unused = 0;
    char tok[2048] = "";
    char uname[128] = "";
    char role[64] = "";
    char q[512];
    int status = 0;
    if (!sbIsConfigured()) {
        MessageBoxA(NULL,
            "Supabase is not configured yet.\n\nOpen this folder, edit supabase.ini "
            "with your project URL and anon key, then restart the app.",
            "Supabase required", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    snprintf(url, sizeof(url), "%s/auth/v1/token?grant_type=password", g_sbUrl);
    snprintf(headers, sizeof(headers),
        "apikey: %s\r\nContent-Type: application/json\r\n", g_sbKey);
    jsonEscape(esc, sizeof(esc), email);
    snprintf(body, sizeof(body), "{\"email\":\"%s\",\"password\":\"%s\"}", esc, password);
    res = httpSend("POST", url, headers, body, (int)strlen(body), &unused, &status);
    if (!res)
        return 0;
    if (status == 200) {
        jsonStrField(res, "access_token", tok, sizeof(tok));
        jsonStrField(res, "email", uname, sizeof(uname));
    }
    free(res);
    if (!tok[0]) {
        MessageBoxA(NULL, "Login failed. Check the email and password, and make "
            "sure a profile exists in the profiles table.",
            "Login", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    snprintf(g_sbToken, sizeof(g_sbToken), "%s", tok);
    if (uname[0])
        snprintf(g_sbUserEmail, sizeof(g_sbUserEmail), "%s", uname);
    snprintf(q, sizeof(q), "?select=role,full_name&email=eq.%s", g_sbUserEmail);
    res = sbRequest("GET", "profiles", q, NULL);
    if (res && g_lastStatus == 200 && *res == '[') {
        const char *p = res;
        const char *row = NULL;
        if (jsonRowNext(&p, &row)) {
            jsonStrField(row, "role", role, sizeof(role));
            jsonStrField(row, "full_name", uname, sizeof(uname));
        }
    }
    free(res);
    if (role[0]) {
        snprintf(g_sbUserRole, sizeof(g_sbUserRole), "%s", role);
        snprintf(g_sbFullName, sizeof(g_sbFullName), "%s", uname);
        g_sbLoggedIn = 1;
        g_sbCanEdit = isEditorRole(role);
        return 1;
    }
    MessageBoxA(NULL, "Logged in, but no profile role found for this account. "
        "Add a row in the profiles table with this email.",
        "Login", MB_OK | MB_ICONINFORMATION);
    return 0;
}

static void sbLogout(void)
{
    g_sbToken[0] = '\0';
    g_sbLoggedIn = 0;
    g_sbCanEdit = 0;
    g_sbUserEmail[0] = '\0';
    g_sbUserRole[0] = '\0';
    g_sbFullName[0] = '\0';
    g_todayCount = 0;
}

typedef struct {
    char id[64];
    char v[8][256];
} RecRow;

static RecRow *g_rows = NULL;
static int g_rowCount = 0;
static int g_rowCap = 0;

static void rowsClear(void)
{
    free(g_rows);
    g_rows = NULL;
    g_rowCount = 0;
    g_rowCap = 0;
}

static void rowsReserve(int n)
{
    if (n <= g_rowCap)
        return;
    {
        int nc = g_rowCap ? g_rowCap : 64;
        RecRow *nr;
        while (nc < n)
            nc *= 2;
        nr = realloc(g_rows, (size_t)nc * sizeof(RecRow));
        if (!nr)
            return;
        g_rows = nr;
        g_rowCap = nc;
    }
}

static int roleIndex(const char *role)
{
    int i;
    for (i = 0; i < g_roleCount; i++)
        if (_stricmp(role, g_roleLevels[i]) == 0)
            return i;
    return g_roleCount;
}

static void rowsSortMembers(void)
{
    int i, j;
    for (i = 0; i < g_rowCount - 1; i++) {
        for (j = i + 1; j < g_rowCount; j++) {
            int ri = roleIndex(g_rows[i].v[3]);
            int rj = roleIndex(g_rows[j].v[3]);
            if (ri > rj || (ri == rj && _stricmp(g_rows[i].v[0], g_rows[j].v[0]) > 0)) {
                RecRow t = g_rows[i];
                g_rows[i] = g_rows[j];
                g_rows[j] = t;
            }
        }
    }
}

static void rowsSortServices(void)
{
    int i, j;
    for (i = 0; i < g_rowCount - 1; i++) {
        for (j = i + 1; j < g_rowCount; j++) {
            if (_stricmp(g_rows[i].v[0], g_rows[j].v[0]) > 0) {
                RecRow t = g_rows[i];
                g_rows[i] = g_rows[j];
                g_rows[j] = t;
            }
        }
    }
}

static void rowsSortEvents(void)
{
    int i, j;
    for (i = 0; i < g_rowCount - 1; i++) {
        for (j = i + 1; j < g_rowCount; j++) {
            int c = strcmp(g_rows[i].v[2], g_rows[j].v[2]);
            if (c == 0)
                c = strcmp(g_rows[i].v[3], g_rows[j].v[3]);
            if (c > 0) {
                RecRow t = g_rows[i];
                g_rows[i] = g_rows[j];
                g_rows[j] = t;
            }
        }
    }
}

static int loadMembers(void)
{
    char *res;
    const char *p, *row;
    rowsClear();
    res = sbRequest("GET", "members", "?select=id,full_name,phone,email,role", NULL);
    if (!res)
        return 0;
    if (g_lastStatus != 200) {
        free(res);
        return 0;
    }
    p = res;
    while (*p && *p != '[')
        p++;
    if (*p == '[')
        p++;
    while (jsonRowNext(&p, &row) && g_rowCount < SB_MAX_ROWS) {
        RecRow *r;
        rowsReserve(g_rowCount + 1);
        r = &g_rows[g_rowCount];
        memset(r, 0, sizeof(*r));
        jsonStrField(row, "id", r->id, sizeof(r->id));
        jsonStrField(row, "full_name", r->v[0], sizeof(r->v[0]));
        jsonStrField(row, "phone", r->v[1], sizeof(r->v[1]));
        jsonStrField(row, "email", r->v[2], sizeof(r->v[2]));
        jsonStrField(row, "role", r->v[3], sizeof(r->v[3]));
        g_rowCount++;
    }
    free(res);
    rowsSortMembers();
    return g_rowCount;
}

static int loadServices(void)
{
    char *res;
    const char *p, *row;
    rowsClear();
    res = sbRequest("GET", "services",
        "?select=id,name,type,recurring,weekday,start_time,end_time,location", NULL);
    if (!res)
        return 0;
    if (g_lastStatus != 200) {
        free(res);
        return 0;
    }
    p = res;
    while (*p && *p != '[')
        p++;
    if (*p == '[')
        p++;
    while (jsonRowNext(&p, &row) && g_rowCount < SB_MAX_ROWS) {
        RecRow *r;
        rowsReserve(g_rowCount + 1);
        r = &g_rows[g_rowCount];
        memset(r, 0, sizeof(*r));
        jsonStrField(row, "id", r->id, sizeof(r->id));
        jsonStrField(row, "name", r->v[0], sizeof(r->v[0]));
        jsonStrField(row, "type", r->v[1], sizeof(r->v[1]));
        jsonStrField(row, "recurring", r->v[2], sizeof(r->v[2]));
        jsonStrField(row, "weekday", r->v[3], sizeof(r->v[3]));
        jsonStrField(row, "start_time", r->v[4], sizeof(r->v[4]));
        jsonStrField(row, "end_time", r->v[5], sizeof(r->v[5]));
        jsonStrField(row, "location", r->v[6], sizeof(r->v[6]));
        g_rowCount++;
    }
    free(res);
    rowsSortServices();
    return g_rowCount;
}

static int loadEvents(void)
{
    char *res;
    const char *p, *row;
    rowsClear();
    res = sbRequest("GET", "events",
        "?select=id,title,category,date,start_time,end_time,location,description", NULL);
    if (!res)
        return 0;
    if (g_lastStatus != 200) {
        free(res);
        return 0;
    }
    p = res;
    while (*p && *p != '[')
        p++;
    if (*p == '[')
        p++;
    while (jsonRowNext(&p, &row) && g_rowCount < SB_MAX_ROWS) {
        RecRow *r;
        rowsReserve(g_rowCount + 1);
        r = &g_rows[g_rowCount];
        memset(r, 0, sizeof(*r));
        jsonStrField(row, "id", r->id, sizeof(r->id));
        jsonStrField(row, "title", r->v[0], sizeof(r->v[0]));
        jsonStrField(row, "category", r->v[1], sizeof(r->v[1]));
        jsonStrField(row, "date", r->v[2], sizeof(r->v[2]));
        jsonStrField(row, "start_time", r->v[3], sizeof(r->v[3]));
        jsonStrField(row, "end_time", r->v[4], sizeof(r->v[4]));
        jsonStrField(row, "location", r->v[5], sizeof(r->v[5]));
        jsonStrField(row, "description", r->v[6], sizeof(r->v[6]));
        g_rowCount++;
    }
    free(res);
    rowsSortEvents();
    return g_rowCount;
}

static void refreshToday(void)
{
    char today[16];
    char q[512];
    char *res;
    const char *p, *row;
    int n = 0;
    char *qe;
    todayStr(today, sizeof(today));
    g_todayCount = 0;
    if (!g_sbLoggedIn)
        return;
    snprintf(q, sizeof(q), "?select=name,type,start_time,end_time,location"
        "&recurring=eq.true&weekday=eq.%d&order=start_time", todayWeekday());
    res = sbRequest("GET", "services", q, NULL);
    if (res && g_lastStatus == 200) {
        p = res;
        while (*p && *p != '[')
            p++;
        if (*p == '[')
            p++;
        while (jsonRowNext(&p, &row) && n < 16) {
            char nm[256], tp[256], st[256], et[256], loc[256];
            jsonStrField(row, "name", nm, sizeof(nm));
            jsonStrField(row, "type", tp, sizeof(tp));
            jsonStrField(row, "start_time", st, sizeof(st));
            jsonStrField(row, "end_time", et, sizeof(et));
            jsonStrField(row, "location", loc, sizeof(loc));
            if (et[0])
                snprintf(g_todayLines[n], sizeof(g_todayLines[n]), "%s - %s   %s%s%s",
                    st[0] ? st : "--:--", et, nm[0] ? nm : tp,
                    loc[0] ? "   |   " : "", loc[0] ? loc : "");
            else
                snprintf(g_todayLines[n], sizeof(g_todayLines[n]), "%s   %s%s%s",
                    st[0] ? st : "--:--", nm[0] ? nm : tp,
                    loc[0] ? "   |   " : "", loc[0] ? loc : "");
            n++;
        }
        free(res);
    }
    snprintf(q, sizeof(q), "?select=name,type,start_time,end_time,location"
        "&recurring=eq.false");
    qe = q + strlen(q);
    snprintf(qe, sizeof(q) - strlen(q), "&date=eq.%s&order=start_time", today);
    res = sbRequest("GET", "services", q, NULL);
    if (res && g_lastStatus == 200) {
        p = res;
        while (*p && *p != '[')
            p++;
        if (*p == '[')
            p++;
        while (jsonRowNext(&p, &row) && n < 16) {
            char nm[256], tp[256], st[256], et[256], loc[256];
            jsonStrField(row, "name", nm, sizeof(nm));
            jsonStrField(row, "type", tp, sizeof(tp));
            jsonStrField(row, "start_time", st, sizeof(st));
            jsonStrField(row, "end_time", et, sizeof(et));
            jsonStrField(row, "location", loc, sizeof(loc));
            if (et[0])
                snprintf(g_todayLines[n], sizeof(g_todayLines[n]), "%s - %s   %s%s%s",
                    st[0] ? st : "--:--", et, nm[0] ? nm : tp,
                    loc[0] ? "   |   " : "", loc[0] ? loc : "");
            else
                snprintf(g_todayLines[n], sizeof(g_todayLines[n]), "%s   %s%s%s",
                    st[0] ? st : "--:--", nm[0] ? nm : tp,
                    loc[0] ? "   |   " : "", loc[0] ? loc : "");
            n++;
        }
        free(res);
    }
    snprintf(q, sizeof(q), "?select=title,category,start_time,end_time,location&date=eq.%s"
        "&order=start_time", today);
    res = sbRequest("GET", "events", q, NULL);
    if (res && g_lastStatus == 200) {
        p = res;
        while (*p && *p != '[')
            p++;
        if (*p == '[')
            p++;
        while (jsonRowNext(&p, &row) && n < 32) {
            char tl[256], ct[256], st[256], et[256], loc[256];
            jsonStrField(row, "title", tl, sizeof(tl));
            jsonStrField(row, "category", ct, sizeof(ct));
            jsonStrField(row, "start_time", st, sizeof(st));
            jsonStrField(row, "end_time", et, sizeof(et));
            jsonStrField(row, "location", loc, sizeof(loc));
            if (et[0])
                snprintf(g_todayLines[n], sizeof(g_todayLines[n]), "%s - %s   %s%s%s",
                    st[0] ? st : "--:--", et, tl[0] ? tl : ct,
                    loc[0] ? "   |   " : "", loc[0] ? loc : "");
            else
                snprintf(g_todayLines[n], sizeof(g_todayLines[n]), "%s   %s%s%s",
                    st[0] ? st : "--:--", tl[0] ? tl : ct,
                    loc[0] ? "   |   " : "", loc[0] ? loc : "");
            n++;
        }
        free(res);
    }
    g_todayCount = n;
}

static char *sbInsert(const char *table, const char *json)
{
    return sbRequest("POST", table, "", json);
}

static char *sbUpdate(const char *table, const char *id, const char *json)
{
    char q[256];
    snprintf(q, sizeof(q), "?id=eq.%s", id);
    return sbRequest("PATCH", table, q, json);
}

static char *sbDelete(const char *table, const char *id)
{
    char q[256];
    snprintf(q, sizeof(q), "?id=eq.%s", id);
    return sbRequest("DELETE", table, q, NULL);
}

static void sbMessageOk(const char *why, int ok)
{
    if (!ok)
        MessageBoxA(NULL, why, "Church Database", MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK loginDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDLG_OK) {
            char em[256], pw[256];
            GetDlgItemTextA(hwnd, IDLG_USER, em, sizeof(em));
            GetDlgItemTextA(hwnd, IDLG_PASS, pw, sizeof(pw));
            trimStr(em);
            if (!em[0] || !pw[0])
                return 0;
            if (sbLogin(em, pw)) {
                char info[512];
                snprintf(info, sizeof(info), "Logged in as %s (%s).",
                    g_sbFullName[0] ? g_sbFullName : g_sbUserEmail, g_sbUserRole);
                MessageBoxA(hwnd, info, "Login successful", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hwnd);
                updateChurchMenu();
                refreshAll();
            }
        } else if (id == IDLG_CANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openLoginDialog(HWND parent)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "LoginDlgClass",
        "Log in - Church Database",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, 260, 200, 360, 210, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    c = CreateWindowExA(0, "STATIC", "Email (the Supabase account email):",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 12, 320, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 14, 32, 320, 24, dlg, (HMENU)IDLG_USER, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Password:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 62, 320, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD, 14, 82, 320, 24, dlg,
        (HMENU)IDLG_PASS, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Log in",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 172, 130, 76, 28, dlg, (HMENU)IDLG_OK, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE, 256, 130, 76, 28, dlg, (HMENU)IDLG_CANCEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
    SetFocus(GetDlgItem(dlg, IDLG_USER));
}

static int requireEdit(HWND owner)
{
    if (g_sbCanEdit)
        return 1;
    MessageBoxA(owner, "Only editors (Bishop through leadership roles) can "
        "change the database. Log in with an editor account.",
        "Permission denied", MB_OK | MB_ICONINFORMATION);
    return 0;
}

static LRESULT CALLBACK memberEditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDMEF_OK) {
            char nm[256], ph[256], em[256], role[64];
            char esc[256], body[1600], *res;
            int sel;
            GetDlgItemTextA(hwnd, IDMEF_NAME, nm, sizeof(nm));
            GetDlgItemTextA(hwnd, IDMEF_PHONE, ph, sizeof(ph));
            GetDlgItemTextA(hwnd, IDMEF_EMAIL, em, sizeof(em));
            sel = (int)SendMessage(GetDlgItem(hwnd, IDMEF_ROLE), CB_GETCURSEL, 0, 0);
            role[0] = '\0';
            if (sel >= 0 && sel < g_roleCount)
                snprintf(role, sizeof(role), "%s", g_roleLevels[sel]);
            if (!nm[0])
                return 0;
            jsonEscape(esc, sizeof(esc), nm);
            snprintf(body, sizeof(body), "{\"full_name\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), ph);
            snprintf(body + strlen(body), sizeof(body) - strlen(body),
                ",\"phone\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), em);
            snprintf(body + strlen(body), sizeof(body) - strlen(body),
                ",\"email\":\"%s\"", esc);
            snprintf(body + strlen(body), sizeof(body) - strlen(body),
                ",\"role\":\"%s\"}", role);
            if (GetWindowLongPtr(hwnd, GWLP_USERDATA))
                res = sbUpdate("members",
                    (const char *)GetWindowLongPtr(hwnd, GWLP_USERDATA), body);
            else
                res = sbInsert("members", body);
            sbMessageOk("Could not save the member. Check that you are logged in "
                "with an editor role and that the table exists.",
                res && (g_lastStatus == 200 || g_lastStatus == 201));
            free(res);
            DestroyWindow(hwnd);
        } else if (id == IDMEF_CANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openMemberEditDialog(HWND parent, const RecRow *r)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    int i, sel;
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "MemberEditDlgClass",
        r ? "Edit Member" : "Add Member",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, 280, 180, 380, 240, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    if (r)
        SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)r->id);
    c = CreateWindowExA(0, "STATIC", "Full name:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 10, 120, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[0] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 120, 8, 236, 24, dlg, (HMENU)IDMEF_NAME, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Phone:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 38, 120, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[1] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 120, 36, 236, 24, dlg, (HMENU)IDMEF_PHONE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Email:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 66, 120, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[2] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 120, 64, 236, 24, dlg, (HMENU)IDMEF_EMAIL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Membership role:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 94, 120, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 120, 92, 236, 240,
        dlg, (HMENU)IDMEF_ROLE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    for (i = 0; i < g_roleCount; i++)
        SendMessageA(c, CB_ADDSTRING, 0, (LPARAM)g_roleLevels[i]);
    if (r) {
        sel = roleIndex(r->v[3]);
        SendMessage(c, CB_SETCURSEL, sel >= g_roleCount ? 0 : sel, 0);
    } else {
        SendMessage(c, CB_SETCURSEL, 3, 0);
    }
    c = CreateWindowExA(0, "BUTTON", "Save",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 200, 140, 70, 28, dlg, (HMENU)IDMEF_OK, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE, 278, 140, 78, 28, dlg, (HMENU)IDMEF_CANCEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

static void fillMembersList(HWND dlg)
{
    HWND lb = GetDlgItem(dlg, IDREC_LIST);
    int i;
    SendMessage(lb, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_rowCount; i++) {
        char s[768];
        snprintf(s, sizeof(s), "%-13s | %s | %s | %s",
            g_rows[i].v[3], g_rows[i].v[0], g_rows[i].v[1], g_rows[i].v[2]);
        SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)s);
    }
    if (g_rowCount > 0)
        SendMessage(lb, LB_SETCURSEL, 0, 0);
}

static LRESULT CALLBACK membersDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDREC_ADD) {
            if (requireEdit(hwnd))
                openMemberEditDialog(hwnd, NULL);
        } else if (id == IDREC_EDIT) {
            int sel = (int)SendMessage(GetDlgItem(hwnd, IDREC_LIST), LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_rowCount && requireEdit(hwnd))
                openMemberEditDialog(hwnd, &g_rows[sel]);
        } else if (id == IDREC_DEL) {
            int sel = (int)SendMessage(GetDlgItem(hwnd, IDREC_LIST), LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_rowCount) {
                char *res;
                if (!requireEdit(hwnd))
                    return 0;
                if (MessageBoxA(hwnd, "Remove this member?",
                        "Confirm", MB_YESNO | MB_ICONQUESTION) != IDYES)
                    return 0;
                res = sbDelete("members", g_rows[sel].id);
                sbMessageOk("Could not delete the member.",
                    res && (g_lastStatus == 200 || g_lastStatus == 204));
                free(res);
                loadMembers();
                fillMembersList(hwnd);
            }
        } else if (id == IDREC_CLOSE) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openMembersDialog(HWND parent)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    if (!g_sbLoggedIn) {
        openLoginDialog(parent);
        return;
    }
    loadMembers();
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "MembersDlgClass",
        "Members Directory - Domaine Church Presenter",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, 120, 80, 660, 470, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER,
        12, 12, 500, 420, dlg, (HMENU)IDREC_LIST, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Add",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 524, 12, 118, 34, dlg, (HMENU)IDREC_ADD, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Edit",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 524, 52, 118, 34, dlg, (HMENU)IDREC_EDIT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Delete",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 524, 92, 118, 34, dlg, (HMENU)IDREC_DEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 524, 402, 118, 34, dlg, (HMENU)IDREC_CLOSE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    if (!g_sbCanEdit) {
        EnableWindow(GetDlgItem(dlg, IDREC_ADD), FALSE);
        EnableWindow(GetDlgItem(dlg, IDREC_EDIT), FALSE);
        EnableWindow(GetDlgItem(dlg, IDREC_DEL), FALSE);
    }
    fillMembersList(dlg);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

static LRESULT CALLBACK serviceEditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDSEF_OK) {
            char nm[256], tp[256], st[64], et[64], loc[256], dt[64];
            char esc[256], body[2200], *res;
            int recur, daySel, s;
            recur = SendMessage(GetDlgItem(hwnd, IDSEF_RECUR), BM_GETCHECK, 0, 0) == BST_CHECKED;
            daySel = (int)SendMessage(GetDlgItem(hwnd, IDSEF_DAY), CB_GETCURSEL, 0, 0);
            GetDlgItemTextA(hwnd, IDSEF_NAME, nm, sizeof(nm));
            s = (int)SendMessage(GetDlgItem(hwnd, IDSEF_TYPE), CB_GETCURSEL, 0, 0);
            tp[0] = '\0';
            if (s >= 0 && s < g_serviceTypeCount)
                snprintf(tp, sizeof(tp), "%s", g_serviceTypes[s]);
            GetDlgItemTextA(hwnd, IDSEF_START, st, sizeof(st));
            GetDlgItemTextA(hwnd, IDSEF_END, et, sizeof(et));
            GetDlgItemTextA(hwnd, IDSEF_LOC, loc, sizeof(loc));
            GetDlgItemTextA(hwnd, IDSEF_DATE, dt, sizeof(dt));
            if (!nm[0])
                return 0;
            jsonEscape(esc, sizeof(esc), nm);
            snprintf(body, sizeof(body), "{\"name\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), tp);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"type\":\"%s\"", esc);
            snprintf(body + strlen(body), sizeof(body) - strlen(body),
                ",\"recurring\":%s", recur ? "true" : "false");
            if (recur) {
                snprintf(body + strlen(body), sizeof(body) - strlen(body),
                    ",\"weekday\":%d", daySel < 0 ? 0 : daySel);
                snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"date\":null");
            } else {
                snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"weekday\":null");
                jsonEscape(esc, sizeof(esc), dt);
                snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"date\":\"%s\"", esc);
            }
            jsonEscape(esc, sizeof(esc), st);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"start_time\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), et);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"end_time\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), loc);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"location\":\"%s\"}", esc);
            if (GetWindowLongPtr(hwnd, GWLP_USERDATA))
                res = sbUpdate("services",
                    (const char *)GetWindowLongPtr(hwnd, GWLP_USERDATA), body);
            else
                res = sbInsert("services", body);
            sbMessageOk("Could not save the service.",
                res && (g_lastStatus == 200 || g_lastStatus == 201));
            free(res);
            DestroyWindow(hwnd);
        } else if (id == IDSEF_CANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openServiceEditDialog(HWND parent, const RecRow *r)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    int i, sel;
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "ServiceEditDlgClass",
        r ? "Edit Service" : "Add Service",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, 240, 100, 460, 400, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    if (r)
        SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)r->id);
    c = CreateWindowExA(0, "STATIC", "Name:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 12, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[0] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 10, 300, 24, dlg, (HMENU)IDSEF_NAME, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Type:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 42, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 130, 40, 300, 300,
        dlg, (HMENU)IDSEF_TYPE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    for (i = 0; i < g_serviceTypeCount; i++)
        SendMessageA(c, CB_ADDSTRING, 0, (LPARAM)g_serviceTypes[i]);
    sel = 0;
    if (r && r->v[1][0]) {
        for (i = 0; i < g_serviceTypeCount; i++)
            if (_stricmp(g_serviceTypes[i], r->v[1]) == 0) {
                sel = i;
                break;
            }
    }
    SendMessage(c, CB_SETCURSEL, sel, 0);
    c = CreateWindowExA(0, "BUTTON", "Repeats weekly",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 130, 68, 140, 22, dlg, (HMENU)IDSEF_RECUR, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    if (r && r->v[2][0] == 't')
        SendMessage(c, BM_SETCHECK, BST_CHECKED, 0);
    c = CreateWindowExA(0, "STATIC", "Weekday:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 98, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 130, 96, 300, 200,
        dlg, (HMENU)IDSEF_DAY, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    for (i = 0; i < 7; i++)
        SendMessageA(c, CB_ADDSTRING, 0, (LPARAM)g_weekDays[i]);
    {
        int wd = r ? atoi(r->v[3]) : 0;
        SendMessage(c, CB_SETCURSEL, wd >= 0 && wd < 7 ? wd : 0, 0);
    }
    c = CreateWindowExA(0, "STATIC", "Date (one-off, YYYY-MM-DD):",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 128, 200, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 126, 300, 24, dlg, (HMENU)IDSEF_DATE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Start (HH:MM):",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 158, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[4] : "10:00",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 156, 130, 24, dlg, (HMENU)IDSEF_START, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "End (HH:MM):",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 188, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[5] : "12:00",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 186, 130, 24, dlg, (HMENU)IDSEF_END, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Location:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 218, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[6] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 216, 300, 24, dlg, (HMENU)IDSEF_LOC, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Save",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 244, 256, 90, 30, dlg, (HMENU)IDSEF_OK, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE, 342, 256, 90, 30, dlg, (HMENU)IDSEF_CANCEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

static void fillServicesList(HWND dlg)
{
    HWND lb = GetDlgItem(dlg, IDREC_LIST);
    int i;
    SendMessage(lb, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_rowCount; i++) {
        char day[64] = "";
        char s[768];
        if (g_rows[i].v[2][0] == 't') {
            int wd = atoi(g_rows[i].v[3]);
            snprintf(day, sizeof(day), "every %s", wd >= 0 && wd < 7 ? g_weekDays[wd] : "?");
        } else {
            snprintf(day, sizeof(day), "once");
        }
        snprintf(s, sizeof(s), "%-10s | %s | %s | %s", g_rows[i].v[4], g_rows[i].v[0],
            day, g_rows[i].v[6]);
        SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)s);
    }
    if (g_rowCount > 0)
        SendMessage(lb, LB_SETCURSEL, 0, 0);
}

static LRESULT CALLBACK servicesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDREC_ADD) {
            if (requireEdit(hwnd))
                openServiceEditDialog(hwnd, NULL);
        } else if (id == IDREC_EDIT) {
            int sel = (int)SendMessage(GetDlgItem(hwnd, IDREC_LIST), LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_rowCount && requireEdit(hwnd))
                openServiceEditDialog(hwnd, &g_rows[sel]);
        } else if (id == IDREC_DEL) {
            int sel = (int)SendMessage(GetDlgItem(hwnd, IDREC_LIST), LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_rowCount) {
                char *res;
                if (!requireEdit(hwnd))
                    return 0;
                if (MessageBoxA(hwnd, "Remove this service?",
                        "Confirm", MB_YESNO | MB_ICONQUESTION) != IDYES)
                    return 0;
                res = sbDelete("services", g_rows[sel].id);
                sbMessageOk("Could not delete the service.",
                    res && (g_lastStatus == 200 || g_lastStatus == 204));
                free(res);
                loadServices();
                fillServicesList(hwnd);
            }
        } else if (id == IDREC_CLOSE) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openServicesDialog(HWND parent)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    if (!g_sbLoggedIn) {
        openLoginDialog(parent);
        return;
    }
    loadServices();
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "ServicesDlgClass",
        "Services & Schedules - Domaine Church Presenter",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, 120, 80, 700, 470, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER,
        12, 12, 540, 420, dlg, (HMENU)IDREC_LIST, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Add",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 12, 118, 34, dlg, (HMENU)IDREC_ADD, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Edit",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 52, 118, 34, dlg, (HMENU)IDREC_EDIT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Delete",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 92, 118, 34, dlg, (HMENU)IDREC_DEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 402, 118, 34, dlg, (HMENU)IDREC_CLOSE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    if (!g_sbCanEdit) {
        EnableWindow(GetDlgItem(dlg, IDREC_ADD), FALSE);
        EnableWindow(GetDlgItem(dlg, IDREC_EDIT), FALSE);
        EnableWindow(GetDlgItem(dlg, IDREC_DEL), FALSE);
    }
    fillServicesList(dlg);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

static LRESULT CALLBACK eventEditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDEVF_OK) {
            char tl[256], ct[256], st[64], et[64], loc[256], dt[64], desc[512];
            char esc[256], body[2200], *res;
            GetDlgItemTextA(hwnd, IDEVF_TITLE, tl, sizeof(tl));
            GetDlgItemTextA(hwnd, IDEVF_CAT, ct, sizeof(ct));
            GetDlgItemTextA(hwnd, IDEVF_DATE, dt, sizeof(dt));
            GetDlgItemTextA(hwnd, IDEVF_START, st, sizeof(st));
            GetDlgItemTextA(hwnd, IDEVF_END, et, sizeof(et));
            GetDlgItemTextA(hwnd, IDEVF_LOC, loc, sizeof(loc));
            GetDlgItemTextA(hwnd, IDEVF_DESC, desc, sizeof(desc));
            if (!tl[0] || !dt[0])
                return 0;
            jsonEscape(esc, sizeof(esc), tl);
            snprintf(body, sizeof(body), "{\"title\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), ct);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"category\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), dt);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"date\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), st);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"start_time\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), et);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"end_time\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), loc);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"location\":\"%s\"", esc);
            jsonEscape(esc, sizeof(esc), desc);
            snprintf(body + strlen(body), sizeof(body) - strlen(body), ",\"description\":\"%s\"}", esc);
            if (GetWindowLongPtr(hwnd, GWLP_USERDATA))
                res = sbUpdate("events", (const char *)GetWindowLongPtr(hwnd, GWLP_USERDATA), body);
            else
                res = sbInsert("events", body);
            sbMessageOk("Could not save the event.",
                res && (g_lastStatus == 200 || g_lastStatus == 201));
            free(res);
            DestroyWindow(hwnd);
        } else if (id == IDEVF_CANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openEventEditDialog(HWND parent, const RecRow *r)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "EventEditDlgClass",
        r ? "Edit Event" : "Add Event",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, 240, 100, 460, 420, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    if (r)
        SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)r->id);
    c = CreateWindowExA(0, "STATIC", "Title:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 12, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[0] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 10, 300, 24, dlg, (HMENU)IDEVF_TITLE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Category:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 42, 110, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[1] : "Church Event",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 40, 300, 24, dlg, (HMENU)IDEVF_CAT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Date (YYYY-MM-DD):",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 72, 130, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[2] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 70, 300, 24, dlg, (HMENU)IDEVF_DATE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Start (HH:MM):",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 102, 130, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[3] : "10:00",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 100, 130, 24, dlg, (HMENU)IDEVF_START, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "End (HH:MM):",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 132, 130, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[4] : "12:00",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 130, 130, 24, dlg, (HMENU)IDEVF_END, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Location:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 162, 130, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[5] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 160, 300, 24, dlg, (HMENU)IDEVF_LOC, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "STATIC", "Description:",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 14, 192, 130, 18, dlg, (HMENU)0, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", r ? r->v[6] : "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN,
        130, 190, 300, 80, dlg, (HMENU)IDEVF_DESC, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Save",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 244, 286, 90, 30, dlg, (HMENU)IDEVF_OK, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE, 342, 286, 90, 30, dlg, (HMENU)IDEVF_CANCEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

static void fillEventsList(HWND dlg)
{
    HWND lb = GetDlgItem(dlg, IDREC_LIST);
    int i;
    SendMessage(lb, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_rowCount; i++) {
        char s[768];
        snprintf(s, sizeof(s), "%s | %s | %s | %s", g_rows[i].v[2],
            g_rows[i].v[3], g_rows[i].v[0], g_rows[i].v[5]);
        SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)s);
    }
    if (g_rowCount > 0)
        SendMessage(lb, LB_SETCURSEL, 0, 0);
}

static LRESULT CALLBACK eventsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDREC_ADD) {
            if (requireEdit(hwnd))
                openEventEditDialog(hwnd, NULL);
        } else if (id == IDREC_EDIT) {
            int sel = (int)SendMessage(GetDlgItem(hwnd, IDREC_LIST), LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_rowCount && requireEdit(hwnd))
                openEventEditDialog(hwnd, &g_rows[sel]);
        } else if (id == IDREC_DEL) {
            int sel = (int)SendMessage(GetDlgItem(hwnd, IDREC_LIST), LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_rowCount) {
                char *res;
                if (!requireEdit(hwnd))
                    return 0;
                if (MessageBoxA(hwnd, "Remove this event?",
                        "Confirm", MB_YESNO | MB_ICONQUESTION) != IDYES)
                    return 0;
                res = sbDelete("events", g_rows[sel].id);
                sbMessageOk("Could not delete the event.",
                    res && (g_lastStatus == 200 || g_lastStatus == 204));
                free(res);
                loadEvents();
                fillEventsList(hwnd);
            }
        } else if (id == IDREC_CLOSE) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void openEventsDialog(HWND parent)
{
    HWND dlg;
    HINSTANCE inst = GetModuleHandle(NULL);
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c;
    if (!g_sbLoggedIn) {
        openLoginDialog(parent);
        return;
    }
    loadEvents();
    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "EventsDlgClass",
        "Events & Activities - Domaine Church Presenter",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, 120, 80, 700, 470, parent, NULL, inst, NULL);
    if (!dlg)
        return;
    c = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER,
        12, 12, 540, 420, dlg, (HMENU)IDREC_LIST, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Add",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 12, 118, 34, dlg, (HMENU)IDREC_ADD, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Edit",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 52, 118, 34, dlg, (HMENU)IDREC_EDIT, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Delete",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 92, 118, 34, dlg, (HMENU)IDREC_DEL, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    c = CreateWindowExA(0, "BUTTON", "Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 564, 402, 118, 34, dlg, (HMENU)IDREC_CLOSE, inst, NULL);
    SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
    if (!g_sbCanEdit) {
        EnableWindow(GetDlgItem(dlg, IDREC_ADD), FALSE);
        EnableWindow(GetDlgItem(dlg, IDREC_EDIT), FALSE);
        EnableWindow(GetDlgItem(dlg, IDREC_DEL), FALSE);
    }
    fillEventsList(dlg);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
}

static void updateChurchMenu(void)
{
    static HMENU church = NULL;
    char cap[180];
    if (!g_menuBar)
        return;
    if (church) {
        int i, n = GetMenuItemCount(g_menuBar);
        for (i = 0; i < n; i++) {
            char nm[64];
            GetMenuStringA(g_menuBar, i, nm, sizeof(nm), MF_BYPOSITION);
            if (strcmp(nm, "Church") == 0) {
                RemoveMenu(g_menuBar, i, MF_BYPOSITION);
                break;
            }
        }
        DestroyMenu(church);
        church = NULL;
    }
    church = CreatePopupMenu();
    if (!g_sbLoggedIn) {
        AppendMenuA(church, MF_STRING, IDM_CHURCH_LOGIN, "Log in...");
    } else {
        snprintf(cap, sizeof(cap), "Log out (%s%s%s)",
            g_sbFullName[0] ? g_sbFullName : g_sbUserEmail,
            g_sbUserRole[0] ? " - " : "", g_sbUserRole);
        AppendMenuA(church, MF_STRING, IDM_CHURCH_LOGOUT, cap);
        AppendMenuA(church, MF_SEPARATOR, 0, NULL);
        AppendMenuA(church, MF_STRING, IDM_CHURCH_MEMBERS, "Members Directory...");
        AppendMenuA(church, MF_STRING, IDM_CHURCH_SERVICES, "Services & Schedules...");
        AppendMenuA(church, MF_STRING, IDM_CHURCH_EVENTS, "Events & Activities...");
        AppendMenuA(church, MF_SEPARATOR, 0, NULL);
        AppendMenuA(church, MF_STRING, IDM_CHURCH_TODAY, "Show Today's Schedule");
        CheckMenuItem(church, IDM_CHURCH_TODAY, MF_BYCOMMAND
            | (g_showToday ? MF_CHECKED : MF_UNCHECKED));
    }
    AppendMenuA(g_menuBar, MF_POPUP, (UINT_PTR)church, "Church");
    if (g_main)
        DrawMenuBar(g_main);
}
