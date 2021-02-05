#ifndef _ACCEL_H
#define _ACCEL_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <uk/arch/types.h>
#ifndef __KERNEL__
#define __user
#endif

///* IOCTLs */
#define VACCEL_SESS_CREATE      _IOWR('@', 0, struct accel_session)
#define VACCEL_SESS_DESTROY     _IOWR('@', 1, struct accel_session)
#define VACCEL_DO_OP            _IOWR('@', 2, struct accel_session)

/* Unikraft IOCTLs */
//#define VACCEL_SESS_CREATE      0x6660
//#define VACCEL_SESS_DESTROY     0x6661
//#define VACCEL_DO_OP            0x6662

struct accel_arg {
	__u32 len;
	__u8 __user *buf;
	__u8 *usr_pages;
	__u32 usr_npages;
	__u8 padding[5];
};

struct accel_op {
	/* number of in arguments */
	__u32 in_nr;
	/* number of out arguments */
	__u32 out_nr;

	/* pointer to in arguments */
	struct accel_arg __user *in;
	/* pointer to out arguments */
	struct accel_arg __user *out;
};

struct accel_session {
	/* session id */
	__u32 id;
	/* operation performed currently */
	struct accel_op op;
};

//#define VACCEL_SESS_CREATE      _IOWR('@', 0, struct accel_session)
//#define VACCEL_SESS_DESTROY     _IOWR('@', 1, struct accel_session)
//#define VACCEL_DO_OP            _IOWR('@', 2, struct accel_session)
struct device *accel_dev_init(void);

#endif /* _ACCEL_H */
