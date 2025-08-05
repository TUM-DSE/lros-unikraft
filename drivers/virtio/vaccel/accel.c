#include <uk/print.h>
#include <devfs/device.h>

#include <accel.h>
#include "virtio_accel-common.h"

#define DEV_VACCEL_NAME "accel"

int dev_accel_open(struct device *device __unused, int mode __unused)
{
	uk_pr_debug("Virtio-accel device open\n");
	return 0;
}


int dev_accel_ioctl(struct device *dev, unsigned long cmd, void *arg)
{
	struct virtio_accel *vaccel = dev->private_data;
	struct virtio_accel_req *req = NULL;
	struct accel_session *sess = NULL;
	int ret = 0;

	req = uk_zalloc(uk_alloc_get_default(), sizeof(struct virtio_accel_req));
	if (!req)
		return -ENOMEM;

	sess = (struct accel_session *) arg;
	req->usr = arg;
	req->priv = sess;
	req->vaccel = vaccel;
	/* 
	 * Just a random value different than 0, so we can wait till virtqueue
	 * callback change it to 0
	 */
	req->ret = 7;
	uk_waitq_init(&req->completion);

	switch(cmd) {
	case VACCEL_SESS_CREATE:
		ret = virtaccel_req_gen_create_session(req);
		if (ret < 0)
			goto err_req;
		break;
	case VACCEL_SESS_DESTROY:
		ret = virtaccel_req_gen_destroy_session(req);
		if (ret < 0)
			goto err_req;
		break;
	case VACCEL_DO_OP:
		ret = virtaccel_req_gen_operation(req);
		if (ret != -EINPROGRESS)
			goto err_req;

		break;
	default:
		uk_pr_err("Invalid IOCTL\n");
		ret = -EFAULT;
		goto err;
	}
	uk_pr_debug("Waiting for request to complete...\n");
	uk_waitq_wait_event(&req->completion, req->ret != 7);
	virtaccel_handle_req_result(req);
	virtaccel_clear_req(req);
	uk_pr_debug("Request completed\n");
	ret = req->ret;
err_req:
	if (req)
		uk_free(uk_alloc_get_default(), req);
err:
	return ret;
}

int dev_accel_close(struct device *device __unused)
{
	uk_pr_info("Virtio-accel device close\n");
	return 0;
}

static struct devops accel_devops = {
	.ioctl = dev_accel_ioctl,
	.open = dev_accel_open,
	.close = dev_accel_close,
};

static struct driver drv_accel = {
	.devops = &accel_devops,
	.devsz = sizeof(struct virtio_accel),
	.name = DEV_VACCEL_NAME
};

struct device *accel_dev_init()
{
	struct device *dev;

	uk_pr_info("Register '%s' to devfs\n", DEV_VACCEL_NAME);

	/* register /dev/accel */
	dev = device_create(&drv_accel, DEV_VACCEL_NAME, D_CHR);
	if (dev == NULL) {
		uk_pr_err("Failed to register '%s' to devfs\n",
			  DEV_VACCEL_NAME);
		return NULL;
	}

	return dev;
}
