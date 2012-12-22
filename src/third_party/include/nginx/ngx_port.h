#ifndef NGX_PORT_H
#define NGX_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define ngx_memset(p, b, n) memset(p, b, n)
#define ngx_memzero(p, n)   ngx_memset(p, 0, n)

#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


#if defined(__LP64__) || defined(__ILP64__) || defined(__LLP64__)
#	define NGX_PTR_SIZE 8
#elif defined(__ILP32__)
#	define NGX_PTR_SIZE 4
#endif

#if !defined(NDEBUG)
#	define NGX_DEBUG_MALLOC 1
#endif

#define NGX_LOG_STDERR            0
#define NGX_LOG_EMERG             1
#define NGX_LOG_ALERT             2
#define NGX_LOG_CRIT              3
#define NGX_LOG_ERR               4
#define NGX_LOG_WARN              5
#define NGX_LOG_NOTICE            6
#define NGX_LOG_INFO              7
#define NGX_LOG_DEBUG             8

typedef unsigned char u_char;
typedef int ngx_int_t;
typedef unsigned int ngx_uint_t;

void ngx_os_init();

#endif // NGX_PORT_H
