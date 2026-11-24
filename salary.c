/*
 * 文件名称：salary.c
 * 功能说明：薪资记录动态单链表的建立、校验、增删改、状态更新和按实发薪资排序。
 *           核心计算关系：应发薪资 = 时薪 × 工作时长；
 *                         实发薪资 = 应发薪资 - 扣除金额。
 */

#include "salary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) {
        copy_text(error, error_size, message);
    }
}

void salary_list_clear(Salary **head)
{
    Salary *current;
    Salary *next;

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

int salary_list_count(const Salary *head)
{
    int count = 0;
    while (head != NULL) {
        ++count;
        head = head->next;
    }
    return count;
}

long salary_next_id(const Salary *head)
{
    long maximum = 0;
    while (head != NULL) {
        if (head->id > maximum) {
            maximum = head->id;
        }
        head = head->next;
    }
    return maximum + 1;
}

Salary *salary_find_by_id(Salary *head, long id)
{
    while (head != NULL) {
        if (head->id == id) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

double salary_calc_gross(const Job *job, double work_hours)
{
    if (job == NULL) {
        return 0.0;
    }
    return round_money(job->hourly_wage * work_hours);
}

double salary_calc_net(double gross_pay, double deduction)
{
    return round_money(gross_pay - deduction);
}

Salary *salary_create(long id, const Job *job, const char *student_id,
                      const char *student_name, const char *salary_month,
                      double work_hours, double deduction,
                      const char *deduction_reason, int status,
                      const char *pay_date, const char *created_date)
{
    Salary *node;
    double gross;

    if (job == NULL) {
        return NULL;
    }
    node = (Salary *)calloc(1, sizeof(Salary));
    if (node == NULL) {
        return NULL;
    }
    node->id = id;
    copy_text(node->student_id, sizeof(node->student_id), student_id);
    copy_text(node->student_name, sizeof(node->student_name), student_name);
    copy_text(node->job_id, sizeof(node->job_id), job->id);
    copy_text(node->salary_month, sizeof(node->salary_month), salary_month);
    node->work_hours = work_hours;
    gross = salary_calc_gross(job, work_hours);
    node->gross_pay = gross;
    node->deduction = round_money(deduction);
    copy_text(node->deduction_reason, sizeof(node->deduction_reason),
              deduction_reason == NULL ? "" : deduction_reason);
    node->net_pay = salary_calc_net(gross, node->deduction);
    node->status = status == PAY_PAID ? PAY_PAID : PAY_UNPAID;
    copy_text(node->pay_date, sizeof(node->pay_date),
              node->status == PAY_PAID && pay_date != NULL ? pay_date : "");
    copy_text(node->created_date, sizeof(node->created_date),
              created_date == NULL ? "" : created_date);
    node->next = NULL;
    return node;
}

int salary_validate(const Job *job, const char *student_id, const char *student_name,
                    const char *salary_month, double work_hours,
                    double deduction, const char *deduction_reason,
                    int status, const char *pay_date,
                    char *error, size_t error_size)
{
    double gross;

    if (job == NULL) {
        set_error(error, error_size, "岗位不存在，无法录入薪资。");
        return 0;
    }
    if (text_is_blank(student_id) || strlen(student_id) >= STUDENT_ID_LEN) {
        set_error(error, error_size, "学号不能为空且长度不能超过 31 个字节。");
        return 0;
    }
    if (text_has_forbidden_char(student_id) || strchr(student_id, ' ') != NULL) {
        set_error(error, error_size, "学号中不能包含空格或竖线“|”。");
        return 0;
    }
    if (text_is_blank(student_name) || strlen(student_name) >= STUDENT_NAME_LEN) {
        set_error(error, error_size, "姓名不能为空且长度不能超过 63 个字节。");
        return 0;
    }
    if (text_has_forbidden_char(student_name)) {
        set_error(error, error_size, "姓名中不能包含竖线“|”。");
        return 0;
    }
    if (!month_is_valid(salary_month)) {
        set_error(error, error_size, "薪资月份格式错误，应为 YYYY-MM。");
        return 0;
    }
    if (work_hours <= 0.0) {
        set_error(error, error_size, "当月工作时长必须大于 0。");
        return 0;
    }
    if (work_hours > job->monthly_hour_cap) {
        char message[256];
        snprintf(message, sizeof(message),
                 "工作时长 %.1f 小时超过岗位“%s”的月上限 %.1f 小时。",
                 work_hours, job->name, job->monthly_hour_cap);
        set_error(error, error_size, message);
        return 0;
    }
    gross = salary_calc_gross(job, work_hours);
    if (deduction < 0.0) {
        set_error(error, error_size, "扣除金额不能为负数。");
        return 0;
    }
    if (deduction > gross) {
        char message[256];
        snprintf(message, sizeof(message),
                 "扣除金额 %.2f 元不能超过应发薪资 %.2f 元。", deduction, gross);
        set_error(error, error_size, message);
        return 0;
    }
    if (deduction > 0.0 && text_is_blank(deduction_reason)) {
        set_error(error, error_size, "存在扣除金额时必须填写扣除原因（如“迟到2次，扣款50元”）。");
        return 0;
    }
    if (deduction_reason != NULL && strlen(deduction_reason) >= REASON_LEN) {
        set_error(error, error_size, "扣除原因过长，请控制在 255 个字节以内。");
        return 0;
    }
    if (deduction_reason != NULL && text_has_forbidden_char(deduction_reason)) {
        set_error(error, error_size, "扣除原因中不能包含竖线“|”。");
        return 0;
    }
    if (status != PAY_PAID && status != PAY_UNPAID) {
        set_error(error, error_size, "发放状态取值非法。");
        return 0;
    }
    if (status == PAY_PAID) {
        if (!date_is_valid(pay_date)) {
            set_error(error, error_size, "已发放记录必须填写合法的发放日期。");
            return 0;
        }
    } else if (pay_date != NULL && *pay_date != '\0') {
        set_error(error, error_size, "未发放记录的发放日期必须为空。");
        return 0;
    }
    return 1;
}

static int salary_key_exists(const Salary *head, const Salary *candidate, long ignore_id)
{
    while (head != NULL) {
        if (head->id != ignore_id &&
            strcmp(head->student_id, candidate->student_id) == 0 &&
            strcmp(head->job_id, candidate->job_id) == 0 &&
            strcmp(head->salary_month, candidate->salary_month) == 0) {
            return 1;
        }
        head = head->next;
    }
    return 0;
}

int salary_add(Salary **head, Salary *node, char *error, size_t error_size)
{
    Salary *tail;

    if (head == NULL || node == NULL) {
        set_error(error, error_size, "薪资链表或结点为空，无法添加。");
        return 0;
    }
    if (node->id <= 0) {
        set_error(error, error_size, "薪资记录编号非法。");
        return 0;
    }
    if (salary_find_by_id(*head, node->id) != NULL) {
        set_error(error, error_size, "薪资记录编号重复。");
        return 0;
    }
    if (salary_key_exists(*head, node, 0)) {
        set_error(error, error_size,
                  "同一学生、同一岗位、同一月份已存在薪资记录，不能重复录入。");
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

int salary_update(Salary *record, const Job *job, double work_hours,
                  double deduction, const char *deduction_reason,
                  int status, const char *pay_date,
                  char *error, size_t error_size)
{
    double gross;

    if (record == NULL) {
        set_error(error, error_size, "薪资记录不存在。");
        return 0;
    }
    if (!salary_validate(job, record->student_id, record->student_name,
                         record->salary_month, work_hours, deduction,
                         deduction_reason, status, pay_date, error, error_size)) {
        return 0;
    }
    gross = salary_calc_gross(job, work_hours);
    record->work_hours = work_hours;
    record->gross_pay = gross;
    record->deduction = round_money(deduction);
    copy_text(record->deduction_reason, sizeof(record->deduction_reason),
              deduction > 0.0 ? deduction_reason : "");
    record->net_pay = salary_calc_net(gross, record->deduction);
    record->status = status;
    copy_text(record->pay_date, sizeof(record->pay_date),
              status == PAY_PAID ? pay_date : "");
    return 1;
}

int salary_update_status(Salary *record, int status, const char *pay_date,
                         char *error, size_t error_size)
{
    if (record == NULL) {
        set_error(error, error_size, "薪资记录不存在。");
        return 0;
    }
    if (status != PAY_PAID && status != PAY_UNPAID) {
        set_error(error, error_size, "发放状态取值非法。");
        return 0;
    }
    if (status == PAY_PAID) {
        if (!date_is_valid(pay_date)) {
            set_error(error, error_size, "发放日期格式错误。");
            return 0;
        }
        record->status = PAY_PAID;
        copy_text(record->pay_date, sizeof(record->pay_date), pay_date);
    } else {
        record->status = PAY_UNPAID;
        record->pay_date[0] = '\0';
    }
    return 1;
}

int salary_delete(Salary **head, long id, char *error, size_t error_size)
{
    Salary **cursor;
    Salary *target;

    if (head == NULL || *head == NULL) {
        set_error(error, error_size, "薪资链表为空，没有可删除的记录。");
        return 0;
    }
    cursor = head;
    while (*cursor != NULL && (*cursor)->id != id) {
        cursor = &(*cursor)->next;
    }
    if (*cursor == NULL) {
        set_error(error, error_size, "未找到指定编号的薪资记录。");
        return 0;
    }
    target = *cursor;
    *cursor = target->next;
    free(target);
    return 1;
}

static int should_come_before(const Salary *left, const Salary *right, int ascending)
{
    if (left->net_pay < right->net_pay) {
        return ascending ? 1 : 0;
    }
    if (left->net_pay > right->net_pay) {
        return ascending ? 0 : 1;
    }
    return left->id < right->id;
}

void salary_sort_by_net(Salary **head, int ascending)
{
    Salary *sorted = NULL;

    if (head == NULL) {
        return;
    }
    while (*head != NULL) {
        Salary *node = *head;
        Salary **position = &sorted;
        *head = node->next;
        while (*position != NULL &&
               !should_come_before(node, *position, ascending)) {
            position = &(*position)->next;
        }
        node->next = *position;
        *position = node;
    }
    *head = sorted;
}

void salary_print_detail(const Salary *record, const Job *job)
{
    if (record == NULL) {
        print_info("薪资记录不存在。");
        return;
    }
    printf("  记录编号：%ld\n", record->id);
    printf("  学    号：%s\n", record->student_id);
    printf("  姓    名：%s\n", record->student_name);
    printf("  岗位编号：%s", record->job_id);
    if (job != NULL) {
        printf("（%s / %s）", job->name, job->department);
    }
    putchar('\n');
    printf("  薪资月份：%s\n", record->salary_month);
    printf("  工作时长：%.1f 小时\n", record->work_hours);
    printf("  应发薪资：%.2f 元\n", record->gross_pay);
    printf("  扣除金额：%.2f 元\n", record->deduction);
    printf("  扣除原因：%s\n",
           record->deduction > 0.0 ? record->deduction_reason : "无");
    printf("  实发薪资：%.2f 元\n", record->net_pay);
    printf("  发放状态：%s\n", pay_status_text(record->status));
    printf("  发放时间：%s\n",
           record->status == PAY_PAID ? record->pay_date : "未发放");
}
