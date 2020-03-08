#pragma once

extern "C" {
    void*	memcpy(void* p_dst, void* p_src, int size);
    void	memset(void* p_dst, char ch, int size);
    int	    strlen(char* p_str);
}
char * strcpy(char *dst,const char *src);