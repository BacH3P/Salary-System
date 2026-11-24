/*
 * 文件名称：input.h
 * 功能说明：带合法性校验的交互输入函数声明。
 */

#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

void input_set_echo(int enabled);
int input_text(const char *prompt, char *out, size_t out_size, int allow_empty);
int input_text_default(const char *prompt, const char *current, char *out, size_t out_size);
int input_int_range(const char *prompt, int min_value, int max_value, int *out);
int input_long_range(const char *prompt, long min_value, long max_value, long *out);
int input_double_range(const char *prompt, double min_value, double max_value, double *out);
int input_date_value(const char *prompt, int allow_empty, int default_today, char *out, size_t out_size);
int input_month_value(const char *prompt, int default_current, char *out, size_t out_size);
int input_confirm(const char *prompt, int default_yes);
int input_password(const char *prompt, char *out, size_t out_size);

#endif /* INPUT_H */
