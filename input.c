/*
 * 文件名称：input.c
 * 功能说明：统一处理键盘输入，完成空值、长度、数值范围、日期格式等校验。
 *           当标准输入被重定向时自动回显输入值，便于生成测试记录。
 */

#include "input.h"
#include "common.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#endif

static int g_echo_input = 0;

void input_set_echo(int enabled)
{
    g_echo_input = enabled;
}

static int stdin_is_interactive(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdin));
#else
    return isatty(fileno(stdin));
#endif
}

/* 读取一行；返回 0 表示输入结束（EOF），调用方应安全退出。 */
static int read_one_line(const char *prompt, char *out, size_t out_size)
{
    size_t length;

    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    if (fgets(out, (int)out_size, stdin) == NULL) {
        putchar('\n');
        return 0;
    }
    length = strlen(out);
    if (length > 0 && out[length - 1] == '\n') {
        out[length - 1] = '\0';
    } else {
        int ch;
        /* 丢弃超长行的剩余字符，避免污染下一次输入 */
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* empty */
        }
    }
    trim_text(out);
    if (g_echo_input) {
        printf("%s\n", out);
    }
    return 1;
}

int input_text(const char *prompt, char *out, size_t out_size, int allow_empty)
{
    for (;;) {
        if (!read_one_line(prompt, out, out_size)) {
            return 0;
        }
        if (text_is_blank(out)) {
            if (allow_empty) {
                return 1;
            }
            print_error("输入不能为空，请重新输入。");
            continue;
        }
        if (text_has_forbidden_char(out)) {
            print_error("输入中不能包含竖线“|”、制表符或换行符。");
            continue;
        }
        return 1;
    }
}

int input_text_default(const char *prompt, const char *current, char *out, size_t out_size)
{
    char line[LINE_LEN];
    char actual_prompt[LINE_LEN];

    snprintf(actual_prompt, sizeof(actual_prompt), "%s [直接回车保留：%s]：", prompt, current);
    for (;;) {
        if (!read_one_line(actual_prompt, line, sizeof(line))) {
            return 0;
        }
        if (text_is_blank(line)) {
            copy_text(out, out_size, current);
            return 1;
        }
        if (text_has_forbidden_char(line)) {
            print_error("输入中不能包含竖线“|”、制表符或换行符。");
            continue;
        }
        copy_text(out, out_size, line);
        return 1;
    }
}

int input_int_range(const char *prompt, int min_value, int max_value, int *out)
{
    char line[64];
    int value;

    for (;;) {
        if (!input_text(prompt, line, sizeof(line), 0)) {
            return 0;
        }
        if (!parse_int_strict(line, &value)) {
            print_error("“%s”不是合法整数，请重新输入。", line);
            continue;
        }
        if (value < min_value || value > max_value) {
            print_error("数值必须在 %d 到 %d 之间。", min_value, max_value);
            continue;
        }
        *out = value;
        return 1;
    }
}

int input_long_range(const char *prompt, long min_value, long max_value, long *out)
{
    char line[64];
    long value;

    for (;;) {
        if (!input_text(prompt, line, sizeof(line), 0)) {
            return 0;
        }
        if (!parse_long_strict(line, &value)) {
            print_error("“%s”不是合法整数，请重新输入。", line);
            continue;
        }
        if (value < min_value || value > max_value) {
            print_error("数值必须在 %ld 到 %ld 之间。", min_value, max_value);
            continue;
        }
        *out = value;
        return 1;
    }
}

int input_double_range(const char *prompt, double min_value, double max_value, double *out)
{
    char line[64];
    double value;

    for (;;) {
        if (!input_text(prompt, line, sizeof(line), 0)) {
            return 0;
        }
        if (!parse_double_strict(line, &value)) {
            print_error("“%s”不是合法数值，请重新输入。", line);
            continue;
        }
        if (value < min_value || value > max_value) {
            print_error("数值必须在 %.2f 到 %.2f 之间。", min_value, max_value);
            continue;
        }
        *out = value;
        return 1;
    }
}

int input_date_value(const char *prompt, int allow_empty, int default_today, char *out, size_t out_size)
{
    char line[32];
    char today[DATE_LEN];

    date_today(today, sizeof(today));
    for (;;) {
        if (!input_text(prompt, line, sizeof(line), allow_empty)) {
            return 0;
        }
        if (*line == '\0') {
            if (default_today) {
                copy_text(out, out_size, today);
                return 1;
            }
            out[0] = '\0';
            return 1;
        }
        if (!date_is_valid(line)) {
            print_error("日期格式错误，应为 YYYY-MM-DD，且日期必须真实存在。");
            continue;
        }
        copy_text(out, out_size, line);
        return 1;
    }
}

int input_month_value(const char *prompt, int default_current, char *out, size_t out_size)
{
    char line[32];
    char current[MONTH_LEN];

    month_today(current, sizeof(current));
    for (;;) {
        if (!input_text(prompt, line, sizeof(line), default_current)) {
            return 0;
        }
        if (*line == '\0') {
            copy_text(out, out_size, current);
            return 1;
        }
        if (!month_is_valid(line)) {
            print_error("月份格式错误，应为 YYYY-MM，例如 2025-10。");
            continue;
        }
        copy_text(out, out_size, line);
        return 1;
    }
}

static int equals_ascii_ignore_case(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
            return 0;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

int input_confirm(const char *prompt, int default_yes)
{
    char line[16];

    for (;;) {
        if (!input_text(prompt, line, sizeof(line), 1)) {
            return 0;
        }
        if (*line == '\0') {
            return default_yes;
        }
        if (equals_ascii_ignore_case(line, "y") ||
            equals_ascii_ignore_case(line, "yes") ||
            strcmp(line, "是") == 0 || strcmp(line, "确认") == 0) {
            return 1;
        }
        if (equals_ascii_ignore_case(line, "n") ||
            equals_ascii_ignore_case(line, "no") ||
            strcmp(line, "否") == 0 || strcmp(line, "取消") == 0) {
            return 0;
        }
        print_error("请输入 y 或 n（也可输入“是/否”）。");
    }
}

int input_password(const char *prompt, char *out, size_t out_size)
{
    size_t length = 0;
    int ch;

    fputs(prompt, stdout);
    fflush(stdout);
    if (!stdin_is_interactive()) {
        if (fgets(out, (int)out_size, stdin) == NULL) {
            putchar('\n');
            return 0;
        }
        length = strlen(out);
        if (length > 0 && out[length - 1] == '\n') {
            out[length - 1] = '\0';
        }
        trim_text(out);
        printf("******\n");
        return 1;
    }

#ifdef _WIN32
    for (;;) {
        ch = _getch();
        if (ch == '\r' || ch == '\n') {
            break;
        }
        if (ch == 8) {
            if (length > 0) {
                --length;
                putchar('\b');
                putchar(' ');
                putchar('\b');
                fflush(stdout);
            }
            continue;
        }
        if (ch == 3) {
            putchar('\n');
            return 0;
        }
        if (length + 1 < out_size) {
            out[length++] = (char)ch;
            putchar('*');
            fflush(stdout);
        }
    }
    out[length] = '\0';
#else
    if (fgets(out, (int)out_size, stdin) == NULL) {
        putchar('\n');
        return 0;
    }
    length = strlen(out);
    if (length > 0 && out[length - 1] == '\n') {
        out[length - 1] = '\0';
    }
    trim_text(out);
#endif
    putchar('\n');
    return 1;
}
