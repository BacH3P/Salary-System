/*
 * 文件名称：common.c
 * 功能说明：公共工具函数实现，包括字符串安全处理、UTF-8 表格对齐、
 *           日期校验、控制台初始化、目录与路径处理等。
 */

#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

double round_money(double value)
{
    if (value >= 0.0) {
        return floor(value * 100.0 + 0.5) / 100.0;
    }
    return ceil(value * 100.0 - 0.5) / 100.0;
}

int parse_int_strict(const char *text, int *value)
{
    char *end = NULL;
    long number;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    number = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    if (number < -2147483647L - 1L || number > 2147483647L) {
        return 0;
    }
    *value = (int)number;
    return 1;
}

int parse_long_strict(const char *text, long *value)
{
    char *end = NULL;
    long number;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    number = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    *value = number;
    return 1;
}

int parse_double_strict(const char *text, double *value)
{
    char *end = NULL;
    double number;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    number = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    if (!isfinite(number)) {
        return 0;
    }
    *value = number;
    return 1;
}

void trim_text(char *text)
{
    char *start;
    char *end;

    if (text == NULL) {
        return;
    }
    start = text;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        ++start;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        --end;
    }
    *end = '\0';
}

int text_is_blank(const char *text)
{
    if (text == NULL) {
        return 1;
    }
    while (*text != '\0') {
        if (!isspace((unsigned char)*text)) {
            return 0;
        }
        ++text;
    }
    return 1;
}

int text_has_forbidden_char(const char *text)
{
    if (text == NULL) {
        return 1;
    }
    while (*text != '\0') {
        if (*text == '|' || *text == '\t' || *text == '\r' || *text == '\n') {
            return 1;
        }
        ++text;
    }
    return 0;
}

void copy_text(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

int text_contains(const char *haystack, const char *needle)
{
    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    if (*needle == '\0') {
        return 1;
    }
    return strstr(haystack, needle) != NULL;
}

static int utf8_char_bytes(unsigned char lead)
{
    if (lead < 0x80) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return 1;
}

static int utf8_char_width(const char *text, int bytes)
{
    unsigned char lead = (unsigned char)text[0];

    if (bytes == 1) {
        return 1;
    }
    if (bytes >= 3) {
        (void)lead;
        return 2; /* 中文、全角标点及常见符号按两个字符宽度显示 */
    }
    return 1;
}

int utf8_display_width(const char *text)
{
    int width = 0;
    int bytes;
    const char *p = text;

    if (text == NULL) {
        return 0;
    }
    while (*p != '\0') {
        bytes = utf8_char_bytes((unsigned char)*p);
        if (bytes > 1 && (size_t)bytes > strlen(p)) {
            bytes = 1;
        }
        width += utf8_char_width(p, bytes);
        p += bytes;
    }
    return width;
}

void print_padded(const char *text, int width, int align_right)
{
    int used;
    int remain;
    int bytes;
    const char *p;

    if (text == NULL) {
        text = "";
    }
    used = utf8_display_width(text);
    if (used > width) {
        /* 按显示宽度截断，保证表格列不被长文本撑开 */
        int printed = 0;
        p = text;
        while (*p != '\0') {
            bytes = utf8_char_bytes((unsigned char)*p);
            if (bytes > 1 && (size_t)bytes > strlen(p)) {
                bytes = 1;
            }
            if (printed + utf8_char_width(p, bytes) > width) {
                break;
            }
            fwrite(p, 1, (size_t)bytes, stdout);
            printed += utf8_char_width(p, bytes);
            p += bytes;
        }
        return;
    }
    remain = width - used;
    if (!align_right) {
        fputs(text, stdout);
        print_repeat(' ', remain);
    } else {
        print_repeat(' ', remain);
        fputs(text, stdout);
    }
}

void print_repeat(char ch, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        putchar(ch);
    }
}

void print_rule(int width)
{
    print_repeat('-', width);
    putchar('\n');
}

