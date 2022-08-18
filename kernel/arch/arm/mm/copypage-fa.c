/*
 *  linux/arch/arm/lib/copypage-fa.S
 *
 *  Copyright (C) 2005 Faraday Corp.
 *  Copyright (C) 2008-2009 Paulius Zaleckas <paulius.zaleckas@teltonika.lt>
 *
 * Based on copypage-v4wb.S:
 *  Copyright (C) 1995-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/spinlock.h>
#include <linux/mm.h>

#include <asm/pgtable.h>
#include <asm/shmparam.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <asm/cachetype.h>

#include "mm.h"

#define from_address	(0xffff8000)
#define to_address	(0xffffc000)

static DEFINE_RAW_SPINLOCK(fa_lock);

/*
 * Faraday optimised copy_user_page
 */
static void __naked
fa_copy_user_page(void *kto, const void *kfrom)
{
	asm("\
	stmfd	sp!, {r4, lr}			@ 2\n\
	mov	r2, %0				@ 1\n\
1:	ldmia	r1!, {r3, r4, ip, lr}		@ 4\n\
	stmia	r0, {r3, r4, ip, lr}		@ 4\n\
	mcr	p15, 0, r0, c7, c14, 1		@ 1   clean and invalidate D line\n\
	add	r0, r0, #16			@ 1\n\
	ldmia	r1!, {r3, r4, ip, lr}		@ 4\n\
	stmia	r0, {r3, r4, ip, lr}		@ 4\n\
	mcr	p15, 0, r0, c7, c14, 1		@ 1   clean and invalidate D line\n\
	add	r0, r0, #16			@ 1\n\
	subs	r2, r2, #1			@ 1\n\
	bne	1b				@ 1\n\
	mcr	p15, 0, r2, c7, c10, 4		@ 1   drain WB\n\
	ldmfd	sp!, {r4, pc}			@ 3"
	:
	: "I" (PAGE_SIZE / 32));
}

void fa_copy_user_highpage(struct page *to, struct page *from,
	unsigned long vaddr, struct vm_area_struct *vma)
{
	void *kto, *kfrom;

	kto = kmap_atomic(to);
	kfrom = kmap_atomic(from);
	flush_cache_page(vma, vaddr, page_to_pfn(from));
	fa_copy_user_page(kto, kfrom);
	kunmap_atomic(kfrom);
	kunmap_atomic(kto);
}

/*
 * Faraday optimised clear_user_page
 *
 * Same story as above.
 */
void fa_clear_user_highpage(struct page *page, unsigned long vaddr)
{
	void *ptr, *kaddr = kmap_atomic(page);
	asm volatile("\
	mov	r1, %2				@ 1\n\
	mov	r2, #0				@ 1\n\
	mov	r3, #0				@ 1\n\
	mov	ip, #0				@ 1\n\
	mov	lr, #0				@ 1\n\
1:	stmia	%0, {r2, r3, ip, lr}		@ 4\n\
	mcr	p15, 0, %0, c7, c14, 1		@ 1   clean and invalidate D line\n\
	add	%0, %0, #16			@ 1\n\
	stmia	%0, {r2, r3, ip, lr}		@ 4\n\
	mcr	p15, 0, %0, c7, c14, 1		@ 1   clean and invalidate D line\n\
	add	%0, %0, #16			@ 1\n\
	subs	r1, r1, #1			@ 1\n\
	bne	1b				@ 1\n\
	mcr	p15, 0, r1, c7, c10, 4		@ 1   drain WB"
	: "=r" (ptr)
	: "0" (kaddr), "I" (PAGE_SIZE / 32)
	: "r1", "r2", "r3", "ip", "lr");
	kunmap_atomic(kaddr);
}


/*
 * Discard data in the kernel mapping for the new page.
 * FIXME: needs this MCRR to be supported.
 */
