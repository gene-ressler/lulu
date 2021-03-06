/*
 * utility.h
 *
 *  Created on: Feb 23, 2014
 *      Author: generessler
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include "namespace.h"

#define STATIC_ARRAY_SIZE(A) ((int)(sizeof A / sizeof A[0]))

#ifdef LULU_STD_C

// Our allocators.
#define New(Ptr) do { \
    (Ptr) = safe_malloc(sizeof *(Ptr), __FILE__, __LINE__); \
} while (0)

#define NewArray(Ptr, Size) do { \
    (Ptr) = safe_malloc((Size) * sizeof *(Ptr), __FILE__, __LINE__); \
} while (0)

#define RenewArray(Ptr, Size) do { \
    (Ptr) = safe_realloc((Ptr), (Size) * sizeof *(Ptr), __FILE__, __LINE__); \
} while (0)

#define Free(Ptr) do { \
    free(Ptr); \
    Ptr = NULL; \
} while (0)

#define safe_malloc(Size, File, Line)   NAME(safe_malloc)(Size, File, Line)
void *safe_malloc(size_t size, const char *file, int line);

#define safe_realloc(P, Size, File, Line)   NAME(safe_realloc)(P, Size, File, Line)
void *safe_realloc(void *p, size_t size, const char *file, int line);

#endif

#ifdef LULU_GEM

#include "ruby.h"

// Ruby allocators.  We can't use the macros without rewriting to include type parameters.
#define New(Ptr) do { \
    (Ptr) = (void*)xmalloc(sizeof *(Ptr)); \
} while (0)

#define NewArray(Ptr, Size) do { \
    (Ptr) = (void*)xmalloc2((Size), sizeof *(Ptr)); \
} while (0)

#define RenewArray(Ptr, Size) do { \
    (Ptr) = (void*)xrealloc2((char*)(Ptr), (Size), sizeof *(Ptr)); \
} while (0)

#define Free(Ptr) do { \
    xfree(Ptr); \
    Ptr = NULL; \
} while (0)

#endif

#define NewDecl(Type, Ptr) Type *Ptr; New(Ptr)
#define NewArrayDecl(Type, Ptr, Size) Type *Ptr; NewArray(Ptr, Size)
#define CopyArray(Dst, Src, N)   memcpy((Dst), (Src), (N) * sizeof *(Src))

#define bit(N) (1u << (N))

#ifndef NTRACE
#define TRACE(Args)  trace Args
void trace(const char *fmt, ...);
#else
#define TRACE(Args)
#endif

#define high_bit_position(N)    NAME(high_bit_position)(N)
int high_bit_position(unsigned n);

#endif /* UTILITY_H_ */
