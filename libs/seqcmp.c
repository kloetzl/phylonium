/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2019 (C) Fabian Klötzl
 */

#include "seqcmp.h"
#include "config.h"

#include <assert.h>

#define UNLIKELY(X) __builtin_expect((X), 0)

size_t seqcmp_generic(const char *begin, const char *other, size_t length)
{
	assert(begin != NULL);
	assert(other != NULL);

	size_t substitutions = 0;
	size_t i = 0;

	for (; i < length; i++) {
		if (UNLIKELY(begin[i] != other[i])) {
			substitutions++;
		}
	}

	return substitutions;
}

#ifdef ENABLE_X86_SIMD

seqcmp_fn *seqcmp_select(void)
{
	// As ifunc resolvers are called before any constructors run, we explicitly
	// have to initialize the cpu model detector.
	// https://gcc.gnu.org/onlinedocs/gcc/x86-Built-in-Functions.html
	__builtin_cpu_init();

#ifdef ENABLE_AVX512
	if (__builtin_cpu_supports("popcnt") &&
		__builtin_cpu_supports("avx512bw") &&
		__builtin_cpu_supports("avx512vl")) {
		return seqcmp_avx512;
	} else
#endif
		if (__builtin_cpu_supports("popcnt") &&
			__builtin_cpu_supports("avx2")) {
		return seqcmp_avx2;
	} else if (__builtin_cpu_supports("popcnt") &&
			   __builtin_cpu_supports("sse2")) {
		return seqcmp_sse2;
	} else {
		return seqcmp_generic;
	}
}

#if __has_attribute(ifunc) && HAVE_FUNC_ATTRIBUTE_IFUNC

size_t seqcmp(const char *begin, const char *other, size_t length)
	__attribute__((ifunc("seqcmp_select")));

#else

size_t seqcmp(const char *begin, const char *other, size_t length)
{
	return (seqcmp_select())(begin, other, length);
}

#endif

#else

size_t seqcmp(const char *begin, const char *other, size_t length)
{
	return seqcmp_generic(begin, other, length);
}

#endif
