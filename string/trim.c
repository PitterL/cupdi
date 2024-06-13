#include <stdio.h>
#include <string.h>
#include <ctype.h>

// 自定义 trim 函数，用于去掉字符串两端的空白字符
char *trim(char *str)
{
    char *end;

    // 移除字符串开头的空白字符
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) // 全是空白字符的字符串
        return str;

    // 移除字符串结尾的空白字符
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // 在新结尾添加空字符
    *(end + 1) = '\0';

    return str;
}

// 自定义函数，用于检查字符是否在给定字符集中
int is_char_in_set(char ch, const char *char_set)
{
    while (*char_set)
    {
        if (*char_set == ch)
        {
            return 1;
        }
        char_set++;
    }
    return 0;
}

// 自定义 trim 函数，用于移除字符串两端的指定字符集中的字符
char *trim_chars(char *str, const char *char_set)
{
    char *end;

    // 移除字符串开头的指定字符
    while (is_char_in_set(*str, char_set))
        str++;

    if (*str == 0) // 全是指定字符的字符串
        return str;

    // 移除字符串结尾的指定字符
    end = str + strlen(str) - 1;
    while (end > str && is_char_in_set(*end, char_set))
        end--;

    // 在新结尾添加空字符
    *(end + 1) = '\0';

    return str;
}