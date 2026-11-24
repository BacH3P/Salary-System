/*
 * 文件名称：menu.c
 * 功能说明：实现管理员登录、主菜单、岗位管理、薪资管理、查询统计和数据维护界面。
 *           所有修改操作完成后立即持久化，避免意外退出造成数据丢失。
 */

#include "menu.h"
#include "fileio.h"
#include "input.h"
#include "job.h"
#include "query.h"
#include "salary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void save_after_change(AppData *app, const char *action)
{
    char error[512];

    app->dirty = 1;
    if (fileio_save_all(app, error, sizeof(error))) {
        print_success("%s，数据已保存到文件。", action);
    } else {
        print_error("%s，但保存失败：%s", action, error);
        print_info("修改仍保留在内存中，请在主菜单选择“保存并退出”重试。");
    }
}

static int prompt_job(AppData *app, Job **out_job)
{
    char job_id[JOB_ID_LEN];

    if (app->job_head == NULL) {
        print_info("岗位链表为空，请先添加岗位。");
        return 0;
    }
    if (!input_text("请输入岗位编号：", job_id, sizeof(job_id), 0)) {
        return 0;
    }
    *out_job = job_find_by_id(app->job_head, job_id);
    if (*out_job == NULL) {
        print_error("未找到岗位编号“%s”。", job_id);
        return 0;
    }
    return 1;
}

static int prompt_salary(AppData *app, Salary **out_record)
{
    long id;

    if (app->salary_head == NULL) {
        print_info("薪资链表为空，请先录入薪资记录。");
        return 0;
    }
    if (!input_long_range("请输入薪资记录编号：", 1, 2147483647L, &id)) {
        return 0;
    }
    *out_record = salary_find_by_id(app->salary_head, id);
    if (*out_record == NULL) {
        print_error("未找到记录编号 %ld。", id);
        return 0;
    }
    return 1;
}

int menu_login(AppData *app)
{
    int attempt;
    char username[32];
    char password[64];

    app_clear_screen(app);
    app_print_banner();
    app_print_section("管理员登录（仅管理员可操作系统）");
    printf("  默认账号：admin\n");
    printf("  默认密码：admin123\n\n");

    for (attempt = 1; attempt <= 3; ++attempt) {
        printf("第 %d/3 次登录\n", attempt);
        if (!input_text("用户名：", username, sizeof(username), 0)) {
            return 0;
        }
        if (!input_password("密  码：", password, sizeof(password))) {
            return 0;
        }
        if (strcmp(username, "admin") == 0 && strcmp(password, "admin123") == 0) {
            print_success("登录成功，欢迎使用本系统！");
            app_pause(app);
            return 1;
        }
        print_error("用户名或密码错误，请重新输入。");
    }
    print_error("连续 3 次登录失败，程序退出。");
    return 0;
}

static void job_menu_add(AppData *app)
{
    char id[JOB_ID_LEN];
    char name[JOB_NAME_LEN];
    char department[DEPT_NAME_LEN];
    double wage;
    double cap;
    int recruit;
    char error[256];
    Job *node;

    app_print_section("添加岗位信息");
    if (!input_text("岗位编号（格式 J+数字，如 J009）：", id, sizeof(id), 0)) {
        return;
    }
    if (job_find_by_id(app->job_head, id) != NULL) {
        print_error("岗位编号“%s”已存在。", id);
        return;
    }
    if (!input_text("岗位名称：", name, sizeof(name), 0)) {
        return;
    }
    if (!input_text("所属部门：", department, sizeof(department), 0)) {
        return;
    }
    if (!input_double_range("时薪（元/小时，0.01-1000）：", 0.01, MAX_HOURLY_WAGE, &wage)) {
        return;
    }
    if (!input_double_range("每月工作时长上限（小时，0.5-744）：", 0.5,
                            MAX_MONTH_HOURS, &cap)) {
        return;
    }
    if (!input_int_range("招聘人数（1-9999）：", 1, 9999, &recruit)) {
        return;
    }
    if (!job_validate(id, name, department, wage, cap, recruit,
                      error, sizeof(error))) {
        print_error("%s", error);
        return;
    }
    node = job_create(id, name, department, wage, cap, recruit);
    if (node == NULL) {
        print_error("内存不足，岗位结点创建失败。");
        return;
    }
    printf("\n即将添加的岗位：\n");
    job_print_detail(node);
    if (!input_confirm("确认添加该岗位？(y/n)：", 1)) {
        free(node);
        print_info("已取消添加。");
        return;
    }
    if (!job_add(&app->job_head, node, error, sizeof(error))) {
        free(node);
        print_error("%s", error);
        return;
    }
    ++app->job_count;
    save_after_change(app, "岗位添加成功");
}

