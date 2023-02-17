#ifndef __VACCEL_VIRTIO_OPERATIONS_H__
#define __VACCEL_VIRTIO_OPERATIONS_H__

#include <stddef.h>
#include <stdint.h>

struct vaccel_session;

int virtio_noop(struct vaccel_session *sess);
int virtio_sgemm(struct vaccel_session *sess, uint32_t m, uint32_t n, uint32_t k,
		float alpha, float *a, size_t len_a __unused, float *b,
		size_t len_b __unused, float beta, float *c, size_t len_c __unused);
int virtio_minmax(
	struct vaccel_session *sess,
	const double *indata, int ndata,
	int low_threshold, int high_threshold,
	double *out_data, double *min, double *max);
//int virtio_sgemm(struct vaccel_session *sess,
//		uint32_t k, uint32_t m, uint32_t n,
//		size_t len_a, size_t len_b, size_t len_c,
//		float *a, float *b, float *c);
int virtio_image_op(enum vaccel_op_type op_type, struct vaccel_session *sess,
		const void *img, unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname);
int virtio_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img,	size_t len_out_text, size_t len_out_imgname);

int virtio_image_detection(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname);

int virtio_image_segmentation(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname);

int virtio_image_depth(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname);

int virtio_image_pose(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img, size_t len_out_imgname);

int virtio_exec(struct vaccel_session *sess, const char *library, const char
		*fn_symbol, void *out_args, size_t out_nargs, void *in_args,
		size_t in_nargs);

#endif /* __VACCEL_VIRTIO_OPERATIONS_H__ */
