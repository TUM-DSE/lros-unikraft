/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "error.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

#define VACCEL_DEBUG_LVL_ERROR 1
#define VACCEL_DEBUG_LVL_WARN  2
#define VACCEL_DEBUG_LVL_INFO  3
#define VACCEL_DEBUG_LVL_DEBUG 4

/*
 * TODO: Set debug level and file, similar to LWIP in Unikraft
 */
#if 0
static void set_debug_level(void)
{
	char *env = getenv("VACCEL_DEBUG_LEVEL");
	if (!env)
		return;

	int level = atoi(env);
	int nEnabledLevels = 0;
	switch (level) {
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_DEBUG:
			nEnabledLevels |= SLOG_DEBUG;
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_INFO:
			nEnabledLevels |= SLOG_INFO;
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_WARN:
			nEnabledLevels |= SLOG_WARN;
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_ERROR:
			nEnabledLevels |= SLOG_ERROR;
		default:
			break;
	}

	SLogConfig cfg;
	cfg.nFlags = nEnabledLevels;
}

static void set_log_file(void)
{
	SLogConfig cfg;

	char *env = getenv("VACCEL_LOG_FILE");
	if (!env)
		return;

	strncpy(cfg.sFileName, env, SLOG_NAME_MAX-1);

	if (strncmp(cfg.sFileName, "/dev/stdout", SLOG_NAME_MAX-1)
			&& strncmp(cfg.sFileName, "/dev/stderr", SLOG_NAME_MAX-1)) {
		cfg.nToFile = 1;
		cfg.nToScreen = 0;
	}

}
#endif

int vaccel_log_init(void)
{
#if 0
	set_debug_level();
	set_log_file();
#endif

	return VACCEL_OK;
}

int vaccel_log_shutdown(void)
{
	return VACCEL_OK;
}