static void job_menu_modify(AppData *app)
{
    Job *job;
    char name[JOB_NAME_LEN];
    char department[DEPT_NAME_LEN];
    double wage;
    double cap;
    int recruit;
    char error[256];

    app_print_section("修改岗位信息");
    if (!prompt_job(app, &job)) {
        return;
    }
    printf("\n当前岗位信息：\n");
    job_print_detail(job);
    printf("\n");
    if (!input_text_default("岗位名称", job->name, name, sizeof(name))) {
        return;
    }
    if (!input_text_default("所属部门", job->department, department,
                            sizeof(department))) {
        return;
    }
    if (!input_double_range("新的时薪（元/小时，0.01-1000）：", 0.01,
                            MAX_HOURLY_WAGE, &wage)) {
        return;
    }
    if (!input_double_range("新的月工作时长上限（0.5-744）：", 0.5,
                            MAX_MONTH_HOURS, &cap)) {
        return;
    }
    if (!input_int_range("新的招聘人数（1-9999）：", 1, 9999, &recruit)) {
        return;
    }
    if (!job_update(job, name, department, wage, cap, recruit,
                    error, sizeof(error))) {
        print_error("%s", error);
        return;
    }
    save_after_change(app, "岗位修改成功");
}

static void job_menu_delete(AppData *app)
{
    Job *job;
    char error[256];

    app_print_section("删除岗位信息");
    if (!prompt_job(app, &job)) {
        return;
    }
    printf("\n待删除岗位：\n");
    job_print_detail(job);
    if (!input_confirm("删除后不可恢复，确认删除？(y/n)：", 0)) {
        print_info("已取消删除。");
        return;
    }
    if (!job_delete(&app->job_head, job->id, app->salary_head,
                    error, sizeof(error))) {
        print_error("%s", error);
        return;
    }
    --app->job_count;
    save_after_change(app, "岗位删除成功");
}

static void job_menu_query_department(AppData *app)
{
    char department[DEPT_NAME_LEN];
    app_print_section("按部门查询岗位");
    if (!input_text("请输入部门名称（支持部分匹配）：", department,
                    sizeof(department), 0)) {
        return;
    }
    job_query_by_department(app->job_head, department);
}

static void job_menu_query_name(AppData *app)
{
    char name[JOB_NAME_LEN];
    app_print_section("按岗位名称查询");
    if (!input_text("请输入岗位名称关键字（支持部分匹配）：", name,
                    sizeof(name), 0)) {
        return;
    }
    job_query_by_name(app->job_head, name);
}

static void menu_job_management(AppData *app)
{
    int choice;

    for (;;) {
        app_clear_screen(app);
        app_print_banner();
        app_print_section("岗位管理");
        printf("  1. 浏览全部岗位\n");
        printf("  2. 添加岗位\n");
        printf("  3. 修改岗位（编号不可改）\n");
        printf("  4. 删除岗位（被薪资记录引用时禁止删除）\n");
        printf("  5. 按部门查询岗位\n");
        printf("  6. 按名称查询岗位\n");
        printf("  0. 返回主菜单\n");
        if (!input_int_range("请选择操作：", 0, 6, &choice)) {
            return;
        }
        switch (choice) {
        case 1:
            app_print_section("全部岗位列表");
            job_print_table(app->job_head);
            app_pause(app);
            break;
        case 2:
            job_menu_add(app);
            app_pause(app);
            break;
        case 3:
            job_menu_modify(app);
            app_pause(app);
            break;
        case 4:
            job_menu_delete(app);
            app_pause(app);
            break;
        case 5:
            job_menu_query_department(app);
            app_pause(app);
            break;
        case 6:
            job_menu_query_name(app);
            app_pause(app);
            break;
        default:
            return;
        }
    }
}

