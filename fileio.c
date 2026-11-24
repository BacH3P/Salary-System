/*
 * 文件名称：fileio.c
 * 功能说明：实现动态链表与文本文件之间的相互转换、全量备份、
 *           超过两年薪资记录的归档清理，以及数据目录初始化。
 * 文件格式：UTF-8 文本，字段之间以竖线“|”分隔，一行一条记录。
 */

#include "fileio.h"
#include "job.h"
#include "salary.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FIELDS 20

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) {
        copy_text(error, error_size, message);
    }
}

static void set_errno_error(char *error, size_t error_size, const char *action,
                            const char *path)
{
    char message[PATH_LEN + 256];
    snprintf(message, sizeof(message), "%s失败：%s（%s）", action, path,
             strerror(errno));
    set_error(error, error_size, message);
}

static void warn_line(int line_number, const char *message)
{
    printf("[警告] 第 %d 行：%s，已跳过该记录。\n", line_number, message);
}

static int split_fields(char *line, char *fields[], int max_fields)
{
    int count = 0;
    char *p = line;

    fields[count++] = p;
    while (*p != '\0') {
        if (*p == '|') {
            *p = '\0';
            if (count >= max_fields) {
                return -1;
            }
            fields[count++] = p + 1;
        }
        ++p;
    }
    return count;
}

static void strip_bom(char *line)
{
    unsigned char *p = (unsigned char *)line;
    if (p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
        memmove(line, line + 3, strlen(line + 3) + 1);
    }
}

int fileio_file_exists(const char *path)
{
    FILE *file = file_open_utf8(path, "rb");
    if (file == NULL) {
        return 0;
    }
    fclose(file);
    return 1;
}

int fileio_init(AppData *app, const char *data_dir, char *error, size_t error_size)
{
    char executable_dir[PATH_LEN];

    if (app == NULL) {
        set_error(error, error_size, "系统数据对象为空。");
        return 0;
    }
    memset(app, 0, sizeof(*app));
    app->next_salary_id = 1;
    if (data_dir == NULL || *data_dir == '\0') {
        if (!get_executable_dir(executable_dir, sizeof(executable_dir))) {
            copy_text(executable_dir, sizeof(executable_dir), ".");
        }
        path_join(app->data_dir, sizeof(app->data_dir), executable_dir, "data");
    } else {
        copy_text(app->data_dir, sizeof(app->data_dir), data_dir);
    }
    if (!ensure_directory(app->data_dir)) {
        char message[PATH_LEN + 128];
        snprintf(message, sizeof(message), "无法创建数据目录：%s（%s）",
                 app->data_dir, strerror(errno));
        set_error(error, error_size, message);
        return 0;
    }
    path_join(app->job_file, sizeof(app->job_file), app->data_dir, "jobs.txt");
    path_join(app->salary_file, sizeof(app->salary_file), app->data_dir, "salary.txt");
    path_join(app->backup_file, sizeof(app->backup_file), app->data_dir,
              "salary_backup.txt");
    path_join(app->backup_all_file, sizeof(app->backup_all_file), app->data_dir,
              "salary_all.txt");
    return 1;
}

static int load_jobs(AppData *app, char *error, size_t error_size)
{
    FILE *file;
    char line[LINE_LEN];
    int line_number = 0;
    int loaded = 0;
    int skipped = 0;

    file = file_open_utf8(app->job_file, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            print_info("未找到岗位数据文件，将以空岗位链表启动：%s", app->job_file);
            return 1;
        }
        set_errno_error(error, error_size, "打开岗位数据文件", app->job_file);
        return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[MAX_FIELDS];
        int count;
        char validate_error[256];
        Job *node;
        double wage;
        double cap;
        int recruit;

        ++line_number;
        if (line_number == 1) {
            strip_bom(line);
        }
        trim_text(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        count = split_fields(line, fields, MAX_FIELDS);
        if (count != 6) {
            warn_line(line_number, "字段数量应为 6 个");
            ++skipped;
            continue;
        }
        if (!parse_double_strict(fields[3], &wage) ||
            !parse_double_strict(fields[4], &cap) ||
            !parse_int_strict(fields[5], &recruit)) {
            warn_line(line_number, "时薪、月上限或招聘人数不是合法数值");
            ++skipped;
            continue;
        }
        if (!job_validate(fields[0], fields[1], fields[2], wage, cap, recruit,
                          validate_error, sizeof(validate_error))) {
            warn_line(line_number, validate_error);
            ++skipped;
            continue;
        }
        if (job_find_by_id(app->job_head, fields[0]) != NULL) {
            warn_line(line_number, "岗位编号重复");
            ++skipped;
            continue;
        }
        node = job_create(fields[0], fields[1], fields[2], wage, cap, recruit);
        if (node == NULL) {
            fclose(file);
            set_error(error, error_size, "内存不足，创建岗位结点失败。");
            return 0;
        }
        if (!job_add(&app->job_head, node, validate_error, sizeof(validate_error))) {
            free(node);
            warn_line(line_number, validate_error);
            ++skipped;
            continue;
        }
        ++loaded;
    }
    if (ferror(file)) {
        fclose(file);
        set_error(error, error_size, "读取岗位数据文件时发生 I/O 错误。");
        return 0;
    }
    fclose(file);
    app->job_count = job_list_count(app->job_head);
    printf("岗位数据加载完成：成功 %d 条，跳过 %d 条。\n", loaded, skipped);
    return 1;
}

