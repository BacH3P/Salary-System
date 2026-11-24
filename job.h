/*
 * 文件名称：job.h
 * 功能说明：岗位动态链表操作接口。
 */

#ifndef JOB_H
#define JOB_H

#include "common.h"

void job_list_clear(Job **head);
int job_list_count(const Job *head);

Job *job_create(const char *id, const char *name, const char *department,
                double hourly_wage, double monthly_hour_cap, int recruit_count);
Job *job_find_by_id(Job *head, const char *id);

int job_validate(const char *id, const char *name, const char *department,
                 double hourly_wage, double monthly_hour_cap, int recruit_count,
                 char *error, size_t error_size);

int job_add(Job **head, Job *node, char *error, size_t error_size);
int job_update(Job *job, const char *name, const char *department,
               double hourly_wage, double monthly_hour_cap, int recruit_count,
               char *error, size_t error_size);
int job_delete(Job **head, const char *id, const Salary *salaries,
               char *error, size_t error_size);

void job_print_table(const Job *head);
void job_print_detail(const Job *job);
void job_query_by_department(const Job *head, const char *department);
void job_query_by_name(const Job *head, const char *name);

#endif /* JOB_H */