static void salary_menu_add(AppData *app)
{
    char job_id[JOB_ID_LEN];
    char student_id[STUDENT_ID_LEN];
    char student_name[STUDENT_NAME_LEN];
    char month[MONTH_LEN];
    char month_prompt[128];
    char hours_prompt[128];
    char deduction_prompt[128];
    char reason[REASON_LEN];
    char today[DATE_LEN];
    char error[256];
    double hours;
    double deduction;
    double gross;
    double net;
    int status = PAY_UNPAID;
    Job *job;
    Salary *node;

    app_print_section("录入薪资记录");
    if (app->job_head == NULL) {
        print_info("岗位链表为空，请先在“岗位管理”中添加岗位。");
        return;
    }
    job_print_table(app->job_head);
    if (!input_text("请输入岗位编号：", job_id, sizeof(job_id), 0)) {
        return;
    }
    job = job_find_by_id(app->job_head, job_id);
    if (job == NULL) {
        print_error("未找到岗位编号“%s”。", job_id);
        return;
    }
    if (!input_text("请输入学号：", student_id, sizeof(student_id), 0)) {
        return;
    }
    if (!input_text("请输入学生姓名：", student_name, sizeof(student_name), 0)) {
        return;
    }
    month_today(month, sizeof(month));
    snprintf(month_prompt, sizeof(month_prompt),
             "薪资月份（YYYY-MM，回车默认 %s）：", month);
    if (!input_month_value(month_prompt, 1, month, sizeof(month))) {
        return;
    }
    printf("岗位“%s”时薪 %.2f 元/小时，月工作时长上限 %.1f 小时。\n",
           job->name, job->hourly_wage, job->monthly_hour_cap);
    snprintf(hours_prompt, sizeof(hours_prompt),
             "当月工作时长（0.1-%.1f 小时）：", job->monthly_hour_cap);
    if (!input_double_range(hours_prompt, 0.1, job->monthly_hour_cap, &hours)) {
        return;
    }
    gross = salary_calc_gross(job, hours);
    printf("应发薪资 = %.2f 元/小时 × %.1f 小时 = %.2f 元。\n",
           job->hourly_wage, hours, gross);
    snprintf(deduction_prompt, sizeof(deduction_prompt),
             "扣除金额（0-%.2f 元，0 表示无扣款）：", gross);
    if (!input_double_range(deduction_prompt, 0.0, gross, &deduction)) {
        return;
    }
    reason[0] = '\0';
    if (deduction > 0.0) {
        if (!input_text("扣除原因（如“迟到2次，扣款50元”）：", reason,
                        sizeof(reason), 0)) {
            return;
        }
    }
    if (!salary_validate(job, student_id, student_name, month, hours, deduction,
                         reason, status, "", error, sizeof(error))) {
        print_error("%s", error);
        return;
    }
    net = salary_calc_net(gross, deduction);
    printf("\n薪资计算明细：\n");
    printf("  应发薪资：%.2f 元\n", gross);
    printf("  扣除金额：%.2f 元%s%s\n", deduction,
           deduction > 0.0 ? "，原因：" : "", deduction > 0.0 ? reason : "");
    printf("  实发薪资：%.2f - %.2f = %.2f 元\n", gross, deduction, net);
    printf("  发放状态：未发放（可在“更新发放状态”中办理）\n");
    if (!input_confirm("确认保存该薪资记录？(y/n)：", 1)) {
        print_info("已取消录入。");
        return;
    }
    date_today(today, sizeof(today));
    node = salary_create(app->next_salary_id, job, student_id, student_name,
                         month, hours, deduction, reason, status, "", today);
    if (node == NULL) {
        print_error("内存不足，薪资结点创建失败。");
        return;
    }
    if (!salary_add(&app->salary_head, node, error, sizeof(error))) {
        free(node);
        print_error("%s", error);
        return;
    }
    ++app->salary_count;
    app->next_salary_id = salary_next_id(app->salary_head);
    save_after_change(app, "薪资记录添加成功");
}

