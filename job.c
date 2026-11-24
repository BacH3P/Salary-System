/*
 * 文件名称：job.c
 * 功能说明：岗位信息动态单链表的建立、查找、增加、修改、删除和显示。
 */

#include "job.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) {
        copy_text(error, error_size, message);
    }
}

static int job_id_is_valid(const char *id)
{
    const char *p;

    if (id == NULL || id[0] != 'J' || strlen(id) < 4 || strlen(id) >= JOB_ID_LEN) {
        return 0;
    }
    for (p = id + 1; *p != '\0'; ++p) {
        if (!isdigit((unsigned char)*p)) {
            return 0;
        }
    }
    return 1;
}

void job_list_clear(Job **head)
{
    Job *current;
    Job *next;

    if (head == NULL) {
        return;
    }
    current = *head;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *head = NULL;
}

int job_list_count(const Job *head)
{
    int count = 0;
    while (head != NULL) {
        ++count;
        head = head->next;
    }
    return count;
}

Job *job_create(const char *id, const char *name, const char *department,
                double hourly_wage, double monthly_hour_cap, int recruit_count)
{
    Job *node = (Job *)calloc(1, sizeof(Job));
    if (node == NULL) {
        return NULL;
    }
    copy_text(node->id, sizeof(node->id), id);
    copy_text(node->name, sizeof(node->name), name);
    copy_text(node->department, sizeof(node->department), department);
    node->hourly_wage = hourly_wage;
    node->monthly_hour_cap = monthly_hour_cap;
    node->recruit_count = recruit_count;
    node->next = NULL;
    return node;
}

