//from zmq
#ifndef __TSY_ERR_H__
#define __TSY_ERR_H__
#include "likely.h"
#include <errno.h>
#include <string>


inline void	my_abort (const char *errmsg_)
{
    (void)errmsg_;
    abort();
}



#ifdef PLATFORM_WINDOWS

void win_error (char *buffer_, size_t buffer_size_);

// Provides convenient way to check GetLastError-style errors on Windows.
#define win_assert(x) \
	do {\
		if (unlikely (!(x))) {\
			char errstr [256];\
			win_error (errstr, 256);\
			fprintf (stderr, "Assertion failed: %s (%s:%d)\n", errstr, \
					__FILE__, __LINE__);\
			my_abort (errstr);\
		}\
	} while (false)
#else

//  Provides convenient way to check for POSIX errors.
#define posix_assert(x) \
	do {\
		if (unlikely (x)) {\
			const char *errstr = strerror (x);\
			fprintf (stderr, "%s (%s:%d)\n", errstr, __FILE__, __LINE__);\
			my_abort (errstr);\
		}\
	} while (false)
#endif

//  This macro works in exactly the same way as the normal assert. It is used
//  in its stead because standard assert on Win32 in broken - it prints nothing
//  when used within the scope of JNI library.
#define tsy_assert(x) \
	do {\
		if (unlikely (!(x))) {\
			fprintf (stderr, "Assertion failed: %s (%s:%d)\n", #x, \
				__FILE__, __LINE__);\
			my_abort (#x);\
		}\
	} while (false) 

#define errno_assert(x) \
	do {\
		if (unlikely (!(x))) {\
			const char *errstr = strerror (errno);\
			fprintf (stderr, "%s (%s:%d)\n", errstr, __FILE__, __LINE__);\
			my_abort (errstr);\
	}\
	} while (false)




#endif	/* __TSY_ERR_H__ */
