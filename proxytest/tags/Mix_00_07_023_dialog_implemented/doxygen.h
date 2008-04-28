//Define all features if we are running in documentation creation mode
#ifndef __DOXYGEN_H
#define __DOXYGEN_H
	#ifdef DOXYGEN
		#define REPLAY_DETECTION
		#define DELAY_USERS
		#define COUNTRY_STATS
		#define USE_POOL
		#define PAYMENT
		#define HAVE_UNIX_DOMAIN_PROTOCOL
		#define HAVE_EPOLL
	#endif
#endif