static void vprint_prefixed(const char *prefix, const char *fmt, va_list args)
{
    fputs(prefix, stdout);
    vprintf(fmt, args);
    putchar('\n');
    fflush(stdout);
}

void print_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprint_prefixed("[错误] ", fmt, args);
    va_end(args);
}

void print_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprint_prefixed("[提示] ", fmt, args);
    va_end(args);
}

void print_success(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprint_prefixed("[成功] ", fmt, args);
    va_end(args);
}

static int is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int date_is_valid(const char *date)
{
    int year;
    int month;
    int day;
    int days_in_month;
    static const int month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (date == NULL || strlen(date) != 10 || date[4] != '-' || date[7] != '-') {
        return 0;
    }
    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3) {
        return 0;
    }
    if (year < 1900 || year > 2200 || month < 1 || month > 12 || day < 1) {
        return 0;
    }
    days_in_month = month_days[month - 1];
    if (month == 2 && is_leap_year(year)) {
        days_in_month = 29;
    }
    return day <= days_in_month;
}

int month_is_valid(const char *month)
{
    int year;
    int mon;

    if (month == NULL || strlen(month) != 7 || month[4] != '-') {
        return 0;
    }
    if (sscanf(month, "%4d-%2d", &year, &mon) != 2) {
        return 0;
    }
    return year >= 1900 && year <= 2200 && mon >= 1 && mon <= 12;
}

int date_compare(const char *left, const char *right)
{
    return strcmp(left, right);
}

void date_today(char *out, size_t out_size)
{
    time_t now = time(NULL);
    struct tm local_value;
    struct tm *local_ptr;

#ifdef _MSC_VER
    localtime_s(&local_value, &now);
    local_ptr = &local_value;
#else
    local_ptr = localtime(&now);
    if (local_ptr != NULL) {
        local_value = *local_ptr;
        local_ptr = &local_value;
    }
#endif
    if (local_ptr == NULL) {
        copy_text(out, out_size, "1970-01-01");
        return;
    }
    strftime(out, out_size, "%Y-%m-%d", local_ptr);
}

void month_today(char *out, size_t out_size)
{
    time_t now = time(NULL);
    struct tm local_value;
    struct tm *local_ptr;

#ifdef _MSC_VER
    localtime_s(&local_value, &now);
    local_ptr = &local_value;
#else
    local_ptr = localtime(&now);
    if (local_ptr != NULL) {
        local_value = *local_ptr;
        local_ptr = &local_value;
    }
#endif
    if (local_ptr == NULL) {
        copy_text(out, out_size, "1970-01");
        return;
    }
    strftime(out, out_size, "%Y-%m", local_ptr);
}

void date_minus_years(const char *date, int years, char *out, size_t out_size)
{
    struct tm value;
    int year;
    int month;
    int day;

    if (!date_is_valid(date)) {
        copy_text(out, out_size, "1970-01-01");
        return;
    }
    memset(&value, 0, sizeof(value));
    sscanf(date, "%4d-%2d-%2d", &year, &month, &day);
    value.tm_year = year - years - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = 12;
    value.tm_isdst = -1;
    if (mktime(&value) == (time_t)-1) {
        copy_text(out, out_size, "1970-01-01");
        return;
    }
    strftime(out, out_size, "%Y-%m-%d", &value);
}

int date_is_before_years(const char *date, int years)
{
    char today[DATE_LEN];
    char cutoff[DATE_LEN];

    if (!date_is_valid(date)) {
        return 0;
    }
    date_today(today, sizeof(today));
    date_minus_years(today, years, cutoff, sizeof(cutoff));
    return date_compare(date, cutoff) < 0;
}

int console_is_interactive(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdin)) && isatty(fileno(stdout));
#endif
}

void console_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    SetConsoleTitleA("大学生勤工助学薪资发放管理系统");
#endif
}

