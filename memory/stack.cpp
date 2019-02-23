#include "stack.h"
#include <cassert>

namespace Memory
{

void Stack::init(void* address, size_t size)
{
	base_address = address;
	total_size = size;
	used = 0;
	offset = 0;
}

void* Stack::allocate(size_t allocation_size)
{
	assert(offset + allocation_size < total_size);

	size_t top = (size_t)base_address + offset;
	offset += allocation_size;
	used = offset;

	return (void*)top;
}

void Stack::free(void* address)
{
	size_t top = (size_t)address;
	offset = top - (size_t)base_address;
	used = offset;
}

void Stack::clear()
{
	offset = 0;
	used = 0;
}

} // namespace Memory
