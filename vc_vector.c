#include "vc_vector.h"
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define GROWTH_FACTOR 1.5
#define DEFAULT_COUNT_OF_ELEMENETS 8

// ----------------------------------------------------------------------------

// vc_vector structure

struct vc_vector {
  size_t element_count;
  size_t element_size;
  size_t reserved_size;
  void* data;
  vc_vector_free_func* free_func;
};

// ----------------------------------------------------------------------------

// auxillary methods

bool vc_vector_realloc(vc_vector* vector, size_t new_count) {
  const size_t new_size = new_count * vector->element_size;
  void* new_data = realloc(vector->data, new_size);
  if (unlikely(!new_data)) {
    return false;
  }
  
  vector->reserved_size = new_size;
  vector->data = new_data;
  return true;
}

// [first_index, last_index)
void vc_vector_call_free_func(vc_vector* vector, size_t first_index, size_t last_index) {
  for (size_t i = first_index; i < last_index; ++i) {
    vector->free_func(vc_vector_at(vector, i));
  }
}

void vc_vector_call_free_func_all(vc_vector* vector) {
  vc_vector_call_free_func(vector, 0, vc_vector_count(vector));
}

// ----------------------------------------------------------------------------

// Contol

vc_vector* vc_vector_create(size_t count_elements, size_t size_of_element, vc_vector_free_func* free_func) {
  vc_vector* v = (vc_vector*)malloc(sizeof(vc_vector));
  if (likely(v != NULL)) {
    v->data = NULL;
    v->element_count = 0;
    v->element_size = size_of_element;
    v->free_func = free_func;
    
    if (unlikely(count_elements == 0)) {
      count_elements = DEFAULT_COUNT_OF_ELEMENETS;
    }
    
    if (unlikely(size_of_element < 1 ||
                 !vc_vector_realloc(v, count_elements))) {
      free(v);
      v = NULL;
    }
  }

  return v;
}

vc_vector* vc_vector_create_copy(const vc_vector* vector) {
  vc_vector* new_vector = vc_vector_create(vector->reserved_size / vector->element_count,
                                           vector->element_size,
                                           vector->free_func);
  if (unlikely(!new_vector)) {
    return new_vector;
  }
  
  if (unlikely(!vc_vector_append(new_vector, vector->data, vector->element_count))) {
    vc_vector_release(new_vector);
    new_vector = NULL;
    return new_vector;
  }
  
  return new_vector;
}

void vc_vector_release(vc_vector* vector) {
  if (unlikely(vector->free_func != NULL)) {
    vc_vector_call_free_func_all(vector);
  }
  
  if (likely(vector->reserved_size != 0)) {
    free(vector->data);
  }
  
  free(vector);
}

bool vc_vector_is_equals(vc_vector* vector1, vc_vector* vector2) {
  const size_t size_vector1 = vc_vector_size(vector1);
  if (unlikely(size_vector1 != vc_vector_size(vector2))) {
    return false;
  }
  
  return memcmp(vector1->data, vector2->data, size_vector1) == 0;
}

float vc_vector_get_growth_factor() {
  return GROWTH_FACTOR;
}

size_t vc_vector_get_default_count_of_elements() {
  return DEFAULT_COUNT_OF_ELEMENETS;
}

size_t vc_vector_struct_size() {
  return sizeof(struct vc_vector);
}

// ----------------------------------------------------------------------------

// Element access

void* vc_vector_at(vc_vector* vector, size_t index) {
  return vector->data + index * vector->element_size;
}

void* vc_vector_front(vc_vector* vector) {
  return vector->data;
}

void* vc_vector_back(vc_vector* vector) {
  return vector->data + (vector->element_count - 1) * vector->element_size;
}

void* vc_vector_data(vc_vector* vector) {
  return vector->data;
}

// ----------------------------------------------------------------------------

// Iterators

void* vc_vector_begin(vc_vector* vector) {
  return vector->data;
}

void* vc_vector_end(vc_vector* vector) {
  return vector->data + vector->element_size * vector->element_count;
}

void* vc_vector_next(vc_vector* vector, void* i) {
  return i + vector->element_size;
}

// ----------------------------------------------------------------------------

// Capacity

bool vc_vector_empty(vc_vector* vector) {
  return vector->element_count == 0;
}

