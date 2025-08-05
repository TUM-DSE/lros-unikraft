#include <string.h>

#include <accel.h>
#include "virtio_accel-common.h"

static int virtaccel_prepare_args(struct virtio_accel_arg **vargs,
		struct accel_arg *user_arg, uint32_t nr_args)
{
	struct accel_arg *args;
	struct virtio_accel_arg *v;
	unsigned int i;

	if (!nr_args) {
		*vargs = NULL;
		return 0;
	}

	v = kzalloc_node(nr_args * sizeof(*v));
	if (!v)
		return -ENOMEM;

	args = (struct accel_arg *) user_arg;

	for (i = 0; i < nr_args; ++i) {
		v[i].len = args[i].len;
		v[i].usr_buf = args[i].buf;
		v[i].buf = kzalloc_node(args[i].len);;
		if (!v[i].buf)
			goto free_vargs_buf;
	}

	*vargs = v;

	return nr_args + 1;

free_vargs_buf:
	nr_args = i;
	for (i = 0; i < nr_args; ++i)
		kfree_node(v[i].buf);
	kfree_node(v);
	return -ENOMEM;
}

static int virtaccel_copy_args(struct virtio_accel_arg *vargs, uint32_t nr_args)
{
	unsigned int i;

	for (i = 0; i < nr_args; ++i)
		memcpy(vargs[i].buf, vargs[i].usr_buf, vargs[i].len);

	return 0;
}

static void virtaccel_cleanup_args(struct virtio_accel_arg *vargs, u32 nr_args)
{
	unsigned int i;

	if (!vargs)
		return;

	for (i = 0; i < nr_args; ++i)
		kfree_node(vargs[i].buf);

	kfree_node(vargs);
}

static int virtaccel_prepare_request(uint32_t op_type,
				     struct virtio_accel_hdr *virtio,
				     struct accel_session *usr_sess)
{
	struct accel_op *op = &usr_sess->op;
	int ret, total_sgs = 0;

	virtio->sess_id = usr_sess->id;
	virtio->op_type = op_type;
	virtio->op.in_nr = op->in_nr;
	virtio->op.out_nr = op->out_nr;

	ret = virtaccel_prepare_args(&virtio->op.in, usr_sess->op.in,
			virtio->op.in_nr);
	if (ret < 0)
		return ret;

	total_sgs += ret;

	ret = virtaccel_prepare_args(&virtio->op.out, usr_sess->op.out,
			virtio->op.out_nr);
	if (ret < 0)
		goto free_in;

	total_sgs += ret;

	ret = virtaccel_copy_args(virtio->op.out, virtio->op.out_nr);
	if (ret < 0)
		goto free_out;

	return total_sgs;

free_out:
	virtaccel_cleanup_args(virtio->op.out, virtio->op.out_nr);
free_in:
	virtaccel_cleanup_args(virtio->op.in, virtio->op.in_nr);
	return ret;
}

int virtaccel_req_gen_operation(struct virtio_accel_req *req)
{
	struct virtio_accel *vaccel = req->vaccel;
	struct virtio_accel_hdr *h = &req->hdr;
	struct accel_session *sess = req->priv;
	int ret;

	ret = virtaccel_prepare_request(VIRTIO_ACCEL_DO_OP, h, sess);
	if (ret < 0)
		return ret;

	ret = vaccel_send_request_op(vaccel, h, req);
	return ret;
}

int virtaccel_req_gen_destroy_session(struct virtio_accel_req *req)
{
	struct virtio_accel *vaccel = req->vaccel;
	struct virtio_accel_hdr *h = &req->hdr;
	struct accel_session *sess = req->priv;
	int ret;

	h->op_type = VIRTIO_ACCEL_DESTROY_SESSION;
	h->sess_id = sess->id;

	ret = vaccel_send_request_destroy(vaccel, h, req);

	return ret;
}

int virtaccel_req_gen_create_session(struct virtio_accel_req *req)
{
	struct virtio_accel *vaccel = req->vaccel;
	struct virtio_accel_hdr *h = &req->hdr;
	struct accel_session *sess = req->priv;
	int ret;

	ret = virtaccel_prepare_request(VIRTIO_ACCEL_CREATE_SESSION, h, sess);
	if (ret < 0)
		return ret;
	ret = vaccel_send_request(vaccel, h, req, sess->id);
	return ret;
}

static int virtaccel_write_user_output(struct virtio_accel_arg *varg, uint32_t nr_arg)
{
	unsigned int i = 0;

	if (!nr_arg)
		return 0;

	for (i = 0; i < nr_arg; ++i) {
		memcpy(varg[i].usr_buf, varg[i].buf, varg[i].len);
	}

	return 0;
}

void virtaccel_handle_req_result(struct virtio_accel_req *req)
{
	struct virtio_accel_hdr *h = &req->hdr;
	struct accel_session *sess;
	int ret;

	if (req->status != VIRTIO_ACCEL_OK) {
		uk_pr_err("request status not ok\n");
		return;
	}

	switch (h->op_type) {
	case VIRTIO_ACCEL_CREATE_SESSION:
		sess = req->priv;
		uk_pr_debug("handle req: succesfully created session %u\n", sess->id);
		ret = virtaccel_write_user_output(h->op.in, h->op.in_nr);
		if (ret) {
			req->ret = -EINVAL;
			return;
		}
		/*
		 * We use the user struct, no need to memcpy back
		 */
		//memcpy(req->usr, sess, sizeof(*sess));
		break;
	case VIRTIO_ACCEL_DESTROY_SESSION:
		uk_pr_debug(" handle req result destroy session\n");
		break;
	case VIRTIO_ACCEL_DO_OP:
		uk_pr_debug(" handle req result do op\n");
		ret = virtaccel_write_user_output(h->op.in, h->op.in_nr);
		if (ret) {
			req->ret = -EINVAL;
			return;
		}
		break;
	default:
		uk_pr_err("hadle req: invalid op returned\n");
		req->ret = -EBADMSG;
		break;
	}	
}

void virtaccel_clear_req(struct virtio_accel_req *req)
{
	struct virtio_accel_hdr *h = &req->hdr;

	switch (h->op_type) {
	case VIRTIO_ACCEL_CREATE_SESSION:
	case VIRTIO_ACCEL_DO_OP:
		uk_pr_debug("clear req create/op session\n");
		virtaccel_cleanup_args(h->op.out, h->op.out_nr);
		virtaccel_cleanup_args(h->op.in, h->op.in_nr);
		break;
	case VIRTIO_ACCEL_DESTROY_SESSION:
		uk_pr_debug(" clear req gen create/destroy session\n");
		break;
	default:
		uk_pr_err("clear req: invalid op returned\n");
		break;
	}
	req->usr = NULL;
}
