// SPDX-License-Identifier: Apache-2.0

//#include "arg.h"
#include "error.h"
#include "log.h"
#include "matmul.h"
#include "vaccel_ops.h"
//#include "plugin.h"
//#include "prof.h"
#include "genop.h"
#include "session.h"
#include "../operations.h"
#include <inttypes.h>
#include <stdint.h>

#define _unused __attribute__((unused))


int vaccel_matmul_create(struct vaccel_session *sess, vaccel_matmul_ctx* ctx,
			vaccel_matmul_info* info, vaccel_matmul_io_attr* io_attr)
{
    return virtio_matmul_create(sess, ctx, info, io_attr);
}

int vaccel_matmul_create_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error("Wrong number of read arguments in matmul_create: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 2) {
		vaccel_error("Wrong number of write arguments in matmul_create: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_matmul_info* info = read[0].buf;

	vaccel_matmul_ctx* ctx = write[0].buf;
	vaccel_matmul_io_attr* io_attr = write[1].buf;

	return vaccel_matmul_create(sess, ctx, info, io_attr);
}


int vaccel_create_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx, uint32_t size, vaccel_tensor_mem* result)
{
	return virtio_create_mem(sess, ctx, size, result);
}

int vaccel_create_mem_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in create_mem: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in create_mem: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_matmul_ctx ctx = *(vaccel_matmul_ctx*)read[0].buf;
	uint32_t size = *(uint32_t*)read[1].buf;

	vaccel_tensor_mem* result = write[0].buf;

	return vaccel_create_mem(sess, ctx, size, result);
}

int vaccel_destroy_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx, vaccel_tensor_mem *mem)
{
    return virtio_destroy_mem(sess, ctx, mem);
}

int vaccel_destroy_mem_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, _unused struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in destroy_mem: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 0) {
		vaccel_error("Wrong number of write arguments in destroy_mem: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_matmul_ctx ctx = *(vaccel_matmul_ctx*)read[0].buf;
	vaccel_tensor_mem* mem = read[1].buf;


	return vaccel_destroy_mem(sess, ctx, mem);
}

int vaccel_matmul_destroy(struct vaccel_session *sess, vaccel_matmul_ctx ctx)
{
    return virtio_matmul_destroy(sess, ctx);
}

int vaccel_matmul_destroy_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, _unused struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error("Wrong number of read arguments in matmul_destroy: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 0) {
		vaccel_error("Wrong number of write arguments in matmul_destroy: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_matmul_ctx ctx = *(vaccel_matmul_ctx*)read[0].buf;


	return vaccel_matmul_destroy(sess, ctx);
}

int vaccel_matmul_set_io_mem(struct vaccel_session *sess, vaccel_matmul_ctx ctx, vaccel_tensor_mem_handle* mem, vaccel_matmul_tensor_attr* attr)
{
    return virtio_matmul_set_io_mem(sess, ctx, mem, attr);
}

int vaccel_matmul_set_io_mem_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, _unused struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 3) {
		vaccel_error("Wrong number of read arguments in matmul_set_io_mem: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 0) {
		vaccel_error("Wrong number of write arguments in matmul_set_io_mem: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_matmul_ctx ctx = *(vaccel_matmul_ctx*)read[0].buf;
	vaccel_tensor_mem_handle* mem = *(vaccel_tensor_mem_handle**)read[1].buf;
	vaccel_matmul_tensor_attr* attr = read[2].buf;


	return vaccel_matmul_set_io_mem(sess, ctx, mem, attr);
}

int vaccel_matmul_set_core_mask(struct vaccel_session *sess, vaccel_matmul_ctx ctx, vaccel_core_mask core_mask)
{
    return virtio_matmul_set_core_mask(sess, ctx, core_mask);
}

int vaccel_matmul_set_core_mask_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, _unused struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in matmul_set_core_mask: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 0) {
		vaccel_error("Wrong number of write arguments in matmul_set_core_mask: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_matmul_ctx ctx = *(vaccel_matmul_ctx*)read[0].buf;
	vaccel_core_mask core_mask = *(vaccel_core_mask*)read[1].buf;


	return vaccel_matmul_set_core_mask(sess, ctx, core_mask);
}

int vaccel_matmul_run(struct vaccel_session *sess, vaccel_matmul_ctx ctx)
{
    return virtio_matmul_run(sess, ctx);
}

int vaccel_matmul_run_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, _unused struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error("Wrong number of read arguments in matmul_run: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 0) {
		vaccel_error("Wrong number of write arguments in matmul_run: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_matmul_ctx ctx = *(vaccel_matmul_ctx*)read[0].buf;


	return vaccel_matmul_run(sess, ctx);
}

int vaccel_matmul_set_matrix(struct vaccel_session *sess, vaccel_tensor_mem_handle* dst, void* src, size_t nbytes)
{
	return virtio_matmul_set_matrix(sess, dst, src, nbytes);
}

int vaccel_matmul_set_matrix_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
				    int nr_read, _unused struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 3) {
		vaccel_error("Wrong number of read arguments in matmul_set_matrix: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 0) {
		vaccel_error("Wrong number of write arguments in matmul_set_matrix: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_tensor_mem_handle* dst = *(vaccel_tensor_mem_handle**)read[0].buf;
	void* src = read[1].buf;
	size_t nbytes = *(size_t*)read[2].buf;


	return vaccel_matmul_set_matrix(sess, dst, src, nbytes);
}

int vaccel_matmul_get_matrix(struct vaccel_session *sess, void* dst, vaccel_tensor_mem_handle* src, size_t nbytes)
{
	return virtio_matmul_get_matrix(sess, dst, src, nbytes);
}

int vaccel_matmul_get_matrix_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
				    int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in matmul_set_matrix: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in matmul_set_matrix: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	vaccel_tensor_mem_handle* src = *(vaccel_tensor_mem_handle**)read[0].buf;
	size_t nbytes = *(size_t*)read[1].buf;

	void* dst = write[0].buf;

	return vaccel_matmul_get_matrix(sess, dst, src, nbytes);
}

int vaccel_matmul_get_props(struct vaccel_session *sess, char *props,
			    size_t nbytes)
{
	return virtio_matmul_get_props;
}

int vaccel_matmul_get_props_unpack(struct vaccel_session *sess,
				   struct vaccel_arg *read, int nr_read,
				   struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error(
		    "Wrong number of read arguments in matmul_get_props: %d",
		    nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error(
		    "Wrong number of write arguments in matmul_get_props: %d",
		    nr_write);
		return VACCEL_EINVAL;
	}

	size_t nbytes = *(size_t *)read[0].buf;

	void *props = write[0].buf;

	return vaccel_matmul_get_props(sess, props, nbytes);
}


__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
//	vaccel_prof_region_print(&rknn_op_stats);
//	vaccel_prof_region_release(&rknn_op_stats);
}
