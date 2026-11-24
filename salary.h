/*
 * 文件名称：salary.h
 * 功能说明：薪资记录动态链表、薪资计算与排序接口。
 */

#ifndef SALARY_H
#define SALARY_H

#include "common.h"

void salary_list_clear(Salary **head);
int salary_list_count(const Salary *head);
long salary_next_id(const Salary *head);

Salary *salary_find_by_id(Salary *head, long id);
Salary *salary_create(long id, const Job *job, const char *student_id,
                      const char *student_name, const char *salary_month,
                      double work_hours, double deduction,
                      const char *deduction_reason, int status,
                      const char *pay_date, const char *created_date);

double salary_calc_gross(const Job *job, double work_hours);
double salary_calc_net(double gross_pay, double deduction);

int salary_validate(const Job *job, const char *student_id, const char *student_name,
                    const char *salary_month, double work_hours,
                    double deduction, const char *deduction_reason,
                    int status, const char *pay_date,
                    char *error, size_t error_size);

int salary_add(Salary **head, Salary *node, char *error, size_t error_size);
int salary_update(Salary *record, const Job *job, double work_hours,
                  double deduction, const char *deduction_reason,
                  int status, const char *pay_date,
                  char *error, size_t error_size);
int salary_delete(Salary **head, long id, char *error, size_t error_size);
int salary_update_status(Salary *record, int status, const char *pay_date,
                         char *error, size_t error_size);

void salary_sort_by_net(Salary **head, int ascending);
void salary_print_detail(const Salary *record, const Job *job);

#endif /* SALARY_H */
