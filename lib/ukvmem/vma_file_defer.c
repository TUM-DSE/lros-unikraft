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
	struct uk_vma_file_defer_thread_args *args =
	    (struct uk_vma_file_defer_thread_args *)data;
	const unsigned long block_size = args->block_size;
	struct vnode *vp = args->f->f_dentry->d_vnode;
	__off *arr = args->arr;
	__off *arr_p = args->arr_p;
	__sz len = ALIGN_UP(args->len + args->offset % block_size, block_size);
	__vaddr_t buf = args->buf;
	__sz buf_len = ALIGN_DOWN(args->buf_len, block_size);
	__off offset = ALIGN_DOWN(args->offset, block_size);

	for (__sz off_file = 0, off_buf = 0;; off_file += block_size,
		  off_file %= len, off_buf += block_size, off_buf %= buf_len) {
		struct iovec iovec = {
		    .iov_base = (void *)(buf + off_buf),
		    .iov_len = block_size,
		};
		struct uio uio = {
		    .uio_iov = &iovec,
		    .uio_iovcnt = 1,
		    .uio_offset = offset + off_file,
		    .uio_resid = block_size,
		    .uio_rw = UIO_READ,
		};

		if (arr[off_file / block_size] >= 0) {
			// Already there
			if (len <= buf_len) {
				if (args->waiting_thread)
					uk_thread_wake(args->waiting_thread);
				uk_thread_exit();
			}
			// TODO Proper policy
			continue;
		}

		arr[off_file / block_size] = -2;

		__off prev = arr_p[off_buf / block_size];

		if (prev >= 0) {
			arr[prev / block_size] = -1;
			// Unmap
			__vaddr_t vaddr =
			    prev ? args->start - (args->offset % block_size) + prev
				 : args->start;
			unsigned long ulen = (MIN(vaddr + block_size,
						  args->start + args->len)) -
					     vaddr;
			ukplat_page_unmap(
			    args->vma->vas->pt, vaddr, ulen >> PAGE_SHIFT,
			    PAGE_FLAG_KEEP_PTES | PAGE_FLAG_KEEP_FRAMES);
		}
		vn_lock(vp);
		VOP_READ(vp, args->f, &uio, 0);
		vn_unlock(vp);

		arr[off_file / block_size] = off_buf;

		arr_p[off_buf / block_size] = off_file;

		if (args->waiting_thread)
			uk_thread_wake(args->waiting_thread);
		if (args->exit)
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

	__sz block_size = 2 * 1024lu * 1024lu; // 2 MB
	__sz buf_len = 512LLU * 1024LLU * 1024LLU;
	__sz arr_len = sizeof(__off) * ALIGN_UP(len, block_size) / block_size;
	__sz arr_p_len = sizeof(__off) * buf_len / block_size;

	struct uk_vma_file_defer_thread_args *thread_args =
	    uk_malloc(vas->a, sizeof(struct uk_vma_file_defer_thread_args) +
				  arr_len + arr_p_len);

	if (unlikely(!thread_args)) {
		uk_free(vas->a, vma_file);
		return -ENOMEM;
	}

	thread_args->vma = &vma_file->base;
	vma_file->thread_args = thread_args;

	__off *arr = (__off *)(thread_args + 1);
	__off *arr_p = (__off *)(arr + arr_len / sizeof(__off));

	memset(arr, -1, arr_len);
	memset(arr_p, -1, arr_p_len);

	thread_args->arr = arr;
	thread_args->arr_p = arr_p;

	thread_args->f = vfscore_get_file(args->fd);
	if (unlikely(!thread_args->f)) {
		uk_free(vas->a, vma_file);
		uk_free(vas->a, thread_args);
		return -EBADF;
	}
	thread_args->start = vaddr;
	thread_args->offset = args->offset;
	thread_args->len = PAGE_ALIGN_UP(len);
	thread_args->block_size = block_size;
	thread_args->waiting_thread = NULL;
	thread_args->count = 1;
	thread_args->exit = false;

	/* Use the file name as VMA name. Since the memory management of the
	 * string is tied to the file object, we do not need to care about
	 * freeing it. So it is ok, if the caller should override the name.
	 */
	vma_file->base.name = thread_args->f->f_dentry->d_path;

	UK_ASSERT(vma);
	*vma = &vma_file->base;

	__paddr_t buf = uk_falloc(vas->pt->fa, buf_len / PAGE_SIZE);
	if (unlikely(buf == __PADDR_INV)) {
		fdrop(thread_args->f);
		uk_free(vas->a, vma_file);
		uk_free(vas->a, thread_args);
		return -ENOMEM;
	}
	thread_args->buf_len = buf_len;

	__vaddr_t vaddr1 =
	    ukplat_page_kmap(vas->pt, buf, buf_len / PAGE_SIZE, 0);

	thread_args->buf = vaddr1;
	vma_file->buf_p = buf;

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

	struct uk_thread *preload_thread =
	    uk_sched_thread_create(uk_sched_current(), defer_file_load,
				   thread_args, "File map preload");

	// Remove again
	uk_list_del(&vma_file->base.vma_list);

	vma_file->preload_thread = preload_thread;

	return 0;
}

