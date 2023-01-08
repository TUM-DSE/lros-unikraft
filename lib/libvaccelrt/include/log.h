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

#ifndef __VACCEL_LOG__
#define __VACCEL_LOG__

#include <uk/print.h>

#define vaccel_info uk_pr_info
#define vaccel_warn uk_pr_warn
#define vaccel_debug uk_pr_debug
#define vaccel_error uk_pr_err
#define vaccel_trace uk_pr_debug
#define vaccel_fatal uk_pr_crit

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_log_init(void);
int vaccel_log_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_LOG__ */
