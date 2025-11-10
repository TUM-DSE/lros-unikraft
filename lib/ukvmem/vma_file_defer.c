/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "vmem.h"

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/falloc.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#endif /* CONFIG_HAVE_PAGING */
#include <vfscore/file.h>
#include <vfscore/vnode.h>
#include <vfscore/uio.h>

static __noreturn void defer_file_load(void *data)
{
	/*
	 * Read blockwise into buffer
	 */
	struct uk_vma_file_defer *vma_file = (struct uk_vma_file_defer *)data;
	const int block_size = PAGE_SIZE;
	struct vnode *vp = vma_file->f->f_dentry->d_vnode;
	__off *arr = vma_file->arr;
	__off *arr_p = vma_file->arr_p;
	__sz len = ALIGN_UP(vma_file->len, block_size);
	__vaddr_t buf = vma_file->buf;
	__sz buf_len = ALIGN_DOWN(vma_file->buf_len, block_size);

	for (__sz off_file = 0, off_buf = 0;; off_file += block_size,
		  off_file %= len, off_buf += block_size, off_buf %= buf_len) {
		struct iovec iovec = {
		    .iov_base = (void *)(buf + off_buf),
		    .iov_len = block_size,
		};
		struct uio uio = {
		    .uio_iov = &iovec,
		    .uio_iovcnt = 1,
		    .uio_offset = vma_file->offset + off_file,
		    .uio_resid = block_size,
		    .uio_rw = UIO_READ,
		};
		int rc;

		if (arr[off_file / PAGE_SIZE] >= 0) {
			// Already there
			if (len <= buf_len) {
				if (vma_file->waiting_thread)
					uk_thread_wake(vma_file->waiting_thread);
				uk_thread_exit();
			}
			// TODO Proper policy
			continue;
		}

		// Respect blocksize
		arr[off_file / PAGE_SIZE] = -2;

		__off prev = arr_p[off_buf / PAGE_SIZE];

		if (prev >= 0) {
			// Respect blocksize
			arr[prev / PAGE_SIZE] = -1;
			// Unmap
			ukplat_page_unmap(
		    		vma_file->base.vas->pt, vma_file->base.start + prev,
		    		block_size >> PAGE_SHIFT, PAGE_FLAG_KEEP_PTES);
		}
		vn_lock(vp);
		rc = VOP_READ(vp, vma_file->f, &uio, 0);
		vn_unlock(vp);

		// Respect blocksize
		arr[off_file / PAGE_SIZE] = off_buf;

		arr_p[off_buf / PAGE_SIZE] = off_file;

		if (vma_file->waiting_thread)
			uk_thread_wake(vma_file->waiting_thread);
		if (vma_file->exit)
			uk_thread_exit();
		uk_sched_yield(); // Allow other threads to progress
	}
}

int vma_op_file_defer_new(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
			  void *data, unsigned long attr, unsigned long *flags,
			  struct uk_vma **vma)
{
	struct uk_vma_file_args *args = (struct uk_vma_file_args *)data;
	struct uk_vma_file_defer *vma_file;

	UK_ASSERT(data);
	UK_ASSERT(args->fd >= 0);
	UK_ASSERT(args->offset >= 0);
	UK_ASSERT(PAGE_ALIGNED(args->offset));

	*flags &= ~UK_VMA_MAP_POPULATE;

	/* Writable shared mappings are not supported.
	 * Read-only shared mappings are partially supported.
	 *
	 * We treat read-only shared mappings as private. Note that any writes
	 * to the underlying file while the mapping is established will not be
	 * reflected in memory.
	 */
	if ((*flags & UK_VMA_FILE_SHARED) && (attr & PAGE_ATTR_PROT_WRITE))
		return -ENOTSUP;

	vma_file = uk_malloc(vas->a, sizeof(struct uk_vma_file_defer));
	if (unlikely(!vma_file))
		return -ENOMEM;

	vma_file->f = vfscore_get_file(args->fd);
	if (unlikely(!vma_file->f)) {
		uk_free(vas->a, vma_file);
		return -EBADF;
	}
	vma_file->offset = args->offset;

	/* Use the file name as VMA name. Since the memory management of the
	 * string is tied to the file object, we do not need to care about
	 * freeing it. So it is ok, if the caller should override the name.
	 */
	vma_file->base.name = vma_file->f->f_dentry->d_path;

	UK_ASSERT(vma);
	*vma = &vma_file->base;

	__sz arr_len = PAGE_ALIGN_UP(len) * sizeof(__off) / PAGE_SIZE;

	__off *arr = uk_malloc(vas->a, arr_len);
	if (unlikely(!arr)) {
		uk_free(vas->a, vma_file);
		return -ENOMEM;
	}

	memset(arr, -1, arr_len);

	__sz buf_len = 512LLU * 1024LLU * 1024LLU ;

	__paddr_t buf = uk_falloc(vas->pt->fa, buf_len / PAGE_SIZE);
	if (unlikely(buf == __PADDR_INV)) {
		uk_free(vas->a, vma_file);
		uk_free(vas->a, arr);
		return -ENOMEM;
	}

	__vaddr_t vaddr1 =
	    ukplat_page_kmap(vas->pt, buf, buf_len / PAGE_SIZE, 0);

	__off *arr_p = uk_malloc(vas->a, sizeof(__off) * buf_len / PAGE_SIZE);
	if (unlikely(!arr_p)) {
		uk_free(vas->a, vma_file);
		uk_free(vas->a, arr);
		uk_ffree(vas->pt->fa, buf, buf_len / PAGE_SIZE);
		return -ENOMEM;
	}

	memset(arr_p, -1, sizeof(__off) * buf_len / PAGE_SIZE);

	vma_file->arr = arr;
	vma_file->arr_p = arr_p;
	vma_file->len = PAGE_ALIGN_UP(len);
	vma_file->buf = vaddr1;
	vma_file->buf_p = buf;
	vma_file->buf_len = buf_len;
	vma_file->waiting_thread = NULL;
	vma_file->exit = false;

	// Temporarily add vma
	vma_file->base.start = vaddr;
	vma_file->base.end = vaddr + len;
	vma_file->base.vas = vas;

	struct uk_list_head *prev;
	struct uk_vma *cur;

	prev = &vas->vma_list;
	uk_list_for_each_entry(cur, &vas->vma_list, vma_list)
	{
		if (vma_file->base.start < cur->end) {
			UK_ASSERT(vma_file->base.end <= cur->start);

			uk_list_add(&vma_file->base.vma_list, prev);
			break;
		}

		prev = &cur->vma_list;
	}

	uk_list_add_tail(&vma_file->base.vma_list, &vas->vma_list);

	struct uk_thread *preload_thread = uk_sched_thread_create(
	    uk_sched_current(), defer_file_load, vma_file, "File map preload");

	// Remove again
	uk_list_del(&vma_file->base.vma_list);

	vma_file->preload_thread = preload_thread;

	return 0;
}
