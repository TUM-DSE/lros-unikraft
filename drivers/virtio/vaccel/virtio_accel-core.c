#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/print.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtio_ids.h>
#include <devfs/device.h>
#include <uk/sglist.h>
#include <uk/plat/spinlock.h>

#include <accel.h>
#include "virtio_accel-common.h"

#define DRIVER_NAME	"vaccel"
static struct uk_alloc *a;

void *kzalloc_node(ssize_t s)
{
	return uk_zalloc(a, s);
}

void kfree_node(void *p)
{
	return uk_free(a, p);
}

int vaccel_send_request_destroy(struct virtio_accel *vaccel, struct virtio_accel_hdr *h,
			struct virtio_accel_req *req)
{
	struct virtio_accel_vq *vaccelq = vaccel->vq;
	size_t in_segs = 0, out_segs = 0;
	int ret = 0;
	unsigned long flags;

	ukplat_spin_lock_irqsave(&vaccelq->lock, flags);

	if (virtqueue_is_full(vaccelq->vq)) {
		uk_pr_err("The virtqueue is full\n");
		ret = -ENOSPC;
		goto out_unlock;
	}

	uk_sglist_reset(&vaccelq->sg);
	ret = uk_sglist_append(&vaccelq->sg, h, sizeof(struct virtio_accel_hdr));
	if (ret < 0) {
		uk_pr_err("failed to append header in sg\n");
		goto out_unlock;
	}
	out_segs++;

	ret = uk_sglist_append(&vaccelq->sg, &req->status, sizeof(req->status));
	if (ret < 0) {
		uk_pr_err("failed to append status in sg\n");
		goto out_unlock;
	}	
	in_segs++;

	ret = virtqueue_buffer_enqueue(vaccelq->vq, req, &vaccelq->sg,
				      out_segs, in_segs);
	virtqueue_host_notify(vaccelq->vq);
out_unlock:
	ukplat_spin_unlock_irqrestore(&vaccelq->lock, flags);
	return ret;

}

int vaccel_send_request_op(struct virtio_accel *vaccel, struct virtio_accel_hdr *h, 
			struct virtio_accel_req *req)
{
	struct virtio_accel_vq *vaccelq = vaccel->vq;
	size_t in_segs = 0, out_segs = 0;
	int ret = 0;
	unsigned long flags;
	unsigned int i;

	ukplat_spin_lock_irqsave(&vaccelq->lock, flags);

	if (virtqueue_is_full(vaccelq->vq)) {
		uk_pr_err("The virtqueue is full\n");
		ret = -ENOSPC;
		goto out_unlock;
	}

	uk_sglist_reset(&vaccelq->sg);
	ret = uk_sglist_append(&vaccelq->sg, h, sizeof(struct virtio_accel_hdr));
	if (ret < 0) {
		uk_pr_err("failed to append header in sg\n");
		goto out_unlock;
	}
	out_segs++;

	if (h->op.out_nr) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.out,
				h->op.out_nr*sizeof(*h->op.out));
		if (ret < 0) {
			uk_pr_err("failed to append out args in sg\n");
			goto out_unlock;
		}
		out_segs++;
	}

	if (h->op.in_nr) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.in,
				h->op.in_nr*sizeof(*h->op.in));
		if (ret < 0) {
			uk_pr_err("failed to append inatgs in sg\n");
			goto out_unlock;
		}
		out_segs++;
	}

	for (i = 0; i < h->op.out_nr; i++) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.out[i].buf,
				h->op.out[i].len);
		if (ret < 0) {
			uk_pr_err("failed to append out buffer %d in sg\n", i);
			goto out_unlock;
		}
		out_segs++;
	}

	for (i = 0; i < h->op.in_nr; i++) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.in[i].buf,
				h->op.in[i].len);
		if (ret < 0) {
			uk_pr_err("failed to append in buffer %d in sg\n", i);
			goto out_unlock;
		}
		in_segs++;
	}
	ret = uk_sglist_append(&vaccelq->sg, &req->status, sizeof(req->status));
	if (ret < 0) {
		uk_pr_err("failed to append status in sg\n");
		goto out_unlock;
	}
	in_segs++;

	uk_pr_debug("out_sgs = %lu, in sgs = %lu\n", out_segs, in_segs);
	ret = virtqueue_buffer_enqueue(vaccelq->vq, req, &vaccelq->sg,
				      out_segs, in_segs);
	if (ret < 0) {
		uk_pr_err("Failed to enqueue buffer");
		goto out_unlock;
	}
	ret = -EINPROGRESS;
	virtqueue_host_notify(vaccelq->vq);
