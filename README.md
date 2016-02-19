# vc_vector
Fast simple C vector implementation


# Getting started
```
#include "vc_vector.h"
...
const size_t count = 10;
vc_vector* v = vc_vector_create(count, sizeof(int), NULL);
if (!v) {
  return;
}

for (int i = 0; i < count; ++i) {
  if (!vc_vector_push_back(v, &i)) {
    printf("Failed pushing to vector element %u.\n", i);
  }
}

for (void* i = vc_vector_begin(v);
           i != vc_vector_end(v);
           i = vc_vector_next(v, i)) {
  printf("%u; ", *(int*)i);
}

vc_vector_release(v);
```
