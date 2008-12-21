/*
 * Note: RTC code is taken from mplayer code, thanks to you guys !
 * http://www.mplayerhq.hu/
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>
#include <clutter/clutter.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_RTC
#ifdef __linux__
#include <linux/rtc.h>
#else
#include <rtc.h>
#define RTC_IRQP_SET RTCIO_IRQP_SET
#define RTC_PIE_ON   RTCIO_PIE_ON
#endif /* __linux__ */
#endif /* HAVE_RTC */

#include "noya.h"

LOG_DECLARE("RTC");

#ifdef HAVE_RTC
unsigned long		rtc_ts;
int					rtc_fd = -1;
char				*rtc_device = NULL;
#endif

void rtc_init()
{
#ifdef HAVE_RTC
	if ( (rtc_fd = open(rtc_device ? rtc_device : "/dev/rtc", O_RDONLY)) < 0 )
	{
		l_errorf("RTC timer: unable to open %s", rtc_device ? rtc_device : "/dev/rtc");
	}
	else
	{
		unsigned long irqp = 1024; /* 512 seemed OK. 128 is jerky. */

		if ( ioctl(rtc_fd, RTC_IRQP_SET, irqp) < 0)
		{
			l_errorf("RTC timer: error in RTC_IRQP_SET : %s", strerror(errno));
			l_errorf("RTC timer: try adding \"echo %lu >" \
					 "/proc/sys/dev/rtc/max-user-freq\" to your system startup scripts. !", irqp);
			close(rtc_fd);
			rtc_fd = -1;
		}
		else if ( ioctl(rtc_fd, RTC_PIE_ON, 0) < 0 )
		{
			/* variable only by the root */
			l_errorf("RTC timer: error in ioctl RTC_PIE_ON : %s", strerror(errno));
			close(rtc_fd);
			rtc_fd = -1;
		}
		else
			l_printf("RTC timer: activated.");
	}
#endif
}

void rtc_sleep()
{
#ifdef HAVE_RTC
	if ( rtc_fd >= 0 )
	{
		if ( read(rtc_fd, &rtc_ts, sizeof(rtc_ts)) <= 0 )
			l_errorf("error while reading RTC timer !");
	}
	else
#endif
	{
		usleep(100);
	}
}