static void salary_menu_modify(AppData *app)
{
    Salary *record;
    Job *job;
    char hours_prompt[128];
    char reason[REASON_LEN];
    double hours;
    double deduction;
    char error[256];

    app_print_section("修改未发放薪资记录");
    if (!prompt_salary(app, &record)) {
        return;
    }
    if (record->status == PAY_PAID) {
        print_error("记录 %ld 已发放，不能直接修改金额；请先撤销发放状态。", record->id);
        return;
    }
    job = job_find_by_id(app->job_head, record->job_id);
    if (job == NULL) {
        print_error("记录关联的岗位不存在，数据异常。");
        return;
    }
    printf("\n当前记录：\n");
    salary_print_detail(record, job);
    printf("\n");
    snprintf(hours_prompt, sizeof(hours_prompt),
             "新的工作时长（0.1-%.1f 小时）：", job->monthly_hour_cap);
    if (!input_double_range(hours_prompt, 0.1, job->monthly_hour_cap, &hours)) {
        return;
    }
    if (!input_double_range("新的扣除金额（0 表示无扣款）：", 0.0,
                            salary_calc_gross(job, hours), &deduction)) {
        return;
    }
    reason[0] = '\0';
    if (deduction > 0.0) {
        if (!input_text_default("扣除原因", record->deduction_reason,
                                reason, sizeof(reason))) {
            return;
        }
    }
    if (!salary_validate(job, record->student_id, record->student_name,
                         record->salary_month, hours, deduction, reason,
                         PAY_UNPAID, "", error, sizeof(error))) {
        print_error("%s", error);
        return;
    }
    printf("修改后应发 %.2f 元，实发 %.2f 元。\n",
           salary_calc_gross(job, hours),
           salary_calc_net(salary_calc_gross(job, hours), deduction));
    if (!input_confirm("确认保存修改？(y/n)：", 1)) {
        print_info("已取消修改。");
        return;
    }
    if (!salary_update(record, job, hours, deduction, reason, PAY_UNPAID, "",
                       error, sizeof(error))) {
        print_error("%s", error);
        return;
    }
    save_after_change(app, "薪资记录修改成功");
}

static void salary_menu_status(AppData *app)
{
    Salary *record;
    Job *job;
    char date[DATE_LEN];
    char today[DATE_LEN];
    char error[256];

    app_print_section("更新薪资发放状态");
    if (!prompt_salary(app, &record)) {
        return;
    }
    job = job_find_by_id(app->job_head, record->job_id);
    printf("\n记录详情：\n");
    salary_print_detail(record, job);
    printf("\n");
    if (record->status == PAY_UNPAID) {
        date_today(today, sizeof(today));
        if (!input_date_value("发放日期（YYYY-MM-DD，回车默认今天）：", 0, 1,
                              date, sizeof(date))) {
            return;
        }
        if (date_compare(date, today) > 0) {
            print_error("发放日期不能晚于今天（%s）。", today);
            return;
        }
        if (!input_confirm("确认将该记录标记为“已发放”？(y/n)：", 1)) {
            print_info("已取消操作。");
            return;
        }
        if (!salary_update_status(record, PAY_PAID, date, error, sizeof(error))) {
            print_error("%s", error);
            return;
        }
        save_after_change(app, "发放状态已更新为“已发放”");
    } else {
        if (!input_confirm("确认撤销发放并改回“未发放”？(y/n)：", 0)) {
            print_info("已取消操作。");
            return;
        }
        if (!salary_update_status(record, PAY_UNPAID, "", error, sizeof(error))) {
            print_error("%s", error);
            return;
        }
        save_after_change(app, "发放状态已更新为“未发放”");
    }
}