static int load_salaries(AppData *app, char *error, size_t error_size)
{
    FILE *file;
    char line[LINE_LEN];
    int line_number = 0;
    int loaded = 0;
    int skipped = 0;

    if (app->job_head == NULL && fileio_file_exists(app->salary_file)) {
        print_info("岗位链表为空，薪资记录无法关联岗位，将全部跳过。");
    }
    file = file_open_utf8(app->salary_file, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            print_info("未找到薪资数据文件，将以空薪资链表启动：%s", app->salary_file);
            return 1;
        }
        set_errno_error(error, error_size, "打开薪资数据文件", app->salary_file);
        return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[MAX_FIELDS];
        int count;
        long id;
        double hours;
        double deduction;
        double stored_gross;
        double stored_net;
        int status;
        char validate_error[256];
        char created[DATE_LEN];
        Salary *node;
        const Job *job;

        ++line_number;
        if (line_number == 1) {
            strip_bom(line);
        }
        trim_text(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        count = split_fields(line, fields, MAX_FIELDS);
        if (count != 13) {
            warn_line(line_number, "字段数量应为 13 个");
            ++skipped;
            continue;
        }
        if (!parse_long_strict(fields[0], &id) || id <= 0 ||
            !parse_double_strict(fields[5], &hours) ||
            !parse_double_strict(fields[6], &stored_gross) ||
            !parse_double_strict(fields[7], &deduction) ||
            !parse_double_strict(fields[9], &stored_net) ||
            !parse_int_strict(fields[11], &status)) {
            warn_line(line_number, "编号、工时、金额或状态字段格式错误");
            ++skipped;
            continue;
        }
        job = job_find_by_id(app->job_head, fields[3]);
        if (job == NULL) {
            warn_line(line_number, "岗位编号不存在");
            ++skipped;
            continue;
        }
        if (!date_is_valid(fields[12])) {
            date_today(created, sizeof(created));
        } else {
            copy_text(created, sizeof(created), fields[12]);
        }
        if (!salary_validate(job, fields[1], fields[2], fields[4], hours,
                             deduction, fields[8], status, fields[10],
                             validate_error, sizeof(validate_error))) {
            warn_line(line_number, validate_error);
            ++skipped;
            continue;
        }
        if (salary_find_by_id(app->salary_head, id) != NULL) {
            warn_line(line_number, "薪资记录编号重复");
            ++skipped;
            continue;
        }
        node = salary_create(id, job, fields[1], fields[2], fields[4], hours,
                             deduction, fields[8], status, fields[10], created);
        if (node == NULL) {
            fclose(file);
            set_error(error, error_size, "内存不足，创建薪资结点失败。");
            return 0;
        }
        if (fabs(node->gross_pay - stored_gross) > 0.005 ||
            fabs(node->net_pay - stored_net) > 0.005) {
            printf("[提示] 第 %d 行金额与计算公式不一致，已按公式重算。\n", line_number);
        }
        if (!salary_add(&app->salary_head, node, validate_error,
                        sizeof(validate_error))) {
            free(node);
            warn_line(line_number, validate_error);
            ++skipped;
            continue;
        }
        ++loaded;
    }
    if (ferror(file)) {
        fclose(file);
        set_error(error, error_size, "读取薪资数据文件时发生 I/O 错误。");
        return 0;
    }
    fclose(file);
    app->salary_count = salary_list_count(app->salary_head);
    app->next_salary_id = salary_next_id(app->salary_head);
    printf("薪资数据加载完成：成功 %d 条，跳过 %d 条。\n", loaded, skipped);
    return 1;
}

int fileio_load_all(AppData *app, char *error, size_t error_size)
{
    job_list_clear(&app->job_head);
    salary_list_clear(&app->salary_head);
    app->job_count = 0;
    app->salary_count = 0;
    app->next_salary_id = 1;
    if (!load_jobs(app, error, error_size)) {
        return 0;
    }
    if (!load_salaries(app, error, error_size)) {
        return 0;
    }
    app->job_count = job_list_count(app->job_head);
    app->salary_count = salary_list_count(app->salary_head);
    app->next_salary_id = salary_next_id(app->salary_head);
    app->dirty = 0;
    return 1;
}

