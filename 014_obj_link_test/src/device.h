#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdarg.h> //support for va_list

#define KERN_SOH	"\001"		/* ASCII Start Of Header */
#define KERN_SOH_ASCII	'\001'

#define KERN_EMERG	KERN_SOH "0"	/* system is unusable */
#define KERN_ALERT	KERN_SOH "1"	/* action must be taken immediately */
#define KERN_CRIT	KERN_SOH "2"	/* critical conditions */
#define KERN_ERR	KERN_SOH "3"	/* error conditions */
#define KERN_WARNING	KERN_SOH "4"	/* warning conditions */
#define KERN_NOTICE	KERN_SOH "5"	/* normal but significant condition */
#define KERN_INFO	KERN_SOH "6"	/* informational */
#define KERN_DEBUG	KERN_SOH "7"	/* debug-level messages */

#define KERN_DEFAULT	KERN_SOH "d"	/* the default kernel loglevel */

struct va_format {
    const char *fmt;
    va_list *va;
};

void dev_emerg(const struct device *dev, const char *fmt, ...);
void dev_alert(const struct device *dev, const char *fmt, ...);
void dev_crit(const struct device *dev, const char *fmt, ...);
void dev_err(const struct device *dev, const char *fmt, ...);
void dev_warn(const struct device *dev, const char *fmt, ...);
void dev_notice(const struct device *dev, const char *fmt, ...);
void _dev_info(const struct device *dev, const char *fmt, ...);

#endif//_DEVICE_H_