out_unlock:
	ukplat_spin_unlock_irqrestore(&vaccelq->lock, flags);
	return ret;
}

int vaccel_send_request(struct virtio_accel *vaccel, struct virtio_accel_hdr *h, 
			struct virtio_accel_req *req, __u32 sid)
{
	struct virtio_accel_vq *vaccelq = vaccel->vq;
	size_t in_segs = 0, out_segs = 0;
	int ret = 0;
	unsigned long flags;
	unsigned int i;

	ukplat_spin_lock_irqsave(&vaccelq->lock, flags);

	if (virtqueue_is_full(vaccelq->vq)) {
		uk_pr_err("The virtqueue is full\n");
		ret = -ENOSPC;
		goto out_unlock;
	}

	uk_sglist_reset(&vaccelq->sg);
	ret = uk_sglist_append(&vaccelq->sg, h, sizeof(struct virtio_accel_hdr));
	if (ret < 0) {
		uk_pr_err("failed to append header in sg\n");
		goto out_unlock;
	}
	out_segs++;

	if (h->op.out_nr) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.out,
				h->op.out_nr*sizeof(*h->op.out));
		if (ret < 0) {
			uk_pr_err("failed to append out args in sg\n");
			goto out_unlock;
		}
		out_segs++;
	}

	if (h->op.in_nr) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.in,
				h->op.in_nr*sizeof(*h->op.in));
		if (ret < 0) {
			uk_pr_err("failed to append inatgs in sg\n");
			goto out_unlock;
		}
		out_segs++;
	}
	for (i = 0; i < h->op.out_nr; i++) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.out[i].buf,
				h->op.out[i].len);
		if (ret < 0) {
			uk_pr_err("failed to append out buffer %d in sg\n", i);
			goto out_unlock;
		}
		out_segs++;
	}

	for (i = 0; i < h->op.in_nr; i++) {
		ret = uk_sglist_append(&vaccelq->sg, h->op.in[i].buf,
				h->op.in[i].len);
		if (ret < 0) {
			uk_pr_err("failed to append in buffer %d in sg\n", i);
			goto out_unlock;
		}
		in_segs++;
	}
	ret = uk_sglist_append(&vaccelq->sg, &sid, sizeof(__u32));
	if (ret < 0) {
		uk_pr_err("failed to append session id in sg\n");
		goto out_unlock;
	}
	in_segs++;
	ret = uk_sglist_append(&vaccelq->sg, &req->status, sizeof(req->status));
	if (ret < 0) {
		uk_pr_err("failed to append status in sg\n");
		goto out_unlock;
	}	
	in_segs++;

	uk_pr_debug("out_sgs = %lu, in sgs = %lu\n", out_segs, in_segs);
	ret = virtqueue_buffer_enqueue(vaccelq->vq, req, &vaccelq->sg,
				      out_segs, in_segs);
	virtqueue_host_notify(vaccelq->vq);
out_unlock:
	ukplat_spin_unlock_irqrestore(&vaccelq->lock, flags);
	return ret;
}

static int virtaccel_dataq_callback(struct virtqueue *vq, void *priv)
{
	struct virtio_accel *vaccel;
	struct virtio_accel_req *req = NULL;
	struct virtio_accel_vq *vaccelq;
	uint32_t len;
	int rc = 0;

	vaccel = priv;
	vaccelq = vaccel->vq;
	UK_ASSERT(vq == vaccelq->vq);
	ukarch_spin_lock(&vaccelq->lock);
	while(1) {
		rc = virtqueue_buffer_dequeue(vaccelq->vq, (void **)&req, &len);
		uk_pr_debug("dataq callback: status = %u\n", req->status);
		if (rc < 0) {
			break;
		}

		switch (req->status) {
		case VIRTIO_ACCEL_OK:
			UK_WRITE_ONCE(req->ret, 0);
			break;
		case VIRTIO_ACCEL_INVSESS:
		case VIRTIO_ACCEL_ERR:
			req->ret = -EINVAL;
			break;
		case VIRTIO_ACCEL_BADMSG:
			req->ret = -EBADMSG;
			break;
		default:
			req->ret = -EIO;
			break;
		}	
		ukarch_spin_unlock(&vaccelq->lock);
		uk_waitq_wake_up(&req->completion);
		ukarch_spin_lock(&vaccelq->lock);
		if (rc == 0) {
			break;
		}
	}
	ukarch_spin_unlock(&vaccelq->lock);
	return 1;
}

