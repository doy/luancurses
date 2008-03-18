#ifndef STRINGS_H
#define STRINGS_H

typedef void (*table_cb)(const char* str, int tag, void* data);

int get_color_enum(const char* str);
int get_mode_enum(const char* str);
int get_key_enum(const char* str);
int get_char_enum(const char* str);

const char* get_color_str(int tag);
const char* get_mode_str(int tag);
const char* get_key_str(int tag);
const char* get_char_str(int tag);

void each_color(table_cb cb, void* data);
void each_mode(table_cb cb, void* data);
void each_key(table_cb cb, void* data);
void each_char(table_cb cb, void* data);

#endif
