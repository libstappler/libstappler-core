#include "stappler-buildconfig.h"

#if LINUX

#define BACKTRACE_ELF_SIZE 64
#define HAVE_ATOMIC_FUNCTIONS 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_DECL_GETPAGESIZE 1
#define HAVE_DECL_STRNLEN 1
#define HAVE_DECL__PGMPTR 0
#define HAVE_DLFCN_H 1
#define HAVE_DL_ITERATE_PHDR 1
#define HAVE_FCNTL 1
#define HAVE_GETIPINFO 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIBLZMA 1
#define HAVE_LINK_H 1
#define HAVE_LSTAT 1
#define HAVE_MEMORY_H 1
#define HAVE_READLINK 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYNC_FUNCTIONS 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
#define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#elif ANDROID

#define HAVE_ATOMIC_FUNCTIONS 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_DECL_GETPAGESIZE 1
#define HAVE_DECL_STRNLEN 1
#define HAVE_DECL__PGMPTR 0
#define HAVE_DLFCN_H 1
#define HAVE_DL_ITERATE_PHDR 1
#define HAVE_FCNTL 1
#define HAVE_GETIPINFO 1
#define HAVE_INTTYPES_H 1
#define HAVE_LINK_H 1
#define HAVE_LSTAT 1
#define HAVE_MEMORY_H 1
#define HAVE_READLINK 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYNC_FUNCTIONS 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
#define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#if __aarch64__
#define BACKTRACE_ELF_SIZE 64
#elif __x86_64__
#define BACKTRACE_ELF_SIZE 64
#elif __arm__
#define BACKTRACE_ELF_SIZE 32
#define _FILE_OFFSET_BITS 64
#elif i386
#define BACKTRACE_ELF_SIZE 32
#define _FILE_OFFSET_BITS 64
#else
#error Unknown Android arch
#endif

#elif MACOS

#define BACKTRACE_ELF_SIZE unused
#define BACKTRACE_XCOFF_SIZE unused

#define HAVE_ATOMIC_FUNCTIONS 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_DECL_GETPAGESIZE 1
#define HAVE_DECL_STRNLEN 1
#define HAVE_DECL__PGMPTR 0
#define HAVE_DLFCN_H 1
#define HAVE_FCNTL 1
#define HAVE_GETIPINFO 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIBLZMA 1
#define HAVE_LSTAT 1
#define HAVE_MACH_O_DYLD_H 1
#define HAVE_MEMORY_H 1
#define HAVE_READLINK 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYNC_FUNCTIONS 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_ZLIB 1
#define STDC_HEADERS 1

#ifndef _ALL_SOURCE
#define _ALL_SOURCE 1
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#ifndef _TANDEM_SOURCE
#define _TANDEM_SOURCE 1
#endif

#ifndef __EXTENSIONS__
#define __EXTENSIONS__ 1
#endif

#ifndef _DARWIN_USE_64_BIT_INODE
#define _DARWIN_USE_64_BIT_INODE 1
#endif

#else

#error Unknown platform

#endif
