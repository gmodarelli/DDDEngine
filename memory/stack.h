#pragma once

namespace Memory
{

struct Stack
{
	void init(void* address, size_t size);
	void* allocate(size_t _size);
	void free(void* address);
	void clear();

	// Base address of the Stack
	void* base_address = nullptr;
	// Size in bytes
	size_t total_size; 
	size_t used;
	// Current top of the stack
	size_t offset;

};

} // namespace Memory
