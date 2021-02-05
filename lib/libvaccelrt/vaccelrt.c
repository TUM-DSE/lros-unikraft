#include <uk/print.h>
#include <vaccel.h>
#include "operations.h"
#include "operations.h"
#include "session.h"
#include "log.h"

int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	return virtio_sess_init(sess, flags);
}

int vaccel_sess_free(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	return virtio_sess_free(sess);
}

int vaccel_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing image classification\n",
			sess->session_id);

	return virtio_image_classification(sess, img, out_text, out_imgname, len_img,
			len_out_text, len_out_imgname);
}
