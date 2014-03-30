/*
 * utility.h
 *
 *  Created on: Feb 23, 2014
 *      Author: generessler
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#define STATIC_ARRAY_SIZE(A) (sizeof A / sizeof A[0])

#define New(Ptr) do { \
	(Ptr) = safe_malloc(sizeof *(Ptr), __FILE__, __LINE__); \
} while (0)

#define NewDecl(Type, Ptr) Type *Ptr; New(Ptr)

#define NewArray(Ptr, Size) do { \
	(Ptr) = safe_malloc((Size) * sizeof *(Ptr), __FILE__, __LINE__); \
} while (0)

#define NewArrayDecl(Type, Ptr, Size) Type *Ptr; NewArray(Ptr, Size)

#define CopyArray(Dst, Src, N)   memcpy((Dst), (Src), (N) * sizeof *(Src))

#define RenewArray(Ptr, Size) do { \
	(Ptr) = safe_realloc((Ptr), (Size) * sizeof *(Ptr), __FILE__, __LINE__); \
} while (0)

#define Free(Ptr) do { \
	free(Ptr); \
	Ptr = NULL; \
} while (0)

#define bit(N) (1u << (N))

#ifndef NTRACE
#define TRACE(Args)  trace Args
void trace(const char *fmt, ...);
#else
#define TRACE(Args)
#endif

void *safe_malloc(size_t size, const char *file, int line);
void *safe_realloc(void *p, size_t size, const char *file, int line);
int high_bit_position(unsigned n);
double rand_double(void);

#endif /* UTILITY_H_ */
