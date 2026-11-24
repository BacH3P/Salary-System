/*
 * 文件名称：query.c
 * 功能说明：按学号、部门、发放时间进行组合查询，按实发薪资排序，
 *           统计部门月度薪资支出，并输出测试数据概览。
 */

#include "query.h"
#include "input.h"
#include "job.h"
#include "salary.h"

#include <stdio.h>
#include <string.h>

#define SALARY_TABLE_WIDTH 104

static void salary_table_header(void)
{
    print_rule(SALARY_TABLE_WIDTH);
    putchar('|');
    print_padded("记录", 5, 0);
    putchar('|');
    print_padded("学号", 9, 0);
    putchar('|');
    print_padded("姓名", 8, 0);
    putchar('|');
    print_padded("岗位", 6, 0);
    putchar('|');
    print_padded("所属部门", 12, 0);
    putchar('|');
    print_padded("月份", 7, 0);
    putchar('|');
    print_padded("工时", 5, 0);
    putchar('|');
    print_padded("应发", 8, 0);
    putchar('|');
    print_padded("扣款", 7, 0);
    putchar('|');
    print_padded("实发", 8, 0);
    putchar('|');
    print_padded("状态", 6, 0);
    putchar('|');
    print_padded("发放日期", 10, 0);
    putchar('|');
    putchar('\n');
    print_rule(SALARY_TABLE_WIDTH);
}

static void salary_table_row(const Salary *record, const Job *job)
{
    char buffer[64];

    putchar('|');
    snprintf(buffer, sizeof(buffer), "%ld", record->id);
    print_padded(buffer, 5, 0);
    putchar('|');
    print_padded(record->student_id, 9, 0);
    putchar('|');
    print_padded(record->student_name, 8, 0);
    putchar('|');
    print_padded(record->job_id, 6, 0);
    putchar('|');
    print_padded(job == NULL ? "未知" : job->department, 12, 0);
    putchar('|');
    print_padded(record->salary_month, 7, 0);
    putchar('|');
    snprintf(buffer, sizeof(buffer), "%.1f", record->work_hours);
    print_padded(buffer, 5, 1);
    putchar('|');
    snprintf(buffer, sizeof(buffer), "%.2f", record->gross_pay);
    print_padded(buffer, 8, 1);
    putchar('|');
    snprintf(buffer, sizeof(buffer), "%.2f", record->deduction);
    print_padded(buffer, 7, 1);
    putchar('|');
    snprintf(buffer, sizeof(buffer), "%.2f", record->net_pay);
    print_padded(buffer, 8, 1);
    putchar('|');
    print_padded(pay_status_text(record->status), 6, 0);
    putchar('|');
    print_padded(record->status == PAY_PAID ? record->pay_date : "-", 10, 0);
    putchar('|');
    putchar('\n');
}

void salary_filter_init(SalaryFilter *filter)
{
    memset(filter, 0, sizeof(*filter));
    filter->status = -1;
}