Job *job_find_by_id(Job *head, const char *id)
{
    while (head != NULL) {
        if (strcmp(head->id, id) == 0) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

int job_validate(const char *id, const char *name, const char *department,
                 double hourly_wage, double monthly_hour_cap, int recruit_count,
                 char *error, size_t error_size)
{
    if (!job_id_is_valid(id)) {
        set_error(error, error_size, "岗位编号格式应为“J+至少3位数字”，例如 J009。");
        return 0;
    }
    if (text_is_blank(name) || strlen(name) >= JOB_NAME_LEN) {
        set_error(error, error_size, "岗位名称不能为空且长度不能超过 63 个字节。");
        return 0;
    }
    if (text_is_blank(department) || strlen(department) >= DEPT_NAME_LEN) {
        set_error(error, error_size, "所属部门不能为空且长度不能超过 63 个字节。");
        return 0;
    }
    if (text_has_forbidden_char(name) || text_has_forbidden_char(department)) {
        set_error(error, error_size, "岗位名称或部门名称含有非法字符“|”。");
        return 0;
    }
    if (hourly_wage <= 0.0 || hourly_wage > MAX_HOURLY_WAGE) {
        set_error(error, error_size, "时薪必须大于 0 且不超过 1000 元/小时。");
        return 0;
    }
    if (monthly_hour_cap <= 0.0 || monthly_hour_cap > MAX_MONTH_HOURS) {
        set_error(error, error_size, "月工作时长上限必须大于 0 且不超过 744 小时。");
        return 0;
    }
    if (recruit_count <= 0 || recruit_count > 9999) {
        set_error(error, error_size, "招聘人数必须在 1 到 9999 之间。");
        return 0;
    }
    return 1;
}

int job_add(Job **head, Job *node, char *error, size_t error_size)
{
    Job *tail;

    if (head == NULL || node == NULL) {
        set_error(error, error_size, "岗位链表或结点为空，无法添加。");
        return 0;
    }
    if (job_find_by_id(*head, node->id) != NULL) {
        set_error(error, error_size, "岗位编号已存在，不能重复添加。");
        return 0;
    }
    if (*head == NULL) {
        *head = node;
        return 1;
    }
    tail = *head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = node;
    return 1;
}

int job_update(Job *job, const char *name, const char *department,
               double hourly_wage, double monthly_hour_cap, int recruit_count,
               char *error, size_t error_size)
{
    if (job == NULL) {
        set_error(error, error_size, "岗位不存在。");
        return 0;
    }
    if (!job_validate(job->id, name, department, hourly_wage, monthly_hour_cap,
                      recruit_count, error, error_size)) {
        return 0;
    }
    copy_text(job->name, sizeof(job->name), name);
    copy_text(job->department, sizeof(job->department), department);
    job->hourly_wage = hourly_wage;
    job->monthly_hour_cap = monthly_hour_cap;
    job->recruit_count = recruit_count;
    return 1;
}

int job_delete(Job **head, const char *id, const Salary *salaries,
               char *error, size_t error_size)
{
    Job **cursor;
    Job *target;

    if (head == NULL || *head == NULL) {
        set_error(error, error_size, "岗位链表为空，没有可删除的岗位。");
        return 0;
    }
    cursor = head;
    while (*cursor != NULL && strcmp((*cursor)->id, id) != 0) {
        cursor = &(*cursor)->next;
    }
    if (*cursor == NULL) {
        set_error(error, error_size, "未找到指定编号的岗位。");
        return 0;
    }
    target = *cursor;
    {
        int references = 0;
        const Salary *record = salaries;
        while (record != NULL) {
            if (strcmp(record->job_id, id) == 0) {
                ++references;
            }
            record = record->next;
        }
        if (references > 0) {
            char message[256];
            snprintf(message, sizeof(message),
                     "该岗位仍被 %d 条薪资记录引用，请先删除相关薪资记录后再删除岗位。",
                     references);
            set_error(error, error_size, message);
            return 0;
        }
    }
    *cursor = target->next;
    free(target);
    return 1;
}

static void job_print_header(void)
{
    print_rule(88);
    putchar('|');
    print_padded("编号", 8, 0);
    putchar('|');
    print_padded("岗位名称", 24, 0);
    putchar('|');
    print_padded("所属部门", 18, 0);
    putchar('|');
    print_padded("时薪(元/时)", 12, 0);
    putchar('|');
    print_padded("月时长上限", 12, 0);
    putchar('|');
    print_padded("招聘人数", 10, 0);
    putchar('|');
    putchar('\n');
    print_rule(88);
}

void job_print_table(const Job *head)
{
    char buffer[64];

    if (head == NULL) {
        print_info("岗位链表为空。");
        return;
    }
    job_print_header();
    while (head != NULL) {
        putchar('|');
        print_padded(head->id, 8, 0);
        putchar('|');
        print_padded(head->name, 24, 0);
        putchar('|');
        print_padded(head->department, 18, 0);
        putchar('|');
        snprintf(buffer, sizeof(buffer), "%.2f", head->hourly_wage);
        print_padded(buffer, 12, 1);
        putchar('|');
        snprintf(buffer, sizeof(buffer), "%.1f", head->monthly_hour_cap);
        print_padded(buffer, 12, 1);
        putchar('|');
        snprintf(buffer, sizeof(buffer), "%d", head->recruit_count);
        print_padded(buffer, 10, 1);
        putchar('|');
        putchar('\n');
        head = head->next;
    }
    print_rule(88);
}

void job_print_detail(const Job *job)
{
    if (job == NULL) {
        print_info("岗位不存在。");
        return;
    }
    printf("  岗位编号：%s\n", job->id);
    printf("  岗位名称：%s\n", job->name);
    printf("  所属部门：%s\n", job->department);
    printf("  时    薪：%.2f 元/小时\n", job->hourly_wage);
    printf("  月上限  ：%.1f 小时\n", job->monthly_hour_cap);
    printf("  招聘人数：%d 人\n", job->recruit_count);
}

static void job_query_common(const Job *head, const char *keyword, int by_department)
{
    int found = 0;

    if (head == NULL) {
        print_info("岗位链表为空，无法查询。");
        return;
    }
    if (text_is_blank(keyword)) {
        print_error("查询关键字不能为空。");
        return;
    }
    job_print_header();
    while (head != NULL) {
        const char *field = by_department ? head->department : head->name;
        if (text_contains(field, keyword)) {
            char buffer[64];
            ++found;
            putchar('|');
            print_padded(head->id, 8, 0);
            putchar('|');
            print_padded(head->name, 24, 0);
            putchar('|');
            print_padded(head->department, 18, 0);
            putchar('|');
            snprintf(buffer, sizeof(buffer), "%.2f", head->hourly_wage);
            print_padded(buffer, 12, 1);
            putchar('|');
            snprintf(buffer, sizeof(buffer), "%.1f", head->monthly_hour_cap);
            print_padded(buffer, 12, 1);
            putchar('|');
            snprintf(buffer, sizeof(buffer), "%d", head->recruit_count);
            print_padded(buffer, 10, 1);
            putchar('|');
            putchar('\n');
        }
        head = head->next;
    }
    print_rule(88);
    printf("共查询到 %d 个岗位。\n", found);
}

void job_query_by_department(const Job *head, const char *department)
{
    job_query_common(head, department, 1);
}

void job_query_by_name(const Job *head, const char *name)
{
    job_query_common(head, name, 0);
}
