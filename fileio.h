/*
 * 文件名称：fileio.h
 * 功能说明：文本文件持久化、全量备份与超期记录清理接口。
 */

#ifndef FILEIO_H
#define FILEIO_H

#include "common.h"

int fileio_init(AppData *app, const char *data_dir, char *error, size_t error_size);
int fileio_load_all(AppData *app, char *error, size_t error_size);
int fileio_save_all(AppData *app, char *error, size_t error_size);
int fileio_save_jobs(AppData *app, char *error, size_t error_size);
int fileio_save_salaries(AppData *app, char *error, size_t error_size);
int fileio_export_salary_all(AppData *app, char *error, size_t error_size);
int fileio_archive_old_records(AppData *app, int *removed_count,
                               char *error, size_t error_size);
int fileio_file_exists(const char *path);

#endif /* FILEIO_H */
