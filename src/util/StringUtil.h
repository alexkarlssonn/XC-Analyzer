
#pragma once


/* ---------------------------------------------------
 * General string utility functions
 * -------------------------------------------------- */
int getStringSize(char* str);
int is_digit(char ch);


/* ---------------------------------------------------
 * String Builder
 * -------------------------------------------------- */
typedef struct {
    char* string = 0;
    int maxsize = 0;
    int size = 0;
} StringBuilder;

int str_init    (StringBuilder* builder, int size);
int str_destory (StringBuilder* builder);
int str_set     (StringBuilder* builder, char* str, int str_size);
int str_set     (StringBuilder* builder, char* str);               // Use this for null-terminated string
int str_append  (StringBuilder* builder, char* str, int str_size);
int str_append  (StringBuilder* builder, char* str);               // Use this for null-terminated string
int str_put     (StringBuilder* builder, char ch);