static int write_job_record(FILE *file, const Job *job)
{
    return fprintf(file, "%s|%s|%s|%.2f|%.1f|%d\n",
                   job->id, job->name, job->department,
                   job->hourly_wage, job->monthly_hour_cap,
                   job->recruit_count) > 0;
}

static int write_salary_record(FILE *file, const Salary *record)
{
    return fprintf(file, "%ld|%s|%s|%s|%s|%.1f|%.2f|%.2f|%s|%.2f|%s|%d|%s\n",
                   record->id, record->student_id, record->student_name,
                   record->job_id, record->salary_month, record->work_hours,
                   record->gross_pay, record->deduction,
                   record->deduction_reason, record->net_pay,
                   record->status == PAY_PAID ? record->pay_date : "",
                   record->status, record->created_date) > 0;
}

static int save_jobs_to_path(AppData *app, const char *path,
                             char *error, size_t error_size)
{
    char temp_path[PATH_LEN];
    FILE *file;
    const Job *job;
    int ok = 1;

    if (strlen(path) + 5 >= sizeof(temp_path)) {
        set_error(error, error_size, "数据文件路径过长，无法保存。");
        return 0;
    }
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    file = file_open_utf8(temp_path, "w");
    if (file == NULL) {
        set_errno_error(error, error_size, "创建岗位临时文件", temp_path);
        return 0;
    }
    fprintf(file, "# SalarySystem jobs v1\n");
    fprintf(file, "# 字段：岗位编号|名称|所属部门|时薪|月时长上限|招聘人数\n");
    job = app->job_head;
    while (job != NULL) {
        if (!write_job_record(file, job)) {
            ok = 0;
            break;
        }
        job = job->next;
    }
    if (fflush(file) != 0) {
        ok = 0;
    }
    if (fclose(file) != 0) {
        ok = 0;
    }
    if (!ok) {
        file_remove_utf8(temp_path);
        set_errno_error(error, error_size, "写入岗位数据文件", path);
        return 0;
    }
    if (file_replace_utf8(temp_path, path) != 0) {
        file_remove_utf8(temp_path);
        set_errno_error(error, error_size, "替换岗位数据文件", path);
        return 0;
    }
    return 1;
}

static int save_salaries_to_path(AppData *app, const char *path,
                                 char *error, size_t error_size)
{
    char temp_path[PATH_LEN];
    FILE *file;
    const Salary *record;
    int ok = 1;

    if (strlen(path) + 5 >= sizeof(temp_path)) {
        set_error(error, error_size, "数据文件路径过长，无法保存。");
        return 0;
    }
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    file = file_open_utf8(temp_path, "w");
    if (file == NULL) {
        set_errno_error(error, error_size, "创建薪资临时文件", temp_path);
        return 0;
    }
    fprintf(file, "# SalarySystem salaries v1\n");
    fprintf(file, "# 字段：编号|学号|姓名|岗位编号|月份|工时|应发|扣除|扣除原因|实发|发放日期|状态(1已发/0未发)|创建日期\n");
    record = app->salary_head;
    while (record != NULL) {
        if (!write_salary_record(file, record)) {
            ok = 0;
            break;
        }
        record = record->next;
    }
    if (fflush(file) != 0) {
        ok = 0;
    }
    if (fclose(file) != 0) {
        ok = 0;
    }
    if (!ok) {
        file_remove_utf8(temp_path);
        set_errno_error(error, error_size, "写入薪资数据文件", path);
        return 0;
    }
    if (file_replace_utf8(temp_path, path) != 0) {
        file_remove_utf8(temp_path);
        set_errno_error(error, error_size, "替换薪资数据文件", path);
        return 0;
    }
    return 1;
}

int fileio_save_jobs(AppData *app, char *error, size_t error_size)
{
    if (!save_jobs_to_path(app, app->job_file, error, error_size)) {
        return 0;
    }
    app->job_count = job_list_count(app->job_head);
    return 1;
}

int fileio_save_salaries(AppData *app, char *error, size_t error_size)
{
    if (!save_salaries_to_path(app, app->salary_file, error, error_size)) {
        return 0;
    }
    app->salary_count = salary_list_count(app->salary_head);
    app->next_salary_id = salary_next_id(app->salary_head);
    return 1;
}

int fileio_save_all(AppData *app, char *error, size_t error_size)
{
    if (!fileio_save_jobs(app, error, error_size)) {
        return 0;
    }
    if (!fileio_save_salaries(app, error, error_size)) {
        return 0;
    }
    app->dirty = 0;
    return 1;
}

static void format_now(char *out, size_t out_size)
{
    time_t now = time(NULL);
    struct tm local_value;

#ifdef _MSC_VER
    localtime_s(&local_value, &now);
#else
    {
        struct tm *ptr = localtime(&now);
        if (ptr != NULL) {
            local_value = *ptr;
        } else {
            memset(&local_value, 0, sizeof(local_value));
        }
    }
#endif
    strftime(out, out_size, "%Y-%m-%d %H:%M:%S", &local_value);
}