static int virtaccel_find_vqs(struct virtio_accel *vaccel)
{
	int vq_avail = 0;
	int rc = 0;
	__u16 qdesc_size;

	vq_avail = virtio_find_vqs(vaccel->vdev, 1, &qdesc_size);
	if (unlikely(vq_avail != 1)) {
		uk_pr_err(DRIVER_NAME"Could not find vq\n");
		rc = -EINVAL;
		goto vq_err;
	}

	uk_sglist_init(&vaccel->vq->sg, 1024, &vaccel->vq->sgsegs[0]);

	vaccel->vq->vq = virtio_vqueue_setup(vaccel->vdev, 0, qdesc_size,
						virtaccel_dataq_callback,
						a);
	if (unlikely(PTRISERR(vaccel->vq->vq))) {
		uk_pr_err(DRIVER_NAME": Failed to set up virtqueue\n");
		rc = PTR2ERR(vaccel->vq->vq);
	}

	vaccel->vq->vq->priv = vaccel;
vq_err:
	return rc;
}

static int virtaccel_alloc_queues(struct virtio_accel *vaccel)
{
	vaccel->vq = uk_zalloc(a, sizeof(*vaccel->vq));
	if (!vaccel->vq)
		return -ENOMEM;

	return 0;
}

static void virtaccel_free_queues(struct virtio_accel *vaccel)
{
	uk_free(a, vaccel->vq);
}

static int virtaccel_init_vqs(struct virtio_accel *vaccel)
{
	int ret;

	ret = virtaccel_alloc_queues(vaccel);
	if (ret)
		goto err;

	ret = virtaccel_find_vqs(vaccel);
	if (ret)
		goto err_free;

	ukarch_spin_lock_init(vaccel->vq->lock);
	return 0;

err_free:
	virtaccel_free_queues(vaccel);
err:
	return ret;
}

static int virtaccel_update_status(struct virtio_accel *vaccel)
{
	u32 status;
	int err;

	err = virtio_config_get(vaccel->vdev,
			__offsetof(struct virtio_accel_conf, status),
			  &status, sizeof(status), 1);
	if (err < 0)
		return -EAGAIN;

	if (status & (~VIRTIO_ACCEL_S_HW_READY)) {
		uk_pr_err("Unknown status bits: 0x%x\n", status);
		return -EPERM;
	}

	if (vaccel->status == status)
		return 0;

	vaccel->status = status;
	if (vaccel->status & VIRTIO_ACCEL_S_HW_READY)
		uk_pr_info("Accelerator is ready\n");
	return 0;
}

static int virtio_accel_start(struct virtio_accel *d)
{
	virtqueue_intr_enable(d->vq->vq);
	virtio_dev_drv_up(d->vdev);
	uk_pr_info(DRIVER_NAME": started\n");

	return 0;
}

static int virtio_accel_add_dev(struct virtio_dev *vdev)
{
	struct virtio_accel *vaccel;
	int rc = 0;

	UK_ASSERT(vdev != NULL);

	vaccel = uk_zalloc(a, sizeof(*vaccel));
	if (!vaccel)
		return -ENOMEM;

	vaccel->vdev = vdev;
	vaccel->chr_dev = accel_dev_init();
	if (!vaccel->chr_dev) {
		uk_pr_err("Failed to register virtio_blk device: %d\n", rc);
		goto err_out;
	}
	vaccel->chr_dev->private_data = vaccel;

	vaccel->vdev->features = virtio_feature_get(vaccel->vdev);

	rc = virtaccel_init_vqs(vaccel);
	if (rc) {
		uk_pr_err("Failed to initialize vqs\n");
		goto err_out;
	}

	rc = virtaccel_update_status(vaccel);
	if (rc) {
		uk_pr_err("Failed to update virtio status %d\n", rc);
		goto err_status;
	}

	virtio_accel_start(vaccel);
out:
	return 0;
err_status:
	virtio_dev_status_update(vaccel->vdev, VIRTIO_CONFIG_STATUS_FAIL);
err_out:
	uk_free(a, vaccel);
	goto out;
}

static int virtio_accel_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;
	return 0;
}

static const struct virtio_dev_id vaccel_dev_id[] = {
	{VIRTIO_ID_ACCEL},
	{VIRTIO_ID_INVALID} /* List Terminator */
};

static struct virtio_driver vaccel_drv = {
	.dev_ids = vaccel_dev_id,
	.init    = virtio_accel_drv_init,
	.add_dev = virtio_accel_add_dev
};

VIRTIO_BUS_REGISTER_DRIVER(&vaccel_drv);