const Job *find_job_const(const Job *head, const char *job_id)
{
    while (head != NULL) {
        if (strcmp(head->id, job_id) == 0) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

int salary_matches(const Salary *record, const Job *jobs, const SalaryFilter *filter)
{
    const Job *job;

    if (record == NULL) {
        return 0;
    }
    if (filter == NULL) {
        return 1;
    }
    if (filter->student_id[0] != '\0' &&
        strcmp(record->student_id, filter->student_id) != 0) {
        return 0;
    }
    if (filter->status != -1 && record->status != filter->status) {
        return 0;
    }
    if (filter->has_month && strcmp(record->salary_month, filter->month) != 0) {
        return 0;
    }
    job = find_job_const(jobs, record->job_id);
    if (filter->department[0] != '\0') {
        if (job == NULL || !text_contains(job->department, filter->department)) {
            return 0;
        }
    }
    if (filter->date_from[0] != '\0' || filter->date_to[0] != '\0') {
        if (record->status != PAY_PAID || !date_is_valid(record->pay_date)) {
            return 0;
        }
        if (filter->date_from[0] != '\0' &&
            date_compare(record->pay_date, filter->date_from) < 0) {
            return 0;
        }
        if (filter->date_to[0] != '\0' &&
            date_compare(record->pay_date, filter->date_to) > 0) {
            return 0;
        }
    }
    return 1;
}

void query_print_salary_table(const AppData *app, const SalaryFilter *filter,
                              const char *title)
{
    const Salary *record;
    SalarySummary summary;

    memset(&summary, 0, sizeof(summary));
    printf("\n【%s】\n", title == NULL ? "薪资记录" : title);
    if (app == NULL || app->salary_head == NULL) {
        print_info("薪资链表为空，暂无记录。");
        return;
    }
    salary_table_header();
    record = app->salary_head;
    while (record != NULL) {
        if (salary_matches(record, app->job_head, filter)) {
            const Job *job = find_job_const(app->job_head, record->job_id);
            salary_table_row(record, job);
            ++summary.count;
            if (record->status == PAY_PAID) {
                ++summary.paid_count;
                summary.paid_net_total += record->net_pay;
            } else {
                ++summary.unpaid_count;
            }
            if (record->deduction > 0.0) {
                ++summary.deduction_count;
            }
            summary.gross_total += record->gross_pay;
            summary.deduction_total += record->deduction;
            summary.net_total += record->net_pay;
        }
        record = record->next;
    }
    print_rule(SALARY_TABLE_WIDTH);
    if (summary.count == 0) {
        print_info("没有符合条件的薪资记录。");
        return;
    }
    printf("记录 %d 条（已发放 %d 条，未发放 %d 条）；应发合计 %.2f 元，"
           "扣除合计 %.2f 元，实发合计 %.2f 元。\n",
           summary.count, summary.paid_count, summary.unpaid_count,
           summary.gross_total, summary.deduction_total, summary.net_total);
    printf("其中已发放实发合计 %.2f 元，未发放实发合计 %.2f 元。\n",
           summary.paid_net_total, summary.net_total - summary.paid_net_total);

    if (summary.deduction_count > 0) {
        printf("\n扣款明细（共 %d 条）：\n", summary.deduction_count);
        record = app->salary_head;
        while (record != NULL) {
            if (salary_matches(record, app->job_head, filter) &&
                record->deduction > 0.0) {
                printf("  记录 %-5ld 学号 %-10s 扣款 %8.2f 元  原因：%s\n",
                       record->id, record->student_id, record->deduction,
                       record->deduction_reason);
            }
            record = record->next;
        }
    }
}

void query_by_student(AppData *app)
{
    SalaryFilter filter;
    char student_id[STUDENT_ID_LEN];

    salary_filter_init(&filter);
    if (!input_text("请输入要查询的学号：", student_id, sizeof(student_id), 0)) {
        return;
    }
    copy_text(filter.student_id, sizeof(filter.student_id), student_id);
    query_print_salary_table(app, &filter, "按学号查询薪资记录");
}

void query_by_department(AppData *app)
{
    SalaryFilter filter;
    char department[DEPT_NAME_LEN];

    salary_filter_init(&filter);
    if (!input_text("请输入部门名称（支持部分匹配）：", department,
                    sizeof(department), 0)) {
        return;
    }
    copy_text(filter.department, sizeof(filter.department), department);
    query_print_salary_table(app, &filter, "按部门查询薪资记录");
}

void query_by_pay_date(AppData *app)
{
    SalaryFilter filter;
    char from[DATE_LEN];
    char to[DATE_LEN];

    salary_filter_init(&filter);
    filter.status = PAY_PAID;
    if (!input_date_value("起始发放日期（YYYY-MM-DD，直接回车表示不限）：",
                          1, 0, from, sizeof(from))) {
        return;
    }
    if (!input_date_value("结束发放日期（YYYY-MM-DD，直接回车表示不限）：",
                          1, 0, to, sizeof(to))) {
        return;
    }
    if (from[0] != '\0' && to[0] != '\0' && date_compare(from, to) > 0) {
        print_error("起始日期不能晚于结束日期。");
        return;
    }
    copy_text(filter.date_from, sizeof(filter.date_from), from);
    copy_text(filter.date_to, sizeof(filter.date_to), to);
    query_print_salary_table(app, &filter, "按发放时间查询薪资记录");
}

void query_by_sort(AppData *app, int ascending)
{
    SalaryFilter filter;

    salary_filter_init(&filter);
    salary_sort_by_net(&app->salary_head, ascending);
    query_print_salary_table(app, &filter,
                             ascending ? "全体记录按实发薪资升序排列"
                                       : "全体记录按实发薪资降序排列");
}

static int department_exists(const Job *jobs, const char *department)
{
    while (jobs != NULL) {
        if (strcmp(jobs->department, department) == 0) {
            return 1;
        }
        jobs = jobs->next;
    }
    return 0;
}

static void print_department_list(const Job *jobs)
{
    const Job *job;
    const char *seen[64];
    int count = 0;
    int i;

    job = jobs;
    while (job != NULL && count < 64) {
        int exists = 0;
        for (i = 0; i < count; ++i) {
            if (strcmp(seen[i], job->department) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists) {
            seen[count++] = job->department;
        }
        job = job->next;
    }
    printf("  现有部门：");
    for (i = 0; i < count; ++i) {
        printf("%s%s", seen[i], i + 1 < count ? "、" : "\n");
    }
}

void query_department_month_total(AppData *app)
{
    char department[DEPT_NAME_LEN];
    char month[MONTH_LEN];
    const Job *job;
    const Salary *record;
    int records = 0;
    int jobs_in_department = 0;
    double gross_total = 0.0;
    double deduction_total = 0.0;
    double net_total = 0.0;
    double paid_net_total = 0.0;

    if (app->job_head == NULL) {
        print_info("岗位链表为空，无法统计。");
        return;
    }
    print_department_list(app->job_head);
    if (!input_text("请输入要统计的部门（须与列表中的名称一致）：",
                    department, sizeof(department), 0)) {
        return;
    }
    if (!department_exists(app->job_head, department)) {
        print_error("未找到部门“%s”，请检查输入。", department);
        return;
    }
    if (!input_month_value("统计月份（YYYY-MM）：", 1, month, sizeof(month))) {
        return;
    }

    job = app->job_head;
    while (job != NULL) {
        if (strcmp(job->department, department) == 0) {
            ++jobs_in_department;
        }
        job = job->next;
    }
    record = app->salary_head;
    while (record != NULL) {
        job = find_job_const(app->job_head, record->job_id);
        if (job != NULL && strcmp(job->department, department) == 0 &&
            strcmp(record->salary_month, month) == 0) {
            ++records;
            gross_total += record->gross_pay;
            deduction_total += record->deduction;
            net_total += record->net_pay;
            if (record->status == PAY_PAID) {
                paid_net_total += record->net_pay;
            }
        }
        record = record->next;
    }

    printf("\n【部门月度薪资统计】\n");
    printf("  部门名称：%s\n", department);
    printf("  统计月份：%s\n", month);
    printf("  岗位数量：%d 个\n", jobs_in_department);
    printf("  薪资记录：%d 条\n", records);
    if (records > 0) {
        printf("  应发合计：%.2f 元\n", gross_total);
        printf("  扣除合计：%.2f 元\n", deduction_total);
        printf("  实发合计：%.2f 元（含未发放）\n", net_total);
        printf("  已发放实发合计：%.2f 元\n", paid_net_total);
        printf("  实际薪资支出（已发放）：%.2f 元\n", paid_net_total);
    } else {
        print_info("该部门在 %s 没有薪资记录。", month);
    }
}

static int array_contains(char array[][STUDENT_ID_LEN], int count, const char *value)
{
    int i;
    for (i = 0; i < count; ++i) {
        if (strcmp(array[i], value) == 0) {
            return 1;
        }
    }
    return 0;
}

static void print_requirement_check(const char *label, int actual, int required)
{
    printf("  ");
    print_padded(label, 22, 0);
    printf(" 实际 %3d  %s（要求 ≥ %d）\n",
           actual, actual >= required ? "[满足]" : "[不足]", required);
}

void query_data_overview(AppData *app)
{
    char departments[128][DEPT_NAME_LEN];
    char students[512][STUDENT_ID_LEN];
    int department_count = 0;
    int student_count = 0;
    int paid_count = 0;
    int unpaid_count = 0;
    int deduction_count = 0;
    int old_count = 0;
    const Job *job;
    const Salary *record;
    char effective_date[DATE_LEN];

    job = app->job_head;
    while (job != NULL) {
        int i;
        int exists = 0;
        for (i = 0; i < department_count; ++i) {
            if (strcmp(departments[i], job->department) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists && department_count < 128) {
            copy_text(departments[department_count], DEPT_NAME_LEN, job->department);
            ++department_count;
        }
        job = job->next;
    }

    record = app->salary_head;
    while (record != NULL) {
        if (!array_contains(students, student_count, record->student_id) &&
            student_count < 512) {
            copy_text(students[student_count], STUDENT_ID_LEN, record->student_id);
            ++student_count;
        }
        if (record->status == PAY_PAID) {
            ++paid_count;
        } else {
            ++unpaid_count;
        }
        if (record->deduction > 0.0) {
            ++deduction_count;
        }
        copy_text(effective_date, sizeof(effective_date),
                  record->status == PAY_PAID && record->pay_date[0] != '\0'
                      ? record->pay_date
                      : record->created_date);
        if (date_is_before_years(effective_date, 2)) {
            ++old_count;
        }
        record = record->next;
    }

    printf("\n【系统数据概览】\n");
    printf("  岗位记录：%d 个    所属部门：%d 个\n",
           app->job_count, department_count);
    printf("  薪资记录：%d 条    涉及学生：%d 名\n",
           app->salary_count, student_count);
    printf("  已发放：%d 条      未发放：%d 条\n", paid_count, unpaid_count);
    printf("  含扣款记录：%d 条  超过两年的记录：%d 条\n",
           deduction_count, old_count);
    printf("\n【测试数据要求核对】\n");
    print_requirement_check("岗位信息", app->job_count, 6);
    print_requirement_check("所属部门", department_count, 3);
    print_requirement_check("薪资记录", app->salary_count, 40);
    print_requirement_check("涉及学生", student_count, 25);
    print_requirement_check("已发放记录", paid_count, 15);
    print_requirement_check("未发放记录", unpaid_count, 15);
    print_requirement_check("扣除金额记录", deduction_count, 10);
    printf("\n  岗位链表头指针：%p；薪资链表头指针：%p（均为动态分配）\n",
           (const void *)app->job_head, (const void *)app->salary_head);
}
