#pragma once

#include "type.h"

void* operator new(size_t size);

void operator delete(void *p);

namespace mem {

}