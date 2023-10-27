#include "j_buffers.h"
#include "utils.h"

void memory_buffer_mallocate(MemoryBuffer* buffer, s64 size_in_bytes, char* name)
{
	ASSERT_TRUE(buffer->size == 0, "Arena is not already allocated");
	buffer->memory = (byte*)malloc(size_in_bytes);
	memset(buffer->memory, 0x00, size_in_bytes);
	buffer->size = size_in_bytes;
	strcpy_s(buffer->name, name);
	printf("Buffer '%s' mallocated with %llu kilobytes.", name, size_in_bytes / 1024);
}

void memory_buffer_wipe(MemoryBuffer* buffer)
{
	memset(buffer->memory, 0x00, buffer->size);
	printf("Buffer '%s' wiped to zero.", buffer->name);
}

void memory_buffer_free(MemoryBuffer* buffer)
{
	free(buffer->memory);
	buffer->size = 0;
	buffer->memory = nullptr;
	printf("Buffer '%s' freed.", buffer->name);
}

JArray j_array_init(s64 max_items, s64 item_size, byte* data_ptr)
{
	ASSERT_TRUE(1 < max_items, "Array has more than one slot allocated");
	JArray array = {
		.item_size_bytes = item_size,
		.max_items = max_items,
		.items_count = 0,
		.data = data_ptr
	};
	return array;
}

s64 j_array_add(JArray* array, byte* element_ptr)
{
	// Always reserve one slot for swaps
	ASSERT_TRUE(array->items_count + 1 < array->max_items, "Array has item capacity left");
	void* array_ptr = &array->data[array->items_count * array->item_size_bytes];
	memcpy(array_ptr, element_ptr, array->item_size_bytes);
	array->items_count++;
	return array->items_count - 1;
}

byte* j_array_get(JArray* array, s64 index)
{
	s64 used_index = index % array->max_items;
	byte* item = &array->data[used_index * array->item_size_bytes];
	return item;
}

void j_array_pop_back(JArray* array)
{
	array->items_count--;
	if (array->items_count < 0) array->items_count = 0;
}

void j_array_swap(JArray* array, s64 index_1, s64 index_2)
{
	s64 unused_index = array->items_count + 1;
	void* temp_ptr = &array->data[unused_index];
	memcpy(temp_ptr, &array->data[index_1], array->item_size_bytes);
	memcpy(&array->data[index_1], &array->data[index_2], array->item_size_bytes);
	memcpy(&array->data[index_2], temp_ptr, array->item_size_bytes);
}

void j_array_replace(JArray* array, byte* item_ptr, u64 index)
{
	ASSERT_TRUE(index <= array->items_count - 1, "Array item index is included");
	void* dest = &array->data[index * array->item_size_bytes];
	memcpy(dest, item_ptr, array->item_size_bytes);
}