#pragma once

#include "../include/ops/matmul.h"

struct vaccel_session;
struct vaccel_arg;


int vaccel_matmul_create_unpack(struct vaccel_session *sess,
				struct vaccel_arg *read, int nr_read,
				struct vaccel_arg *write, int nr_write);

int vaccel_create_mem_unpack(struct vaccel_session *sess,
			     struct vaccel_arg *read, int nr_read,
			     struct vaccel_arg *write, int nr_write);

int vaccel_destroy_mem_unpack(struct vaccel_session *sess,
			      struct vaccel_arg *read, int nr_read,
			      struct vaccel_arg *write, int nr_write);

int vaccel_matmul_destroy_unpack(struct vaccel_session *sess,
				 struct vaccel_arg *read, int nr_read,
				 struct vaccel_arg *write, int nr_write);

int vaccel_matmul_set_io_mem_unpack(struct vaccel_session *sess,
				    struct vaccel_arg *read, int nr_read,
				    struct vaccel_arg *write, int nr_write);

int vaccel_matmul_set_core_mask_unpack(struct vaccel_session *sess,
				       struct vaccel_arg *read, int nr_read,
				       struct vaccel_arg *write, int nr_write);

int vaccel_matmul_run_unpack(struct vaccel_session *sess,
			     struct vaccel_arg *read, int nr_read,
			     struct vaccel_arg *write, int nr_write);

int vaccel_matmul_set_matrix_unpack(struct vaccel_session *sess,
				    struct vaccel_arg *read, int nr_read,
				    struct vaccel_arg *write, int nr_write);

int vaccel_matmul_get_matrix_unpack(struct vaccel_session *sess,
				    struct vaccel_arg *read, int nr_read,
				    struct vaccel_arg *write, int nr_write);
