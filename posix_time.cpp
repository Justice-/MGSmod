#include "posix_time.h"

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

int nanosleep(const struct timespec *requested_delay,
				struct timespec *remaining_delay)
{
//CAT:
	Sleep(requested_delay->tv_sec * 1000 + requested_delay->tv_nsec / 1000000);

   return 0;
}

#endif
