#pragma once

#include "type.h"

template<typename T, u32 Size>
class Array
{
public:
    Array()
    : size(0)
    {

    }

    ~Array()
    {

    }

    T& operator[](u32 i)
    {
        return array[i];
    }

    const T& operator[](u32 i) const
    {
        return array[i];
    }


private:
    T array[Size];
    u32 size;
};