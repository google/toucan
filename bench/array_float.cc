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
  float  a[SIZE];
  double start = get_time_usec();
  for (int i = 0; i < SIZE; i += 1) {
    a[i] = 2000000.0f;
  }
  for (int j = 0; j < COUNT; j += 1) {
    for (int i = 0; i < SIZE; i += 1) {
      a[i] = a[i] * 1.00001f + 1.0f;
    }
  }
  double end = get_time_usec();
  printf("result is %f\n", a[3]);
  printf("elapsed time %f usec, %f usec per iteration\n", end - start,
         (end - start) / (SIZE * COUNT));
  return 0;
}