size_t vc_vector_count(const vc_vector* vector) {
  return vector->element_count;
}

size_t vc_vector_size(const vc_vector* vector) {
  return vector->element_count * vector->element_size;
}

size_t vc_vector_max_count(const vc_vector* vector) {
  return vector->reserved_size / vector->element_size;
}

size_t vc_vector_max_size(const vc_vector* vector) {
  return vector->reserved_size;
}

bool vc_vector_reserve_count(vc_vector* vector, size_t new_count) {
  size_t new_size = vector->element_size * new_count;
  if (unlikely(new_size == vector->reserved_size)) {
    return true;
  }
  
  return vc_vector_realloc(vector, new_count);
}

bool vc_vector_reserve_size(vc_vector* vector, size_t new_size) {
  if (unlikely(new_size == vector->reserved_size)) {
    return true;
  }
  
  return vc_vector_realloc(vector, new_size / vector->element_size);
}

// ----------------------------------------------------------------------------

// Modifiers

void vc_vector_clear(vc_vector* vector) {
  if (unlikely(vector->free_func != NULL)) {
    vc_vector_call_free_func_all(vector);
  }
  
  vector->element_count = 0;
}

bool vc_vector_insert(vc_vector* vector, size_t index, const void* value) {
  if (unlikely(vc_vector_max_count(vector) < vector->element_count + 1)) {
    if (!vc_vector_realloc(vector, vc_vector_max_count(vector) * GROWTH_FACTOR)) {
      return false;
    }
  }
  
  if (unlikely(!memmove(vc_vector_at(vector, index + 1),
                        vc_vector_at(vector, index),
                        vector->element_size * (vector->element_count - index)))) {

    return false;
  }
  
  vc_vector_set(vector, index, value);
  vector->element_count++;
  return true;
}

bool vc_vector_erase(vc_vector* vector, size_t index) {
  if (unlikely(vector->free_func != NULL)) {
    vector->free_func(vc_vector_at(vector, index));
  }
  
  if (unlikely(!memmove(vc_vector_at(vector, index),
                        vc_vector_at(vector, index + 1),
                        vector->element_size * (vector->element_count - index)))) {
    return false;
  }
  
  vector->element_count--;
  return true;
}

bool vc_vector_erase_range(vc_vector* vector, size_t first_index, size_t last_index) {
  if (unlikely(vector->free_func != NULL)) {
    vc_vector_call_free_func(vector, first_index, last_index);
  }
  
  if (unlikely(!memmove(vc_vector_at(vector, first_index),
                        vc_vector_at(vector, last_index),
                        vector->element_size * (vector->element_count - last_index)))) {
    return false;
  }
  
  vector->element_count -= last_index - first_index;
  return true;
}

bool vc_vector_append(vc_vector* vector, const void* values, size_t count) {
  while ((vector->element_count + count) * vector->element_size > vector->reserved_size) {
    if (unlikely(!vc_vector_realloc(vector, vc_vector_max_count(vector) * GROWTH_FACTOR))) {
      return false;
    }
  }
  
  if (unlikely(!vc_vector_set_multiple(vector, vector->element_count, values, count))) {
    return false;
  }
  
  vector->element_count += count;
  return true;
}

bool vc_vector_push_back(vc_vector* vector, const void* value) {
  if (unlikely(!vc_vector_append(vector, value, 1))) {
    return false;
  }
  
  return true;
}

bool vc_vector_pop_back(vc_vector* vector) {
  if (unlikely(vector->free_func != NULL)) {
    vector->free_func(vc_vector_back(vector));
  }
  
  vector->element_count--;
  return true;
}

bool vc_vector_set(vc_vector* vector, size_t index, const void* value) {
  if (unlikely(vector->free_func != NULL)) {
    vector->free_func(vc_vector_at(vector, index));
  }
  
  return memcpy(vc_vector_at(vector, index), value, vector->element_size) != NULL;
}

bool vc_vector_set_multiple(vc_vector* vector, size_t index, const void* values, size_t count) {
  if (unlikely(vector->free_func != NULL)) {
    vc_vector_call_free_func(vector, index, index + count);
  }
  
  return memcpy(vc_vector_at(vector, index), values, vector->element_size * count) != NULL;
}
