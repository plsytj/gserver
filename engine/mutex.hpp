#ifndef __TSY_MUTEX_H__
#define __TSY_MUTEX_H__

#include "err.hpp"
#include <pthread.h>

class mutex_t
{
public:
	inline mutex_t ()
	{
		int rc = pthread_mutex_init (&mutex, NULL);
		posix_assert (rc);
	}

	inline ~mutex_t ()
	{
		int rc = pthread_mutex_destroy (&mutex);
		posix_assert (rc);
	}

	inline void lock ()
	{
		int rc = pthread_mutex_lock (&mutex);
		posix_assert (rc);
	}

	inline void unlock ()
	{
		int rc = pthread_mutex_unlock (&mutex);
		posix_assert (rc);
	}

private:

	pthread_mutex_t mutex;

	// Disable copy construction and assignment.
	mutex_t (const mutex_t&);
	const mutex_t &operator = (const mutex_t&);
};

struct scoped_lock_t
{
    scoped_lock_t (mutex_t& mutex_)
        : mutex (mutex_)
    {
        mutex.lock ();
    }

    ~scoped_lock_t ()
    {
        mutex.unlock ();
    }

private:

    mutex_t& mutex;

    // Disable copy construction and assignment.
    scoped_lock_t (const scoped_lock_t&);
    const scoped_lock_t &operator = (const scoped_lock_t&);
};

#endif