static void salary_menu_delete(AppData *app)
{
    Salary *record;
    Job *job;
    char error[256];

    app_print_section("删除薪资记录");
    if (!prompt_salary(app, &record)) {
        return;
    }
    job = job_find_by_id(app->job_head, record->job_id);
    printf("\n待删除记录：\n");
    salary_print_detail(record, job);
    if (!input_confirm("删除后不可恢复，确认删除？(y/n)：", 0)) {
        print_info("已取消删除。");
        return;
    }
    if (!salary_delete(&app->salary_head, record->id, error, sizeof(error))) {
        print_error("%s", error);
        return;
    }
    --app->salary_count;
    app->next_salary_id = salary_next_id(app->salary_head);
    save_after_change(app, "薪资记录删除成功");
}

static void menu_salary_management(AppData *app)
{
    int choice;

    for (;;) {
        app_clear_screen(app);
        app_print_banner();
        app_print_section("薪资管理");
        printf("  1. 浏览全部薪资记录\n");
        printf("  2. 录入薪资记录（自动计算应发/实发）\n");
        printf("  3. 修改未发放薪资记录\n");
        printf("  4. 更新发放状态\n");
        printf("  5. 删除薪资记录\n");
        printf("  0. 返回主菜单\n");
        if (!input_int_range("请选择操作：", 0, 5, &choice)) {
            return;
        }
        switch (choice) {
        case 1: {
            SalaryFilter filter;
            salary_filter_init(&filter);
            app_print_section("全部薪资记录");
            query_print_salary_table(app, &filter, "全部薪资记录");
            app_pause(app);
            break;
        }
        case 2:
            salary_menu_add(app);
            app_pause(app);
            break;
        case 3:
            salary_menu_modify(app);
            app_pause(app);
            break;
        case 4:
            salary_menu_status(app);
            app_pause(app);
            break;
        case 5:
            salary_menu_delete(app);
            app_pause(app);
            break;
        default:
            return;
        }
    }
}

static void menu_query_statistics(AppData *app)
{
    int choice;

    for (;;) {
        app_clear_screen(app);
        app_print_banner();
        app_print_section("查询与统计");
        printf("  1. 按学号查询薪资记录\n");
        printf("  2. 按部门查询薪资记录\n");
        printf("  3. 按发放时间查询薪资记录\n");
        printf("  4. 按实发薪资升序排列\n");
        printf("  5. 按实发薪资降序排列\n");
        printf("  6. 统计某部门当月薪资支出\n");
        printf("  7. 数据概览与测试数据要求核对\n");
        printf("  0. 返回主菜单\n");
        if (!input_int_range("请选择操作：", 0, 7, &choice)) {
            return;
        }
        switch (choice) {
        case 1:
            app_print_section("按学号查询");
            query_by_student(app);
            app_pause(app);
            break;
        case 2:
            app_print_section("按部门查询");
            query_by_department(app);
            app_pause(app);
            break;
        case 3:
            app_print_section("按发放时间查询");
            query_by_pay_date(app);
            app_pause(app);
            break;
        case 4:
            app_print_section("按实发薪资升序排序");
            query_by_sort(app, 1);
            app->dirty = 1;
            app_pause(app);
            break;
        case 5:
            app_print_section("按实发薪资降序排序");
            query_by_sort(app, 0);
            app->dirty = 1;
            app_pause(app);
            break;
        case 6:
            app_print_section("部门月度薪资统计");
            query_department_month_total(app);
            app_pause(app);
            break;
        case 7:
            app_print_section("数据概览");
            query_data_overview(app);
            app_pause(app);
            break;
        default:
            return;
        }
    }
}

static void show_data_files(const AppData *app)
{
    printf("  数据目录：%s\n", app->data_dir);
    printf("  岗位文件：%s\n", app->job_file);
    printf("  薪资文件：%s\n", app->salary_file);
    printf("  全量备份：%s\n", app->backup_all_file);
    printf("  超期归档：%s\n", app->backup_file);
}