#ifdef _WIN32
static wchar_t *utf8_to_wide(const char *text)
{
    int length;
    wchar_t *wide;

    if (text == NULL) {
        return NULL;
    }
    length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    if (length <= 0) {
        length = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    }
    if (length <= 0) {
        return NULL;
    }
    wide = (wchar_t *)malloc((size_t)length * sizeof(wchar_t));
    if (wide == NULL) {
        return NULL;
    }
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, wide, length) <= 0) {
        MultiByteToWideChar(CP_ACP, 0, text, -1, wide, length);
    }
    return wide;
}

static int wide_to_utf8(const wchar_t *wide, char *out, size_t out_size)
{
    int length;

    if (wide == NULL || out == NULL || out_size == 0) {
        return 0;
    }
    length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (length <= 0 || (size_t)length > out_size) {
        return 0;
    }
    return WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)out_size, NULL, NULL) > 0;
}
#endif

int get_executable_dir(char *out, size_t out_size)
{
#ifdef _WIN32
    wchar_t buffer[PATH_LEN];
    DWORD length;
    wchar_t *slash;
    char utf8_buffer[PATH_LEN];

    length = GetModuleFileNameW(NULL, buffer, (DWORD)(sizeof(buffer) / sizeof(buffer[0])));
    if (length == 0 || length >= (DWORD)(sizeof(buffer) / sizeof(buffer[0]))) {
        copy_text(out, out_size, ".");
        return 0;
    }
    slash = wcsrchr(buffer, L'\\');
    if (slash == NULL) {
        slash = wcsrchr(buffer, L'/');
    }
    if (slash != NULL) {
        *slash = L'\0';
    } else {
        wcscpy(buffer, L".");
    }
    if (!wide_to_utf8(buffer, utf8_buffer, sizeof(utf8_buffer))) {
        copy_text(out, out_size, ".");
        return 0;
    }
    copy_text(out, out_size, utf8_buffer);
    return 1;
#else
    char buffer[PATH_LEN];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length <= 0) {
        copy_text(out, out_size, ".");
        return 0;
    }
    buffer[length] = '\0';
    {
        char *slash = strrchr(buffer, '/');
        if (slash != NULL) {
            *slash = '\0';
        }
    }
    copy_text(out, out_size, buffer);
    return 1;
#endif
}

int ensure_directory(const char *path)
{
#ifdef _WIN32
    wchar_t *work;
    wchar_t *p;
    size_t length;

    if (path == NULL || *path == '\0') {
        return 0;
    }
    work = utf8_to_wide(path);
    if (work == NULL) {
        return 0;
    }
    length = wcslen(work);
    while (length > 1 && (work[length - 1] == L'\\' || work[length - 1] == L'/')) {
        work[--length] = L'\0';
    }
    for (p = work + 1; *p != L'\0'; ++p) {
        if (*p == L'\\' || *p == L'/') {
            wchar_t saved = *p;
            *p = L'\0';
            if (wcslen(work) > 0 && work[wcslen(work) - 1] != L':') {
                if (_wmkdir(work) != 0 && errno != EEXIST) {
                    *p = saved;
                    free(work);
                    return 0;
                }
            }
            *p = saved;
        }
    }
    if (_wmkdir(work) != 0 && errno != EEXIST) {
        free(work);
        return 0;
    }
    free(work);
    return 1;
#else
    char work[PATH_LEN];
    char *p;
    size_t length;

    if (path == NULL || *path == '\0') {
        return 0;
    }
    copy_text(work, sizeof(work), path);
    length = strlen(work);
    while (length > 1 && (work[length - 1] == '\\' || work[length - 1] == '/')) {
        work[--length] = '\0';
    }
    for (p = work + 1; *p != '\0'; ++p) {
        if (*p == '\\' || *p == '/') {
            char saved = *p;
            *p = '\0';
            if (strlen(work) > 0) {
                if (mkdir(work, 0755) != 0 && errno != EEXIST) {
                    *p = saved;
                    return 0;
                }
            }
            *p = saved;
        }
    }
    if (mkdir(work, 0755) != 0 && errno != EEXIST) {
        return 0;
    }
    return 1;
#endif
}

