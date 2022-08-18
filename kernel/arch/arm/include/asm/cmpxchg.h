/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_ARM_CMPXCHG_H
#define __ASM_ARM_CMPXCHG_H

#include <linux/irqflags.h>
#include <linux/prefetch.h>
#include <asm/barrier.h>

#if defined(CONFIG_CPU_SA1100) || defined(CONFIG_CPU_SA110)
/*
 * On the StrongARM, "swp" is terminally broken since it bypasses the
 * cache totally.  This means that the cache becomes inconsistent, and,
 * since we use normal loads/stores as well, this is really bad.
 * Typically, this causes oopsen in filp_close, but could have other,
 * more disastrous effects.  There are two work-arounds:
 *  1. Disable interrupts and emulate the atomic swap
 *  2. Clean the cache, perform atomic swap, flush the cache
 *
 * We choose (1) since its the "easiest" to achieve here and is not
 * dependent on the processor type.
 *
 * NOTE that this solution won't work on an SMP system, so explcitly
 * forbid it here.
 */
#define swp_is_buggy
#endif

static inline unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	extern void __bad_xchg(volatile void *, int);
	unsigned long ret;
#ifdef swp_is_buggy
	unsigned long flags;
#endif
#if __LINUX_ARM_ARCH__ >= 6 || defined(CONFIG_CPU_FMP626)
	unsigned int tmp;
