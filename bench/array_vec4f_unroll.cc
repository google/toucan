// Copyright 2023 The Toucan Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <xmmintrin.h>

#define SIZE 1024
#define COUNT 1000000

#ifdef _WIN32
#include <windows.h>
double get_time_usec() {
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  LARGE_INTEGER v;
  v.LowPart = ft.dwLowDateTime;
  v.HighPart = ft.dwHighDateTime;
  return static_cast<double>(v.QuadPart) / 10.0;
}
#else
#include <sys/time.h>
double get_time_usec() {
  struct timeval t;
  gettimeofday(&t, nullptr);
  return 1000000.0 * t.tv_sec + t.tv_usec;
}
#endif

int main() {
  float  a[SIZE] __attribute__((aligned(16)));
  float  mulconst[4] __attribute__((aligned(16))) = {1.00001f, 1.00001f, 1.00001f, 1.00001f};
  float  addconst[4] __attribute__((aligned(16))) = {1.0f, 1.0f, 1.0f, 1.0f};
  double start = get_time_usec();
  for (int i = 0; i < SIZE; ++i) {
    a[i] = 2000000.0f;
  }
  __m128 mm_mulconst = _mm_load_ps(mulconst);
  __m128 mm_addconst = _mm_load_ps(addconst);
  for (int j = 0; j < COUNT; ++j) {
    for (int i = 0; i < SIZE; i += 16) {
      __m128 v0 = _mm_load_ps(&a[i]);
      v0 = _mm_mul_ps(v0, mm_mulconst);
      v0 = _mm_add_ps(v0, mm_addconst);
      _mm_store_ps(&a[i], v0);
      __m128 v1 = _mm_load_ps(&a[i + 4]);
      v1 = _mm_mul_ps(v1, mm_mulconst);
      v1 = _mm_add_ps(v1, mm_addconst);
      _mm_store_ps(&a[i + 4], v1);
      __m128 v2 = _mm_load_ps(&a[i + 8]);
      v2 = _mm_mul_ps(v2, mm_mulconst);
      v2 = _mm_add_ps(v2, mm_addconst);
      _mm_store_ps(&a[i + 8], v2);
      __m128 v3 = _mm_load_ps(&a[i + 12]);
      v3 = _mm_mul_ps(v3, mm_mulconst);
      v3 = _mm_add_ps(v3, mm_addconst);
      _mm_store_ps(&a[i + 12], v3);
    }
  }
  double end = get_time_usec();
  printf("result is %f\n", a[3]);
  printf("elapsed time %f usec, %f usec per iteration\n", end - start,
         (end - start) / (SIZE * COUNT));
  return 0;
}
