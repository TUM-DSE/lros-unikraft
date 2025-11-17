/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <errno.h>

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
#include <uk/isr/string.h>

#ifdef CONFIG_LIBUKVMEM_FILE_BASE
static __vaddr_t vma_op_file_get_base(struct uk_vas *vas __unused,
				      void *data __unused,
				      unsigned long flags __unused)
{
	return CONFIG_LIBUKVMEM_FILE_BASE;
}
#endif /* CONFIG_LIBUKVMEM_FILE_BASE */

void sched_yield_simulate(struct __regs* regs);


static int vma_op_file_defer_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct uk_vma_file_defer *vma_file = (struct uk_vma_file_defer *)vma;
	__paddr_t paddr = __PADDR_ANY;
	__off off;

	UK_ASSERT(PAGE_ALIGNED(fault->len));
	UK_ASSERT(fault->len == PAGE_Lx_SIZE(fault->level));
	UK_ASSERT(fault->type & UK_VMA_FAULT_NONPRESENT);

	if (!(vma->flags & UK_VMA_FLAG_UNINITIALIZED)) {

		__sz block_size = vma_file->thread_args->block_size;

		off = (fault->vbase - vma->start) + vma_file->thread_args->offset % block_size;

		//rc = vma_file_read(vma_file->f, vaddr, fault->len, off, &bytes);

retry:
		__off buf_off = vma_file->thread_args->arr[off / block_size];

		if (buf_off >= 0) {
			// Currently "paged in"
			paddr = buf_off + vma_file->buf_p + off % block_size;
		} else {
			// Need to wait for background thread to fetch page
			struct __regs* regs = fault->regs;
			uint8_t ectxbuf[ukarch_ectx_size() + ukarch_ectx_align()];
			struct ukarch_ectx *ectx = (struct ukarch_ectx *)
			    ALIGN_UP((__uptr) ectxbuf, ukarch_ectx_align());
			struct ukarch_sysctx sysctx;

			// Switch to non-interrupt context

			/* Save extended register state */
			ukarch_ectx_sanitize(ectx);
			ukarch_ectx_store(ectx);

			/* Save system context state */
			ukarch_sysctx_store(&sysctx);

			// Set the current thread to sleep
			struct uk_thread *current = uk_thread_current();
			vma_file->thread_args->waiting_thread = current;
			uk_thread_set_blocked(current);
			uk_sched_thread_blocked(current);

			// Back to interrupt context

			/* Restore system context state */
			ukarch_sysctx_load(&sysctx);

			// Now we need to simulate a sched_yield call
			sched_yield_simulate(regs);

			// Back to interrupt context pt 2

			/* Restore extended register state */
			ukarch_ectx_load(ectx);

			goto retry;
		}

	}

	fault->paddr = paddr;
	return 0;
}


static int vma_op_file_defer_unmap(struct uk_vma *vma, __vaddr_t vaddr, __sz len)
{
	UK_ASSERT(vaddr >= vma->start);
	UK_ASSERT(vaddr + len <= vma->end);
	UK_ASSERT(PAGE_ALIGNED(len));

	return ukplat_page_unmap(vma->vas->pt, vaddr, len / PAGE_SIZE, PAGE_FLAG_KEEP_FRAMES);
}

static int vma_op_file_defer_split(struct uk_vma *vma, __vaddr_t vaddr,
				   struct uk_vma **new_vma)
{
	struct uk_vma_file_defer *vma_file_defer = (struct uk_vma_file_defer *)vma;
	struct uk_vma_file_defer *v;

	if (vma_file_defer->thread_args->count > 1) {
		return -EPERM;
	}
	vma_file_defer->thread_args->count++;

	v = uk_malloc(vma->vas->a, sizeof(struct uk_vma_file_defer));
	if (unlikely(!v))
		return -ENOMEM;

	v->thread_args = vma_file_defer->thread_args;
	v->preload_thread = vma_file_defer->preload_thread;
	v->buf_p = vma_file_defer->buf_p;

	v->thread_args->vma2 = &v->base;

	UK_ASSERT(new_vma);
	*new_vma = &v->base;

	return 0;
}

int vma_op_file_defer_new(struct uk_vas *vas, __vaddr_t vaddr __unused,
			  __sz len, void *data, unsigned long attr,
			  unsigned long *flags, struct uk_vma **vma);
void vma_op_file_defer_destroy(struct uk_vma *vma);

/* We only support private mappings. Changes are not carried through to the
 * underlying file. So we can just use the default unmap handler that unmaps
 * the memory and forgets about it. Private file mappings can also change their
 * protections without checking for the permissions on the underlying file. We
 * can thus also use the default attribute setter.
 */
const struct uk_vma_ops uk_vma_file_defer_ops = {
#ifdef CONFIG_LIBUKVMEM_FILE_BASE
	.get_base	= vma_op_file_get_base,
#else /* CONFIG_LIBUKVMEM_FILE_BASE */
	.get_base	= __NULL,
#endif /* !CONFIG_LIBUKVMEM_FILE_BASE */
	.new		= vma_op_file_defer_new,
	.destroy	= vma_op_file_defer_destroy,
	.fault		= vma_op_file_defer_fault,
	.unmap		= vma_op_file_defer_unmap,
	.split		= vma_op_file_defer_split,
	.merge		= vma_op_deny,
	.set_attr	= vma_op_deny,
	.advise		= vma_op_advise,	/* default */
};
