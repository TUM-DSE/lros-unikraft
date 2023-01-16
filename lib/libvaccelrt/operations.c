#include <vaccel.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <accel.h>

#include "ioctl.h"
#include "log.h"
#include "session.h"

int virtio_noop(struct vaccel_session *sess)
{
	unsigned int op_type = VACCEL_NO_OP;
	struct accel_session vsess = { 0 };
	struct accel_arg arg = { sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0}};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 1;
	vsess.op.out = &arg;

	vaccel_debug("[virtio] session:%u Executing noop",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_sgemm(struct vaccel_session *sess, long long int m, long long int n,
		long long int k, float alpha, float *a, long long int lda, float *b,
		long long int ldb, float beta, float *c, long long int ldc)
{
	unsigned int op_type = VACCEL_BLAS_SGEMM;
	struct accel_session vsess = { 0 };
	struct accel_arg args[9] = {
		{ sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
		{ sizeof(m), (unsigned char *)&m, NULL, 0, {0} },
		{ sizeof(n), (unsigned char *)&n, NULL, 0, {0} },
		{ sizeof(k), (unsigned char *)&k, NULL, 0, {0} },
		{ sizeof(alpha), (unsigned char *)&alpha, NULL, 0, {0} },
		{ lda, (unsigned char *)a, NULL, 0, {0} },
		{ ldb, (unsigned char *)b, NULL, 0, {0} },
		{ sizeof(beta), (unsigned char *)&beta, NULL, 0, {0} },
		{ ldc, (unsigned char *)c, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 8;
	vsess.op.out = args;
	vsess.op.in_nr = 1;
	vsess.op.in = &args[8];

	vaccel_debug("[virtio] session:%u Executing sgemm",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_image_op(enum vaccel_op_type op_type, struct vaccel_session *sess,
		const void *img, unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	struct accel_session vsess = { 0 };
	struct accel_arg args[4] = {
		{ sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
		{ len_img, (__u8 *) img, NULL, 0, {0} }
	};
	if (out_text == NULL || len_out_text == 0) {
		args[2].len = len_out_imgname;
		args[2].buf = (unsigned char *)out_imgname;

		vsess.op.in_nr = 1;
	} else {
		args[2].len = len_out_text;
		args[2].buf = (unsigned char *)out_text;
		args[3].len = len_out_imgname;
		args[3].buf = (unsigned char *)out_imgname;

		vsess.op.in_nr = 2;
	}

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = &args[0];
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing %s",
			sess->session_id, vaccel_op_type_str(op_type));

	return dev_write(VACCEL_DO_OP, &vsess);
}
#define virtio_image_op_no_text(op_type, sess, img, out_imgname, len_img, \
		len_out_imgname) \
		virtio_image_op(op_type, sess, img, NULL, out_imgname, \
				len_img, 0, len_out_imgname)

int virtio_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	return virtio_image_op(VACCEL_IMG_CLASS, sess, img, out_text,
			out_imgname, len_img, len_out_text, len_out_imgname);
}

int virtio_image_detection(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	return virtio_image_op_no_text(VACCEL_IMG_DETEC, sess, img,
			out_imgname, len_img, len_out_imgname);
}

int virtio_image_segmentation(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	return virtio_image_op_no_text(VACCEL_IMG_SEGME, sess, img,
			out_imgname, len_img, len_out_imgname);
}

int virtio_image_depth(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	return virtio_image_op_no_text(VACCEL_IMG_DEPTH, sess, img,
			out_imgname, len_img, len_out_imgname);
}

int virtio_image_pose(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	return virtio_image_op_no_text(VACCEL_IMG_POSE, sess, img,
			out_imgname, len_img, len_out_imgname);
}

#if 0
int virtio_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	unsigned int op_type = VACCEL_IMG_CLASS;
	struct accel_session vsess = { 0 };
	struct accel_arg args[4] = {
		{ sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
		{ len_img, img, NULL, 0, {0} },
		{ len_out_text, out_text, NULL, 0, {0} },
		{ len_out_imgname, out_imgname, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = &args[0];
	vsess.op.in_nr = 2;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing image classification\n",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_image_detection(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	unsigned int op_type = VACCEL_IMG_DETEC;
	struct accel_session vsess = { 0 };
	struct accel_arg args[3] = {
		{ sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
		{ len_img, img, NULL, 0, {0} },
		{ len_out_imgname, out_imgname, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = &args[0];
	vsess.op.in_nr = 1;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing image detection",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_image_segmentation(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	unsigned int op_type = VACCEL_IMG_SEGME;
	struct accel_session vsess = { 0 };
	struct accel_arg args[3] = {
		{ sizeof(op_type), (unsigned char *)&op_type, NULL, 0, {0} },
		{ len_img, img, NULL, 0, {0} },
		{ len_out_imgname, out_imgname, NULL, 0, {0} },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = &args[0];
	vsess.op.in_nr = 1;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing image segmentation",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}
#endif
