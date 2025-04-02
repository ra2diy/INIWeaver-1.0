#pragma once

typedef unsigned       hash_t;
const int MAX_LINE_LENGTH = 10000;
const int MAX_STRING_LENGTH = 10000;
typedef char BufString[MAX_STRING_LENGTH];

typedef void (*callback_t)();

#define _TEXT_UTF8
#define _TEXT_ANSI
