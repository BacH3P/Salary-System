/*
 * 文件名称：common.h
 * 功能说明：公共数据类型、常量及通用工具函数声明。
 *           本文件定义岗位、薪资记录两个核心结构体，供各功能模块共享。
 * 编译标准：C99（源码同时兼容 MSVC 的 C11 编译模式）
 */

#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>

#define APP_NAME        "大学生勤工助学薪资发放管理系统"
#define APP_VERSION     "1.0.0"
#define APP_AUTHOR      "计算机科学与技术专业课程设计"

#define JOB_ID_LEN      16
#define JOB_NAME_LEN    64
#define DEPT_NAME_LEN   64
#define STUDENT_ID_LEN  32
#define STUDENT_NAME_LEN 64
#define DATE_LEN        11
#define MONTH_LEN       8
#define REASON_LEN      256
#define PATH_LEN        512
#define LINE_LEN        1024

#define MAX_MONTH_HOURS 744.0   /* 31 天 × 24 小时 */
#define MAX_HOURLY_WAGE 1000.0

/* 发放状态：界面显示为“已发放/未发放”，文件中保存为 1/0 */
typedef enum {
    PAY_UNPAID = 0,
    PAY_PAID   = 1
} PayStatus;

/* 岗位信息结点 */
typedef struct Job {
    char id[JOB_ID_LEN];             /* 岗位编号，如 J001 */
    char name[JOB_NAME_LEN];         /* 岗位名称 */
    char department[DEPT_NAME_LEN];  /* 所属部门 */
    double hourly_wage;              /* 时薪（元/小时） */
    double monthly_hour_cap;         /* 每月工作时长上限（小时） */
    int recruit_count;               /* 招聘人数 */
    struct Job *next;                /* 动态链表后继指针 */
} Job;

/* 薪资记录结点 */
typedef struct Salary {
    long id;                          /* 记录编号，便于修改、删除 */
    char student_id[STUDENT_ID_LEN];  /* 学号 */
    char student_name[STUDENT_NAME_LEN]; /* 姓名 */
    char job_id[JOB_ID_LEN];          /* 岗位编号 */
    char salary_month[MONTH_LEN];     /* 薪资所属月份 YYYY-MM */
    double work_hours;                /* 当月工作时长 */
    double gross_pay;                 /* 应发薪资 */
    double deduction;                 /* 扣除金额 */
    char deduction_reason[REASON_LEN];/* 扣除原因说明 */
    double net_pay;                   /* 实发薪资 */
    char pay_date[DATE_LEN];          /* 发放时间，未发放时为空串 */
    int status;                       /* PayStatus：0 未发放，1 已发放 */
    char created_date[DATE_LEN];      /* 记录创建日期，用于超期清理 */
    struct Salary *next;              /* 动态链表后继指针 */
} Salary;

/* 系统运行数据：两条动态链表 + 文件路径 + 统计信息 */
typedef struct AppData {
    Job *job_head;
    Salary *salary_head;
    char data_dir[PATH_LEN];
    char job_file[PATH_LEN];
    char salary_file[PATH_LEN];
    char backup_file[PATH_LEN];
    char backup_all_file[PATH_LEN];
    int job_count;
    int salary_count;
    long next_salary_id;
    int dirty;
    int no_clear;
    int echo_input;
} AppData;

/* ---------- 通用数值、字符串与日期工具 ---------- */
double round_money(double value);
int parse_int_strict(const char *text, int *value);
int parse_long_strict(const char *text, long *value);
int parse_double_strict(const char *text, double *value);
void trim_text(char *text);
int text_is_blank(const char *text);
int text_has_forbidden_char(const char *text); /* 禁止 '|'、换行、制表符 */
void copy_text(char *dst, size_t dst_size, const char *src);
int text_contains(const char *haystack, const char *needle);

int utf8_display_width(const char *text);
void print_padded(const char *text, int width, int align_right);
void print_repeat(char ch, int count);
void print_rule(int width);
void print_error(const char *fmt, ...);
void print_info(const char *fmt, ...);
void print_success(const char *fmt, ...);

int date_is_valid(const char *date);
int month_is_valid(const char *month);
int date_compare(const char *left, const char *right);
void date_today(char *out, size_t out_size);
void month_today(char *out, size_t out_size);
void date_minus_years(const char *date, int years, char *out, size_t out_size);
int date_is_before_years(const char *date, int years);

/* ---------- 控制台与路径工具 ---------- */
int console_is_interactive(void);
void console_init(void);
int get_executable_dir(char *out, size_t out_size);
int ensure_directory(const char *path);
void path_join(char *out, size_t out_size, const char *dir, const char *name);

/* 以 UTF-8 路径打开/删除/重命名文件（Windows 下内部转换为 UTF-16） */
FILE *file_open_utf8(const char *path, const char *mode);
int file_remove_utf8(const char *path);
int file_rename_utf8(const char *old_path, const char *new_path);
int file_replace_utf8(const char *source_path, const char *target_path);
void app_clear_screen(const AppData *app);
void app_pause(const AppData *app);
void app_print_banner(void);
void app_print_section(const char *title);

const char *pay_status_text(int status);

#endif /* COMMON_H */