static void menu_data_maintenance(AppData *app)
{
    int choice;
    char error[512];

    for (;;) {
        app_clear_screen(app);
        app_print_banner();
        app_print_section("数据维护");
        printf("  1. 备份全部薪资数据到 salary_all.txt\n");
        printf("  2. 清理超过两年的记录并归档到 salary_backup.txt\n");
        printf("  3. 查看数据文件位置与记录数量\n");
        printf("  4. 重新从文件加载数据\n");
        printf("  0. 返回主菜单\n");
        if (!input_int_range("请选择操作：", 0, 4, &choice)) {
            return;
        }
        switch (choice) {
        case 1:
            app_print_section("全量备份");
            if (fileio_export_salary_all(app, error, sizeof(error))) {
                print_success("全部薪资数据已备份到：%s", app->backup_all_file);
                printf("  备份记录数：%d 条\n", app->salary_count);
            } else {
                print_error("%s", error);
            }
            app_pause(app);
            break;
        case 2: {
            int removed = 0;
            char today[DATE_LEN];
            char cutoff[DATE_LEN];
            date_today(today, sizeof(today));
            date_minus_years(today, 2, cutoff, sizeof(cutoff));
            app_print_section("清理超过两年的薪资记录");
            printf("  今天日期：%s\n", today);
            printf("  清理截止：早于 %s 的记录（超过两年）\n", cutoff);
            printf("  清理前会先追加写入 salary_backup.txt，再删除主文件中的记录。\n");
            if (!input_confirm("是否继续？(y/n)：", 0)) {
                print_info("已取消操作。");
                app_pause(app);
                break;
            }
            if (fileio_archive_old_records(app, &removed, error, sizeof(error))) {
                if (removed > 0) {
                    print_success("已归档并删除 %d 条超期记录。", removed);
                    printf("  归档文件：%s\n", app->backup_file);
                    printf("  剩余薪资记录：%d 条\n", app->salary_count);
                }
            } else {
                print_error("%s", error);
            }
            app_pause(app);
            break;
        }
        case 3:
            app_print_section("数据文件信息");
            printf("  岗位记录：%d 个\n", app->job_count);
            printf("  薪资记录：%d 条\n", app->salary_count);
            show_data_files(app);
            app_pause(app);
            break;
        case 4:
            app_print_section("重新加载文件数据");
            printf("  将从磁盘重新读取岗位和薪资数据。\n");
            if (!input_confirm("确认重新加载？(y/n)：", 0)) {
                print_info("已取消。");
                app_pause(app);
                break;
            }
            if (fileio_load_all(app, error, sizeof(error))) {
                print_success("重新加载完成：岗位 %d 个，薪资 %d 条。",
                              app->job_count, app->salary_count);
            } else {
                print_error("%s", error);
            }
            app_pause(app);
            break;
        default:
            return;
        }
    }
}

void menu_main(AppData *app)
{
    int choice;
    char error[512];

    for (;;) {
        app_clear_screen(app);
        app_print_banner();
        printf("  当前数据：岗位 %d 个 | 薪资记录 %d 条\n", app->job_count,
               app->salary_count);
        printf("  数据目录：%s\n", app->data_dir);
        app_print_section("主菜单");
        printf("  1. 岗位管理\n");
        printf("  2. 薪资管理\n");
        printf("  3. 查询与统计\n");
        printf("  4. 数据维护\n");
        printf("  0. 保存并退出\n");
        if (!input_int_range("请选择操作：", 0, 4, &choice)) {
            choice = 0;
        }
        switch (choice) {
        case 1:
            menu_job_management(app);
            break;
        case 2:
            menu_salary_management(app);
            break;
        case 3:
            menu_query_statistics(app);
            break;
        case 4:
            menu_data_maintenance(app);
            break;
        default:
            if (fileio_save_all(app, error, sizeof(error))) {
                print_success("数据已保存：岗位 %d 个，薪资记录 %d 条。",
                              app->job_count, app->salary_count);
                print_info("感谢使用，再见！");
                return;
            }
            print_error("退出前保存失败：%s", error);
            if (!input_confirm("仍要退出吗？未保存的修改将丢失。(y/n)：", 0)) {
                continue;
            }
            return;
        }
    }
}
