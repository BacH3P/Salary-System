/*
 * 文件名称：query.h
 * 功能说明：薪资组合查询、统计与数据概览接口。
 */

#ifndef QUERY_H
#define QUERY_H

#include "common.h"

typedef struct SalaryFilter {
    char student_id[STUDENT_ID_LEN];
    char department[DEPT_NAME_LEN];
    char date_from[DATE_LEN];
    char date_to[DATE_LEN];
    char month[MONTH_LEN];
    int status;              /* -1 表示全部，PAY_PAID / PAY_UNPAID 表示筛选 */
    int has_month;
} SalaryFilter;

typedef struct SalarySummary {
    int count;
    int paid_count;
    int unpaid_count;
    int deduction_count;
    double gross_total;
    double deduction_total;
    double net_total;
    double paid_net_total;
} SalarySummary;

void salary_filter_init(SalaryFilter *filter);
int salary_matches(const Salary *record, const Job *jobs, const SalaryFilter *filter);
const Job *find_job_const(const Job *head, const char *job_id);

void query_print_salary_table(const AppData *app, const SalaryFilter *filter,
                              const char *title);
void query_by_student(AppData *app);
void query_by_department(AppData *app);
void query_by_pay_date(AppData *app);
void query_by_sort(AppData *app, int ascending);
void query_department_month_total(AppData *app);
void query_data_overview(AppData *app);

#endif /* QUERY_H */
