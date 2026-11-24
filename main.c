/*
 * 文件名称：main.c
 * 功能说明：程序入口。解析命令行参数，初始化数据目录，加载链表，
 *           完成管理员登录后进入主菜单；支持 --selftest 自动测试。
 */

#include "common.h"
#include "fileio.h"
#include "input.h"
#include "job.h"
#include "menu.h"
#include "query.h"
#include "salary.h"
#include "selftest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(void)
{
    printf("%s v%s\n", APP_NAME, APP_VERSION);
    printf("用法：SalarySystem.exe [选项]\n\n");
    printf("  --data-dir <目录>  指定数据目录（默认使用程序所在目录下的 data）\n");
    printf("  --no-clear         不自动清屏，便于保留运行记录\n");
    printf("  --selftest         运行内置核心功能自测后退出\n");
    printf("  --overview         加载数据并输出数据概览后退出\n");
    printf("  --help             显示本帮助\n\n");
    printf("管理员账号：admin    默认密码：admin123\n");
}

int main(int argc, char *argv[])
{
    AppData app;
    char error[512];
    char data_dir[PATH_LEN] = "";
    int selftest = 0;
    int overview = 0;
    int no_clear = 0;
    int i;

    console_init();
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--data-dir") == 0) {
            if (i + 1 >= argc) {
                print_error("--data-dir 后面必须提供一个目录路径。");
                return 1;
            }
            copy_text(data_dir, sizeof(data_dir), argv[++i]);
        } else if (strcmp(argv[i], "--no-clear") == 0) {
            no_clear = 1;
        } else if (strcmp(argv[i], "--selftest") == 0) {
            selftest = 1;
        } else if (strcmp(argv[i], "--overview") == 0) {
            overview = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else {
            print_error("未知参数：%s", argv[i]);
            print_usage();
            return 1;
        }
    }

    if (selftest) {
        return selftest_run(data_dir[0] != '\0' ? data_dir : NULL);
    }

    input_set_echo(!console_is_interactive());
    if (!fileio_init(&app, data_dir[0] != '\0' ? data_dir : NULL,
                     error, sizeof(error))) {
        print_error("%s", error);
        return 1;
    }
    app.no_clear = no_clear;
    app.echo_input = !console_is_interactive();

    if (!fileio_load_all(&app, error, sizeof(error))) {
        print_error("%s", error);
        job_list_clear(&app.job_head);
        salary_list_clear(&app.salary_head);
        return 1;
    }

    if (overview) {
        app_print_banner();
        query_data_overview(&app);
        job_list_clear(&app.job_head);
        salary_list_clear(&app.salary_head);
        return 0;
    }

    if (!menu_login(&app)) {
        job_list_clear(&app.job_head);
        salary_list_clear(&app.salary_head);
        return 0;
    }
    menu_main(&app);
    job_list_clear(&app.job_head);
    salary_list_clear(&app.salary_head);
    return 0;
}