static void discard_old_kernel_data(void *kto)
{
	unsigned long to = (unsigned long) kto;
	unsigned long end = (unsigned long)kto + PAGE_SIZE;

	for (; to < end; to += L1_CACHE_BYTES) {
		/* invalidate D-cache entry */
		__asm__ ("mcr	p15, 0, %0, c7, c6, 1\n\t"
			 :
			 : "r" (to)
			 : "cc");
	}
}

/*
 * Copy the page, taking account of the cache colour.
 */
static void fa_copy_user_highpage_aliasing(struct page *to,
	struct page *from, unsigned long vaddr, struct vm_area_struct *vma)
{
	unsigned int offset = CACHE_COLOUR(vaddr);
	unsigned long kfrom, kto;

	if (!test_and_set_bit(PG_dcache_clean, &from->flags))
		__flush_dcache_page(page_mapping(from), from);

	/* FIXME: not highmem safe */
	discard_old_kernel_data(page_address(to));

	/*
	 * Now copy the page using the same cache colour as the
	 * pages ultimate destination.
	 */
	raw_spin_lock(&fa_lock);

	kfrom = from_address + (offset << PAGE_SHIFT);
	kto   = to_address + (offset << PAGE_SHIFT);

	set_top_pte(kfrom, mk_pte(from, PAGE_KERNEL));
	set_top_pte(kto, mk_pte(to, PAGE_KERNEL));

	copy_page((void *)kto, (void *)kfrom);

	raw_spin_unlock(&fa_lock);
}

/*
 * Clear the user page.  We need to deal with the aliasing issues,
 * so remap the kernel page into the same cache colour as the user
 * page.
 */
static void fa_clear_user_highpage_aliasing(struct page *page, unsigned long vaddr)
{
	unsigned long to = to_address + (CACHE_COLOUR(vaddr)<< PAGE_SHIFT);

	/* FIXME: not highmem safe */
	discard_old_kernel_data(page_address(page));

	/*
	 * Now clear the page using the same cache colour as
	 * the pages ultimate destination.
	 */
	raw_spin_lock(&fa_lock);

	set_top_pte(to, mk_pte(page, PAGE_KERNEL));
	clear_page((void *)to);

	raw_spin_unlock(&fa_lock);
}

/*
 * Copy the user page.  No aliasing to deal with so we can just
 * attack the kernel's existing mapping of these pages.
 */
static void fa_copy_user_highpage_nonaliasing(struct page *to,
	struct page *from, unsigned long vaddr, struct vm_area_struct *vma)
{
	void *kto, *kfrom;

	kfrom = kmap_atomic(from);
	kto = kmap_atomic(to);
	copy_page(kto, kfrom);
	kunmap_atomic(kto);
	kunmap_atomic(kfrom);
}

/*
 * Clear the user page.  No aliasing to deal with so we can just
 * attack the kernel's existing mapping of this page.
 */
static void fa_clear_user_highpage_nonaliasing(struct page *page, unsigned long vaddr)
{
	void *kaddr = kmap_atomic(page);
	clear_page(kaddr);
	kunmap_atomic(kaddr);
}

struct cpu_user_fns fa_user_fns __initdata = {
	.cpu_clear_user_highpage = fa_clear_user_highpage,
	.cpu_copy_user_highpage	= fa_copy_user_highpage,
};

static int __init fa_userpage_init(void)
{
	if (cache_is_vipt_aliasing()) {
		cpu_user.cpu_clear_user_highpage = fa_clear_user_highpage_aliasing;
		cpu_user.cpu_copy_user_highpage = fa_copy_user_highpage_aliasing;
	} else if (cache_is_vipt_nonaliasing()) {
		cpu_user.cpu_clear_user_highpage = fa_clear_user_highpage_nonaliasing;
		cpu_user.cpu_copy_user_highpage = fa_copy_user_highpage_nonaliasing;
	}

	return 0;
}

core_initcall(fa_userpage_init);

