//
// Created by simon on 23.09.25.
//
#include <vaccel.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <accel.h>
#include <string.h>

#include "ioctl.h"
#include "log.h"
#include "session.h"
#include "operations_matmul.h"
int virtio_matmul_create(struct vaccel_session *sess, vaccel_matmul_ctx *ctx,
			 vaccel_matmul_info *info,
			 vaccel_matmul_io_attr *io_attr)
{
	enum vaccel_op_type op_type = VACCEL_MATMUL_CREATE;
	struct accel_session vsess = { 0 };
	struct accel_arg args[4] = {
	    { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_info), (unsigned char *)info, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_ctx), (unsigned char *)ctx, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_io_attr), (unsigned char *)io_attr, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = args;
	vsess.op.in_nr = 2;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing matmul create",
		     sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_create_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx,
		      uint32_t size, vaccel_tensor_mem *result)
{
	enum vaccel_op_type op_type = VACCEL_CREATE_MEM;
	struct accel_session vsess = { 0 };
	struct accel_arg args[4] = {
	    { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_ctx), (unsigned char *)&ctx, NULL, 0, {0} },
	    { sizeof(size), (unsigned char *)&size, NULL, 0, {0} },
	    { sizeof(vaccel_tensor_mem), (unsigned char *)result, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 3;
	vsess.op.out = args;
	vsess.op.in_nr = 1;
	vsess.op.in = &args[3];

	vaccel_debug("[virtio] session:%u Executing create mem",
		     sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_destroy_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx,
		       vaccel_tensor_mem *mem)
{
	enum vaccel_op_type op_type = VACCEL_DESTROY_MEM;
	struct accel_session vsess = { 0 };
	struct accel_arg args[3] = {
	    { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_ctx), (unsigned char *)&ctx, NULL, 0, {0} },
	    { sizeof(vaccel_tensor_mem), (unsigned char *)&mem, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 3;
	vsess.op.out = args;
	vsess.op.in_nr = 0;
	vsess.op.in = &args[3];

	vaccel_debug("[virtio] session:%u Executing destroy mem",
		     sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_matmul_destroy(struct vaccel_session *sess, vaccel_matmul_ctx ctx)
{
	enum vaccel_op_type op_type = VACCEL_DESTROY_MEM;
	struct accel_session vsess = { 0 };
	struct accel_arg args[2] = {
	    { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_ctx), (unsigned char *)&ctx, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = args;
	vsess.op.in_nr = 0;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing matmul destroy",
		     sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_matmul_set_io_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx,
			     vaccel_tensor_mem_handle *mem,
			     vaccel_matmul_tensor_attr *attr)
{
	enum vaccel_op_type op_type = VACCEL_MATMUL_SET_IO;
	struct accel_session vsess = { 0 };
	struct accel_arg args[4] = {
	    { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_ctx), (unsigned char *)&ctx, NULL, 0, {0} },
	    { sizeof(vaccel_tensor_mem_handle*), (unsigned char *)&mem, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_tensor_attr), (unsigned char *)attr, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 4;
	vsess.op.out = args;
	vsess.op.in_nr = 0;
	vsess.op.in = &args[4];

	vaccel_debug("[virtio] session:%u Executing set io mem",
		     sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_matmul_set_core_mask(struct vaccel_session *sess,
				vaccel_matmul_ctx ctx,
				vaccel_core_mask core_mask)
{
	enum vaccel_op_type op_type = VACCEL_MATMUL_SET_CORE_MASK;
	struct accel_session vsess = { 0 };
	struct accel_arg args[3] = {
	    { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_ctx), (unsigned char *)&ctx, NULL, 0, {0} },
	    { sizeof(vaccel_core_mask), (unsigned char *)&core_mask, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 3;
	vsess.op.out = args;
	vsess.op.in_nr = 0;
	vsess.op.in = &args[3];

	vaccel_debug("[virtio] session:%u Executing set core mask",
		     sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_matmul_run(struct vaccel_session *sess, vaccel_matmul_ctx ctx)
{
	enum vaccel_op_type op_type = VACCEL_MATMUL_RUN;
	struct accel_session vsess = { 0 };
	struct accel_arg args[2] = {
	    { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
	    { sizeof(vaccel_matmul_ctx), (unsigned char *)&ctx, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = args;
	vsess.op.in_nr = 0;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing matmul run",
		     sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}
