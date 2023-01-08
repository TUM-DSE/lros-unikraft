#ifndef __VACCEL_VIRTIO_OPERATIONS_H__
#define __VACCEL_VIRTIO_OPERATIONS_H__

#include <stddef.h>
#include <stdint.h>

struct vaccel_session;

int virtio_noop(struct vaccel_session *sess);
int virtio_sgemm(struct vaccel_session *sess, long long int m, long long int n,
		long long int k, float alpha, float *a, long long int lda, float *b,
		long long int ldb, float beta, float *c, long long int ldc);
//int virtio_sgemm(struct vaccel_session *sess,
//		uint32_t k, uint32_t m, uint32_t n,
//		size_t len_a, size_t len_b, size_t len_c,
//		float *a, float *b, float *c);
int virtio_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img,	size_t len_out_text, size_t len_out_imgname);

int virtio_image_detection(struct vaccel_session *sess, void *img,
		char *out_imgname, size_t len_img, size_t len_out_imgname);

int virtio_image_segmentation(struct vaccel_session *sess, void *img,
		char *out_imgname, size_t len_img, size_t len_out_imgname);


#endif /* __VACCEL_VIRTIO_OPERATIONS_H__ */
