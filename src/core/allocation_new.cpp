#include <gloom/core/allocation_profile.hpp>
#include <stdlib.h>

void* operator new(size_t size) {
    gloom::allocation_profile_record(size);
    return malloc(size ? size : 1);
}
void* operator new[](size_t size) {
    gloom::allocation_profile_record(size);
    return malloc(size ? size : 1);
}
void operator delete(void* value) noexcept {
    free(value);
}
void operator delete[](void* value) noexcept {
    free(value);
}
void operator delete(void* value, size_t) noexcept {
    free(value);
}
void operator delete[](void* value, size_t) noexcept {
    free(value);
}