FILE *file_open_utf8(const char *path, const char *mode)
{
#ifdef _WIN32
    wchar_t *wide_path;
    wchar_t wide_mode[16];
    FILE *file;

    wide_path = utf8_to_wide(path);
    if (wide_path == NULL) {
        return NULL;
    }
    if (mbstowcs(wide_mode, mode, sizeof(wide_mode) / sizeof(wide_mode[0]) - 1) == (size_t)-1) {
        free(wide_path);
        return NULL;
    }
    wide_mode[sizeof(wide_mode) / sizeof(wide_mode[0]) - 1] = L'\0';
    file = _wfopen(wide_path, wide_mode);
    free(wide_path);
    return file;
#else
    return fopen(path, mode);
#endif
}

int file_remove_utf8(const char *path)
{
#ifdef _WIN32
    wchar_t *wide_path;
    int result;

    wide_path = utf8_to_wide(path);
    if (wide_path == NULL) {
        return -1;
    }
    result = _wremove(wide_path);
    free(wide_path);
    return result;
#else
    return remove(path);
#endif
}

int file_rename_utf8(const char *old_path, const char *new_path)
{
#ifdef _WIN32
    wchar_t *wide_old;
    wchar_t *wide_new;
    int result;

    wide_old = utf8_to_wide(old_path);
    wide_new = utf8_to_wide(new_path);
    if (wide_old == NULL || wide_new == NULL) {
        free(wide_old);
        free(wide_new);
        return -1;
    }
    result = _wrename(wide_old, wide_new);
    free(wide_old);
    free(wide_new);
    return result;
#else
    return rename(old_path, new_path);
#endif
}

int file_replace_utf8(const char *source_path, const char *target_path)
{
#ifdef _WIN32
    wchar_t *wide_source;
    wchar_t *wide_target;
    BOOL result;

    wide_source = utf8_to_wide(source_path);
    wide_target = utf8_to_wide(target_path);
    if (wide_source == NULL || wide_target == NULL) {
        free(wide_source);
        free(wide_target);
        return -1;
    }
    result = MoveFileExW(wide_source, wide_target,
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    free(wide_source);
    free(wide_target);
    return result ? 0 : -1;
#else
    return rename(source_path, target_path);
#endif
}

void path_join(char *out, size_t out_size, const char *dir, const char *name)
{
    size_t length;

    if (out == NULL || out_size == 0) {
        return;
    }
    if (dir == NULL || *dir == '\0') {
        copy_text(out, out_size, name == NULL ? "" : name);
        return;
    }
    copy_text(out, out_size, dir);
    length = strlen(out);
    if (length > 0 && out[length - 1] != '\\' && out[length - 1] != '/') {
        if (length + 1 < out_size) {
            out[length] = '\\';
            out[length + 1] = '\0';
        }
    }
    if (name != NULL) {
        strncat(out, name, out_size - strlen(out) - 1);
    }
}

void app_clear_screen(const AppData *app)
{
    if (app != NULL && app->no_clear) {
        return;
    }
    if (!console_is_interactive()) {
        return;
    }
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void app_pause(const AppData *app)
{
    int ch;

    if (app != NULL && app->no_clear) {
        return;
    }
    if (!console_is_interactive()) {
        return;
    }
    printf("\n按 Enter 键继续...");
    fflush(stdout);
    do {
        ch = getchar();
    } while (ch != '\n' && ch != EOF);
}

void app_print_banner(void)
{
    print_repeat('=', 78);
    putchar('\n');
    printf("  %s  v%s\n", APP_NAME, APP_VERSION);
    printf("  数据结构：结构体 + 动态单链表    存储方式：文本文件持久化\n");
    print_repeat('=', 78);
    putchar('\n');
}

void app_print_section(const char *title)
{
    putchar('\n');
    print_repeat('=', 78);
    putchar('\n');
    printf("  %s\n", title);
    print_repeat('=', 78);
    putchar('\n');
}

const char *pay_status_text(int status)
{
    return status == PAY_PAID ? "已发放" : "未发放";
}
