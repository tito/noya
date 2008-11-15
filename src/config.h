#ifndef __CONFIG_H
#define __CONFIG_H

#include <sys/queue.h>

/* Configuration
 * FIXME Move this !
 */
#define NA_VERSION				"0.1"
#ifdef HAVE_RTC
#define NA_ADDVERSION			"(rtc)"
#else
#define NA_ADDVERSION			"(no rtc)"
#endif
#define NA_TITLE				"NOYA - live music sequencer"
#define NA_BANNER				"Noya, ver " NA_VERSION NA_ADDVERSION " - by Mathieu Virbel <tito@bankiz.org>"
#define NA_WEBSITE				"Visit http://noya.txzone.net/ for more information"
#define	NA_TUIO_PORT			"3333"
#define NA_CONFIG_FN			"config.cfg"
#define NA_OUTPUT_CHANNELS		2
#define NA_DEF_SAMPLERATE		44100
#define NA_DEF_SOUNDS_MAX		128
#define NA_DEF_PATH_LEN			512

#define	TUIO_OBJECT_OSCPATH				"/tuio/2Dobj"
#define	TUIO_CURSOR_OSCPATH				"/tuio/2Dcur"
#define	TUIO_OBJECT_MAX					128
#define	TUIO_CURSOR_MAX					64
#define	TUIO_CURSOR_THRESHOLD_CLICK		3

#if NA_OUTPUT_CHANNELS != 2
#error "Noya support only output stereo !"
#endif

#endif
