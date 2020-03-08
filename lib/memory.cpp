#include "memory.h"

void* operator new(size_t size)
{
    return (void*)size;
}

void operator delete(void *p)
{

}