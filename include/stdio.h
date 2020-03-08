#pragma once

typedef	char *			va_list;

int printf(const char *fmt, ...);

int sprintf(char* buf, const char *fmt, ...);

int vsprintf(char *buf, const char *fmt, va_list args);