void vma_op_file_defer_destroy(struct uk_vma *vma)
{
	struct uk_vma_file_defer *vma_file = (struct uk_vma_file_defer *)vma;
	struct uk_vma_file_defer_thread_args *args = vma_file->thread_args;

	// Exit thread. This is easier than changing mapping mid-run.
	// Just start a new one if not last mapping
	if (!uk_thread_is_exited(vma_file->preload_thread)) {
		struct uk_thread *current = uk_thread_current();
		unsigned long flags = ukplat_lcpu_save_irqf();

		args->waiting_thread = current;
		uk_thread_set_blocked(current);
		uk_sched_thread_blocked(current);
		args->exit = true;

		ukplat_lcpu_restore_irqf(flags);
		uk_sched_yield();
	}


	if (args->count > 1) {
		// Still a mapping remaining. Calculate new args and start
		// thread again.
		__vaddr_t start = args->start;
		__vaddr_t vma_start = vma->start;

		if (start == vma_start) {
			// First part getting destroyed
			__sz vma_len = vma->end - vma->start;
			__sz new_len = args->len - vma_len;
			__off new_off = args->offset + vma_len;

			args->start = start + vma_len;
			args->offset = new_off;
			args->len = new_len;

			// Shift array index (arr) by file offset
			args->arr = &args->arr[new_off / args->block_size];

			// Clear reverse index (arr_p).
			// Also clear page mapping to avoid complexity of
			// touching every value in arr_p.
			ukplat_page_unmap(vma->vas->pt, start + vma_len,
					  new_len / PAGE_SIZE,
					  PAGE_FLAG_KEEP_FRAMES);
			memset(args->arr_p, -1,
			       sizeof(__off) * args->buf_len /
				   args->block_size);
			memset(args->arr, -1,
			       sizeof(__off) * new_len / args->block_size);
			args->vma = args->vma2;

		} else {
			// Second part getting destroyed
			__sz vma_len = vma->end - vma->start;
			__sz new_len = args->len - vma_len;

			args->len = new_len;

			// Clear reverse index (arr_p).
			// Also clear page mapping to avoid complexity of
			// touching every value in arr_p.
			ukplat_page_unmap(vma->vas->pt, start,
					  new_len / PAGE_SIZE,
					  PAGE_FLAG_KEEP_FRAMES);
			memset(args->arr_p, -1,
			       sizeof(__off) * args->buf_len /
				   args->block_size);
			memset(args->arr, -1,
			       sizeof(__off) * new_len / args->block_size);

		}
		args->exit = false;
		args->count--;
		struct uk_thread *preload_thread =
		    uk_sched_thread_create(uk_sched_current(), defer_file_load,
					   args, "File map preload");
		((struct uk_vma_file_defer *)args->vma)->preload_thread =
		    preload_thread;
	} else {
		// Last mapping. Cleanup.
		UK_ASSERT(args->f);
		fdrop(args->f);

		ukplat_page_kunmap(vma->vas->pt, args->buf,
				   args->buf_len / PAGE_SIZE, 0);
		uk_ffree(vma->vas->pt->fa, vma_file->buf_p,
			 args->buf_len / PAGE_SIZE);
		uk_free(vma->vas->a, vma_file->thread_args);
	}
}