int fileio_export_salary_all(AppData *app, char *error, size_t error_size)
{
    char temp_path[PATH_LEN];
    char now[32];
    FILE *file;
    const Salary *record;
    int ok = 1;

    if (strlen(app->backup_all_file) + 5 >= sizeof(temp_path)) {
        set_error(error, error_size, "备份文件路径过长。");
        return 0;
    }
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", app->backup_all_file);
    file = file_open_utf8(temp_path, "w");
    if (file == NULL) {
        set_errno_error(error, error_size, "创建全量备份文件", temp_path);
        return 0;
    }
    format_now(now, sizeof(now));
    fprintf(file, "# ============================================================\n");
    fprintf(file, "# 大学生勤工助学薪资发放管理系统 - 薪资数据全量备份\n");
    fprintf(file, "# 导出时间：%s\n", now);
    fprintf(file, "# 记录数量：%d\n", app->salary_count);
    fprintf(file, "# 字段：编号|学号|姓名|岗位编号|月份|工时|应发|扣除|扣除原因|实发|发放日期|状态(1已发/0未发)|创建日期\n");
    fprintf(file, "# ============================================================\n");
    record = app->salary_head;
    while (record != NULL) {
        if (!write_salary_record(file, record)) {
            ok = 0;
            break;
        }
        record = record->next;
    }
    if (fflush(file) != 0) {
        ok = 0;
    }
    if (fclose(file) != 0) {
        ok = 0;
    }
    if (!ok) {
        file_remove_utf8(temp_path);
        set_errno_error(error, error_size, "写入全量备份文件", app->backup_all_file);
        return 0;
    }
    if (file_replace_utf8(temp_path, app->backup_all_file) != 0) {
        file_remove_utf8(temp_path);
        set_errno_error(error, error_size, "生成全量备份文件", app->backup_all_file);
        return 0;
    }
    return 1;
}

static const char *effective_date(const Salary *record)
{
    if (record->status == PAY_PAID && record->pay_date[0] != '\0') {
        return record->pay_date;
    }
    return record->created_date;
}

int fileio_archive_old_records(AppData *app, int *removed_count,
                               char *error, size_t error_size)
{
    char cutoff[DATE_LEN];
    char today[DATE_LEN];
    char now[32];
    FILE *file;
    const Salary *record;
    int old_count = 0;
    int removed = 0;

    if (removed_count != NULL) {
        *removed_count = 0;
    }
    date_today(today, sizeof(today));
    date_minus_years(today, 2, cutoff, sizeof(cutoff));
    record = app->salary_head;
    while (record != NULL) {
        if (date_is_before_years(effective_date(record), 2)) {
            ++old_count;
        }
        record = record->next;
    }
    if (old_count == 0) {
        print_info("没有超过两年的薪资记录（截止日期：%s）。", cutoff);
        return 1;
    }
    file = file_open_utf8(app->backup_file, "a");
    if (file == NULL) {
        set_errno_error(error, error_size, "打开超期记录备份文件", app->backup_file);
        return 0;
    }
    format_now(now, sizeof(now));
    fprintf(file, "\n# ============================================================\n");
    fprintf(file, "# 超期薪资记录归档\n");
    fprintf(file, "# 归档时间：%s\n", now);
    fprintf(file, "# 截止日期：%s（早于该日期的记录被清理）\n", cutoff);
    fprintf(file, "# 记录数量：%d\n", old_count);
    fprintf(file, "# ============================================================\n");
    record = app->salary_head;
    while (record != NULL) {
        if (date_is_before_years(effective_date(record), 2)) {
            if (!write_salary_record(file, record)) {
                fclose(file);
                set_error(error, error_size, "写入超期记录备份文件失败。");
                return 0;
            }
        }
        record = record->next;
    }
    if (fflush(file) != 0 || fclose(file) != 0) {
        set_error(error, error_size, "关闭超期记录备份文件失败，未删除任何记录。");
        return 0;
    }

    {
        Salary **cursor = &app->salary_head;
        while (*cursor != NULL) {
            if (date_is_before_years(effective_date(*cursor), 2)) {
                Salary *target = *cursor;
                *cursor = target->next;
                free(target);
                ++removed;
            } else {
                cursor = &(*cursor)->next;
            }
        }
    }
    app->salary_count = salary_list_count(app->salary_head);
    app->next_salary_id = salary_next_id(app->salary_head);
    app->dirty = 1;
    if (!fileio_save_salaries(app, error, error_size)) {
        return 0;
    }
    if (removed_count != NULL) {
        *removed_count = removed;
    }
    return 1;
}
