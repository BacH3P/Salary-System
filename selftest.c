/*
 * 文件名称：selftest.c
 * 功能说明：自动验证岗位/薪资链表、薪资计算、排序、文件持久化、
 *           备份与超期清理等核心功能，无需人工输入。
 */

#include "selftest.h"
#include "fileio.h"
#include "job.h"
#include "salary.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_tests = 0;
static int g_failures = 0;

static void check(int condition, const char *description)
{
    ++g_tests;
    if (condition) {
        printf("  [通过] %s\n", description);
    } else {
        ++g_failures;
        printf("  [失败] %s\n", description);
    }
}

static int near(double left, double right)
{
    return fabs(left - right) < 0.005;
}

int selftest_run(const char *data_dir)
{
    AppData app;
    char error[512];
    char actual_dir[PATH_LEN];
    Job *job1;
    Job *job2;
    Job *job3;
    Job *duplicate_job;
    Salary *salary1;
    Salary *salary2;
    Salary *salary3;
    Salary *salary_old;
    Salary *duplicate_salary;
    int removed = 0;

    if (data_dir == NULL || *data_dir == '\0') {
        copy_text(actual_dir, sizeof(actual_dir), "selftest_data");
    } else {
        copy_text(actual_dir, sizeof(actual_dir), data_dir);
    }

    printf("==============================================================\n");
    printf("  大学生勤工助学薪资发放管理系统 - 核心功能自测\n");
    printf("==============================================================\n");
    if (!fileio_init(&app, actual_dir, error, sizeof(error))) {
        printf("[失败] 初始化自测目录：%s\n", error);
        return 1;
    }
    app.no_clear = 1;
    file_remove_utf8(app.job_file);
    file_remove_utf8(app.salary_file);
    file_remove_utf8(app.backup_file);
    file_remove_utf8(app.backup_all_file);
    printf("自测数据目录：%s\n\n", app.data_dir);

    printf("【1】岗位结点与链表操作\n");
    check(job_validate("X001", "测试岗位", "测试部门", 20.0, 40.0, 5,
                       error, sizeof(error)) == 0,
          "非法岗位编号被拒绝");
    check(job_validate("J001", "测试岗位", "测试部门", 0.0, 40.0, 5,
                       error, sizeof(error)) == 0,
          "非法时薪（0）被拒绝");
    job1 = job_create("J001", "测试岗位A", "测试部门甲", 20.0, 40.0, 5);
    job2 = job_create("J002", "测试岗位B", "测试部门乙", 30.0, 20.0, 3);
    job3 = job_create("J003", "测试岗位C", "测试部门甲", 18.0, 10.0, 2);
    check(job1 != NULL && job2 != NULL && job3 != NULL, "动态分配岗位结点成功");
    check(job_add(&app.job_head, job1, error, sizeof(error)) == 1,
          "添加岗位 J001");
    check(job_add(&app.job_head, job2, error, sizeof(error)) == 1,
          "添加岗位 J002");
    check(job_add(&app.job_head, job3, error, sizeof(error)) == 1,
          "添加岗位 J003");
    app.job_count = job_list_count(app.job_head);
    check(app.job_count == 3, "岗位链表结点数为 3");
    duplicate_job = job_create("J001", "重复岗位", "测试部门", 10.0, 10.0, 1);
    check(duplicate_job != NULL &&
              job_add(&app.job_head, duplicate_job, error, sizeof(error)) == 0,
          "重复岗位编号被拒绝");
    free(duplicate_job);

    printf("\n【2】薪资计算与合法性校验\n");
    salary1 = salary_create(1, job1, "S001", "张三", "2025-10", 30.0, 50.0,
                            "迟到2次，扣款50元", PAY_UNPAID, "", "2025-10-30");
    salary2 = salary_create(2, job1, "S002", "李四", "2025-10", 40.0, 0.0,
                            "", PAY_UNPAID, "", "2025-10-30");
    salary3 = salary_create(3, job2, "S003", "王五", "2025-10", 10.0, 20.0,
                            "任务未完成，扣款20元", PAY_PAID, "2025-11-08",
                            "2025-10-30");
    check(salary1 != NULL && near(salary1->gross_pay, 600.00) &&
              near(salary1->net_pay, 550.00),
          "应发 600.00、实发 550.00 计算正确");
    check(salary3 != NULL && near(salary3->gross_pay, 300.00) &&
              near(salary3->net_pay, 280.00),
          "时薪 30 × 10 小时、扣 20 元计算正确");
    check(salary_validate(job1, "S009", "测试", "2025-10", 41.0, 0.0, "",
                          PAY_UNPAID, "", error, sizeof(error)) == 0,
          "工作时长超过岗位上限被拒绝");
    check(salary_validate(job1, "S009", "测试", "2025-10", 30.0, 601.0,
                          "测试", PAY_UNPAID, "", error, sizeof(error)) == 0,
          "扣除金额超过应发薪资被拒绝");
    check(salary_validate(job1, "S009", "测试", "2025-10", 30.0, 50.0, "",
                          PAY_UNPAID, "", error, sizeof(error)) == 0,
          "有扣款但未填写原因被拒绝");
    check(salary_validate(job1, "S009", "测试", "2025-10", 30.0, 0.0, "",
                          PAY_PAID, "", error, sizeof(error)) == 0,
          "已发放但无发放日期被拒绝");

    printf("\n【3】薪资链表增删与重复校验\n");
    check(salary_add(&app.salary_head, salary1, error, sizeof(error)) == 1,
          "添加薪资记录 1");
    check(salary_add(&app.salary_head, salary2, error, sizeof(error)) == 1,
          "添加薪资记录 2");
    check(salary_add(&app.salary_head, salary3, error, sizeof(error)) == 1,
          "添加薪资记录 3");
    app.salary_count = salary_list_count(app.salary_head);
    app.next_salary_id = salary_next_id(app.salary_head);
    check(app.salary_count == 3, "薪资链表结点数为 3");
    duplicate_salary = salary_create(4, job1, "S001", "张三", "2025-10",
                                     20.0, 0.0, "", PAY_UNPAID, "",
                                     "2025-10-30");
    check(duplicate_salary != NULL &&
              salary_add(&app.salary_head, duplicate_salary, error,
                         sizeof(error)) == 0,
          "同一学生/岗位/月份重复记录被拒绝");
    free(duplicate_salary);

    printf("\n【4】按实发薪资排序\n");
    salary_sort_by_net(&app.salary_head, 1);
    check(near(app.salary_head->net_pay, 280.00) &&
              near(app.salary_head->next->net_pay, 550.00) &&
              near(app.salary_head->next->next->net_pay, 800.00),
          "升序排序结果 280 → 550 → 800");
    salary_sort_by_net(&app.salary_head, 0);
    check(near(app.salary_head->net_pay, 800.00) &&
              near(app.salary_head->next->next->net_pay, 280.00),
          "降序排序结果 800 → 550 → 280");

    printf("\n【5】发放状态更新\n");
    check(salary_update_status(salary2, PAY_PAID, "2025-11-09",
                               error, sizeof(error)) == 1 &&
              salary2->status == PAY_PAID &&
              strcmp(salary2->pay_date, "2025-11-09") == 0,
          "未发放记录可标记为已发放");
    check(salary_update_status(salary2, PAY_UNPAID, "",
                               error, sizeof(error)) == 1 &&
              salary2->status == PAY_UNPAID && salary2->pay_date[0] == '\0',
          "已发放记录可撤销为未发放");

    printf("\n【6】文件保存、加载与全量备份\n");
    check(fileio_save_all(&app, error, sizeof(error)) == 1,
          "岗位与薪资链表保存为文本文件");
    check(fileio_file_exists(app.job_file) && fileio_file_exists(app.salary_file),
          "jobs.txt 与 salary.txt 均已生成");
    check(fileio_export_salary_all(&app, error, sizeof(error)) == 1 &&
              fileio_file_exists(app.backup_all_file),
          "全部薪资数据备份到 salary_all.txt");
    check(fileio_load_all(&app, error, sizeof(error)) == 1 &&
              app.job_count == 3 && app.salary_count == 3,
          "从文本文件重新加载后记录数一致");
    check(salary_find_by_id(app.salary_head, 1) != NULL &&
              near(salary_find_by_id(app.salary_head, 1)->net_pay, 550.00),
          "重新加载后金额与链表关系正确");

    printf("\n【7】岗位引用完整性与删除\n");
    check(job_delete(&app.job_head, "J001", app.salary_head,
                     error, sizeof(error)) == 0,
          "被薪资记录引用的岗位禁止删除");
    check(job_delete(&app.job_head, "J003", app.salary_head,
                     error, sizeof(error)) == 1,
          "未被引用的岗位可以删除");
    check(job_list_count(app.job_head) == 2, "删除后岗位链表结点数为 2");

    printf("\n【8】超过两年记录归档清理\n");
    job2 = job_find_by_id(app.job_head, "J002");
    check(job2 != NULL, "重新加载后仍可定位岗位 J002");
    salary_old = salary_create(app.next_salary_id, job2, "S004", "赵六",
                               "2023-09", 10.0, 0.0, "", PAY_PAID,
                               "2023-10-08", "2023-09-30");
    check(salary_old != NULL &&
              salary_add(&app.salary_head, salary_old, error, sizeof(error)) == 1,
          "添加一条 2023 年历史记录");
    app.salary_count = salary_list_count(app.salary_head);
    check(date_is_before_years("2023-09-30", 2) == 1,
          "日期比较判定 2023 年记录已超过两年");
    check(fileio_save_all(&app, error, sizeof(error)) == 1,
          "清理前保存数据");
    check(fileio_archive_old_records(&app, &removed, error, sizeof(error)) == 1 &&
              removed == 1,
          "超期记录归档并删除 1 条");
    check(app.salary_count == 3 && salary_find_by_id(app.salary_head, 4) == NULL,
          "清理后剩余 3 条且历史记录已移除");
    check(fileio_file_exists(app.backup_file), "归档文件 salary_backup.txt 已生成");

    printf("\n【9】最终数据一致性\n");
    check(fileio_load_all(&app, error, sizeof(error)) == 1,
          "清理后再次从文件加载成功");
    check(app.job_count == 2 && app.salary_count == 3,
          "最终岗位 2 个、薪资 3 条");
    app.next_salary_id = salary_next_id(app.salary_head);
    check(app.next_salary_id == 4, "下一条记录编号计算正确（4）");

    job_list_clear(&app.job_head);
    salary_list_clear(&app.salary_head);
    file_remove_utf8(app.job_file);
    file_remove_utf8(app.salary_file);
    file_remove_utf8(app.backup_file);
    file_remove_utf8(app.backup_all_file);

    printf("\n==============================================================\n");
    printf("  自测完成：共 %d 项，通过 %d 项，失败 %d 项。\n",
           g_tests, g_tests - g_failures, g_failures);
    printf("  结论：%s\n", g_failures == 0 ? "全部核心功能测试通过。" : "存在失败项，请检查。");
    printf("==============================================================\n");
    return g_failures == 0 ? 0 : 1;
}