#endif

	prefetchw((const void *)ptr);

	switch (size) {
#if __LINUX_ARM_ARCH__ >= 6
#ifndef CONFIG_CPU_V6 /* MIN ARCH >= V6K */
	case 1:
		asm volatile("@	__xchg1\n"
		"1:	ldrexb	%0, [%3]\n"
		"	strexb	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
	case 2:
		asm volatile("@	__xchg2\n"
		"1:	ldrexh	%0, [%3]\n"
		"	strexh	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
#endif
	case 4:
		asm volatile("@	__xchg4\n"
		"1:	ldrex	%0, [%3]\n"
		"	strex	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
#elif defined(CONFIG_CPU_FMP626)
	case 1:
		asm volatile("@	__xchg1\n"
		"1:	ldc	p13, c0, [%3], {0}	@ set address for ldrexb\n"
		"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexb\n"
		"	mcr	p13, 0, %2, c0, c0, 0	@ data for strexb\n"
		"	stc	p13, c0, [%3], {0}	@ strexb to address\n"
		"	mrc	p13, 0, %1, c0, c0, 2	@ strexb status\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
	case 4:
		asm volatile("@	__xchg4\n"
		"1:	ldc	p13, c0, [%3], {2}	@ set address for ldrex\n"
		"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
		"	mcr	p13, 0, %2, c0, c0, 0	@ data for strex\n"
		"	stc	p13, c0, [%3], {2}	@ strex to address\n"
		"	mrc	p13, 0, %1, c0, c0, 2	@ strex status\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
#elif defined(swp_is_buggy)
#ifdef CONFIG_SMP
#error SMP is not supported on this platform
#endif
	case 1:
		raw_local_irq_save(flags);
		ret = *(volatile unsigned char *)ptr;
		*(volatile unsigned char *)ptr = x;
		raw_local_irq_restore(flags);
		break;

	case 4:
		raw_local_irq_save(flags);
		ret = *(volatile unsigned long *)ptr;
		*(volatile unsigned long *)ptr = x;
		raw_local_irq_restore(flags);
		break;
#else
	case 1:
		asm volatile("@	__xchg1\n"
		"	swpb	%0, %1, [%2]"
			: "=&r" (ret)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
	case 4:
		asm volatile("@	__xchg4\n"
		"	swp	%0, %1, [%2]"
			: "=&r" (ret)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
#endif
	default:
		/* Cause a link-time error, the xchg() size is not supported */
		__bad_xchg(ptr, size), ret = 0;
		break;
	}

	return ret;
}

#define xchg_relaxed(ptr, x) ({						\
	(__typeof__(*(ptr)))__xchg((unsigned long)(x), (ptr),		\
				   sizeof(*(ptr)));			\
})

#include <asm-generic/cmpxchg-local.h>

#if __LINUX_ARM_ARCH__ < 6 && !defined(CONFIG_CPU_FMP626)
/* min ARCH < ARMv6 */

#ifdef CONFIG_SMP
#error "SMP is not supported on this platform"
#endif

#define xchg xchg_relaxed

/*
 * cmpxchg_local and cmpxchg64_local are atomic wrt current CPU. Always make
 * them available.
 */
#define cmpxchg_local(ptr, o, n) ({					\
	(__typeof(*ptr))__cmpxchg_local_generic((ptr),			\
					        (unsigned long)(o),	\
					        (unsigned long)(n),	\
					        sizeof(*(ptr)));	\
})

#define cmpxchg64_local(ptr, o, n) __cmpxchg64_local_generic((ptr), (o), (n))

#include <asm-generic/cmpxchg.h>

#else	/* min ARCH >= ARMv6 */

extern void __bad_cmpxchg(volatile void *ptr, int size);

/*
 * cmpxchg only support 32-bits operands on ARMv6.
 */

static inline unsigned long __cmpxchg(volatile void *ptr, unsigned long old,
				      unsigned long new, int size)
{
	unsigned long oldval, res;

	prefetchw((const void *)ptr);

	switch (size) {
#ifndef CONFIG_CPU_FMP626
#ifndef CONFIG_CPU_V6	/* min ARCH >= ARMv6K */
	case 1:
		do {
			asm volatile("@ __cmpxchg1\n"
			"	ldrexb	%1, [%2]\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	strexbeq %0, %4, [%2]\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
	case 2:
		do {
			asm volatile("@ __cmpxchg1\n"
			"	ldrexh	%1, [%2]\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	strexheq %0, %4, [%2]\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
#endif
	case 4:
		do {
			asm volatile("@ __cmpxchg4\n"
			"	ldrex	%1, [%2]\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	strexeq %0, %4, [%2]\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
#else	/* CONFIG_CPU_FMP626 */
	case 1:
		do {
			asm volatile("@ __cmpxchg1\n"
			"	ldc	p13, c0, [%2], {0}	@ set address for ldrexb\n"
			"	mrc	p13, 0, %1, c0, c0, 0	@ ldrexb\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	mcreq	p13, 0, %4, c0, c0, 0	@ data for strexb\n"
			"	stceq	p13, c0, [%2], {0}	@ strexb to address\n"
			"	mrceq	p13, 0, %0, c0, c0, 2	@ strexb status\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
	case 2:
		do {
			asm volatile("@ __cmpxchg1\n"
			"	ldc	p13, c0, [%2], {1}	@ set address for ldrexh\n"
			"	mrc	p13, 0, %1, c0, c0, 0	@ ldrexh\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	mcreq	p13, 0, %4, c0, c0, 0	@ data for strexh\n"
			"	stceq	p13, c0, [%2], {1}	@ strexh to address\n"
			"	mrceq	p13, 0, %0, c0, c0, 2	@ strexh status\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
	case 4:
		do {
			asm volatile("@ __cmpxchg4\n"
			"	ldc	p13, c0, [%2], {2}	@ set address for ldrex\n"
			"	mrc	p13, 0, %1, c0, c0, 0	@ ldrex\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	mcreq	p13, 0, %4, c0, c0, 0	@ data for strex\n"
			"	stceq	p13, c0, [%2], {2}	@ strex to address\n"
			"	mrceq	p13, 0, %0, c0, c0, 2	@ strex status\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
#endif	/* CONFIG_CPU_FMP626 */
	default:
		__bad_cmpxchg(ptr, size);
		oldval = 0;
	}

	return oldval;
}

#define cmpxchg_relaxed(ptr,o,n) ({					\
	(__typeof__(*(ptr)))__cmpxchg((ptr),				\
				      (unsigned long)(o),		\
				      (unsigned long)(n),		\
				      sizeof(*(ptr)));			\
})

static inline unsigned long __cmpxchg_local(volatile void *ptr,
					    unsigned long old,
					    unsigned long new, int size)
{
	unsigned long ret;

	switch (size) {
#if defined(CONFIG_CPU_V6) && !defined(CONFIG_CPU_FMP626)/* min ARCH == ARMv6 */
	case 1:
	case 2:
		ret = __cmpxchg_local_generic(ptr, old, new, size);
		break;
#endif
	default:
		ret = __cmpxchg(ptr, old, new, size);
	}

	return ret;
}

#define cmpxchg_local(ptr, o, n) ({					\
	(__typeof(*ptr))__cmpxchg_local((ptr),				\
				        (unsigned long)(o),		\
				        (unsigned long)(n),		\
				        sizeof(*(ptr)));		\
})

static inline unsigned long long __cmpxchg64(unsigned long long *ptr,
					     unsigned long long old,
					     unsigned long long new)
{
	unsigned long long oldval;
	unsigned long res;

	prefetchw(ptr);

	__asm__ __volatile__(
"1:	ldrexd		%1, %H1, [%3]\n"
"	teq		%1, %4\n"
"	teqeq		%H1, %H4\n"
"	bne		2f\n"
"	strexd		%0, %5, %H5, [%3]\n"
"	teq		%0, #0\n"
"	bne		1b\n"
"2:"
	: "=&r" (res), "=&r" (oldval), "+Qo" (*ptr)
	: "r" (ptr), "r" (old), "r" (new)
	: "cc");

	return oldval;
}

#define cmpxchg64_relaxed(ptr, o, n) ({					\
	(__typeof__(*(ptr)))__cmpxchg64((ptr),				\
					(unsigned long long)(o),	\
					(unsigned long long)(n));	\
})

#define cmpxchg64_local(ptr, o, n) cmpxchg64_relaxed((ptr), (o), (n))

#endif	/* __LINUX_ARM_ARCH__ >= 6 */

#endif /* __ASM_ARM_CMPXCHG_H */
