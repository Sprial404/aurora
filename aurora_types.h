#ifndef AURORA_TYPES_H
#define AURORA_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef union {
  struct {
    u32 x, y;
  };
  struct {
    u32 width, height;
  };
  u32 v[2];
} v2u;

#endif // AURORA_TYPES_H
