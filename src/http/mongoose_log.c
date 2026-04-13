#include <stdarg.h>

#include "mongoose.h"
#include "zlog.h"

static _Thread_local int s_mg_ll = MG_LL_INFO;

static zlog_category_t *mongoose_cat(void)
{
	return zlog_get_category("mongoose");
}

void mg_log_prefix(int level, const char *file, int line, const char *fname)
{
	(void)file;
	(void)line;
	(void)fname;
	s_mg_ll = level;
}

void mg_log(const char *fmt, ...)
{
	int zlvl;
	if (s_mg_ll <= MG_LL_ERROR) {
		zlvl = ZLOG_LEVEL_ERROR;
	} else if (s_mg_ll <= MG_LL_INFO) {
		zlvl = ZLOG_LEVEL_INFO;
	} else {
		zlvl = ZLOG_LEVEL_DEBUG;
	}

	va_list ap;
	va_start(ap, fmt);
	vzlog(mongoose_cat(), __FILE__, sizeof(__FILE__) - 1, __func__,
	      sizeof(__func__) - 1, __LINE__, zlvl, fmt, ap);
	va_end(ap);
}
