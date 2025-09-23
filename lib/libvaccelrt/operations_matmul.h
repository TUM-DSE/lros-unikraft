//
// Created by simon on 23.09.25.
//

#ifndef __VACCEL_VIRTIO_OPERATIONS_MATMUL_H__
#define __VACCEL_VIRTIO_OPERATIONS_MATMUL_H__

#include "ops/matmul.h"

struct vaccel_session;

int virtio_matmul_create(struct vaccel_session *sess, vaccel_matmul_ctx *ctx,
			 vaccel_matmul_info *info,
			 vaccel_matmul_io_attr *io_attr);

int virtio_create_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx,
		      uint32_t size, vaccel_tensor_mem *result);

int virtio_destroy_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx,
		       vaccel_tensor_mem *mem);

int virtio_matmul_destroy(struct vaccel_session *sess, vaccel_matmul_ctx ctx);

int virtio_matmul_set_io_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx,
			     vaccel_tensor_mem_handle *mem,
			     vaccel_matmul_tensor_attr *attr);

int virtio_matmul_set_core_mask(struct vaccel_session *sess,
				vaccel_matmul_ctx ctx,
				vaccel_core_mask core_mask);

int virtio_matmul_run(struct vaccel_session *sess, vaccel_matmul_ctx ctx);

#endif // __VACCEL_VIRTIO_OPERATIONS_MATMUL_H__
