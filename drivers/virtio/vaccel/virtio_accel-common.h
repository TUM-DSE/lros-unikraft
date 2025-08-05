#ifndef _VIRTIO_ACCEL_COMMON_H
#define _VIRTIO_ACCEL_COMMON_H

#include <uk/sglist.h>
#include <sys/types.h>
#include <uk/arch/spinlock.h>
#include <uk/wait.h>
#include "virtio_accel.h"

#define u32 __u32

#define VQ_SGSEGS	1024

struct virtio_accel_vq {
	struct virtqueue *vq;
	struct uk_sglist sg;
	struct uk_sglist_seg sgsegs[VQ_SGSEGS];
	spinlock_t lock;
};

struct virtio_accel {
	struct virtio_dev *vdev;
	struct virtio_accel_vq *vq;
	struct device *chr_dev;
	unsigned long status;
};

struct virtio_accel_req {
	struct virtio_accel_hdr hdr;
	struct virtio_accel *vaccel;
	void *priv;
	void __user *usr;
	struct uk_waitq completion;
	u32 status;
	int ret;
};

int virtaccel_req_gen_create_session(struct virtio_accel_req *req);
int virtaccel_req_gen_destroy_session(struct virtio_accel_req *req);
int virtaccel_req_gen_operation(struct virtio_accel_req *req);
void virtaccel_clear_req(struct virtio_accel_req *req);
void virtaccel_handle_req_result(struct virtio_accel_req *req);

void *kzalloc_node(ssize_t s);
void kfree_node(void *p);
int vaccel_send_request(struct virtio_accel *, struct virtio_accel_hdr *, 
			struct virtio_accel_req *, __u32 );
int vaccel_send_request_op(struct virtio_accel *, struct virtio_accel_hdr *, 
			struct virtio_accel_req *);
int vaccel_send_request_destroy(struct virtio_accel *, struct virtio_accel_hdr *,
			struct virtio_accel_req *);
#endif /* _VIRTIO_ACCEL_COMMON_H */
