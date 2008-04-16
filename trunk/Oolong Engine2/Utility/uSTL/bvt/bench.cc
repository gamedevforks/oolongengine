// This file is part of the ustl library, an STL implementation.
//
// Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//

#include <ustl.h>
#include <time.h>
using namespace ustl;

//----------------------------------------------------------------------
// Copy functions
//----------------------------------------------------------------------

#if __i386__ || __x86_64__
extern "C" void movsb_copy (const char* src, size_t nBytes, char* dest)
{
    asm volatile (
	"cld\n\trep\n\tmovsb"
	: "=&S"(src), "=&D"(dest)
	: "0"(src), "1"(dest), "c"(nBytes)
	: "memory");
}

extern "C" void movsd_copy (const char* src, size_t nBytes, char* dest)
{
    asm volatile (
	"cld\n\trep\n\tmovsl"
	: "=&S"(src), "=&D"(dest)
	: "0"(src), "1"(dest), "c"(nBytes / 4)
	: "memory");
}

extern "C" void risc_copy (const char* src, size_t nBytes, char* dest)
{
    unsigned long* ldest ((unsigned long*) dest);
    const unsigned long* lsrc ((const unsigned long*) src);
    nBytes /= sizeof(*lsrc);
    do {
	*ldest++ = *lsrc++;
    } while (--nBytes);
}

extern "C" void unroll_copy (const char* src, size_t nBytes, char* dest)
{
    unsigned long* ldest ((unsigned long*) dest);
    const unsigned long* lsrc ((const unsigned long*) src);
    nBytes /= 4 * sizeof(unsigned long);
    do {
	ldest[0] = lsrc[0];
	ldest[1] = lsrc[1];
	ldest[2] = lsrc[2];
	ldest[3] = lsrc[3];
	ldest += 4;
	lsrc += 4;
    } while (--nBytes);
}

#if CPU_HAS_MMX
extern "C" void mmx_copy (const char* src, size_t nBytes, char* dest)
{
    nBytes /= 32;
    do {
	prefetch (src + 512, 0, 0);
	asm (
	    "movq	%4, %%mm0	\n\t"
	    "movq	%5, %%mm1	\n\t"
	    "movq	%6, %%mm2	\n\t"
	    "movq	%7, %%mm3	\n\t"
	    "movq	%%mm0, %0	\n\t"
	    "movq	%%mm1, %1	\n\t"
	    "movq	%%mm2, %2	\n\t"
	    "movq	%%mm3, %3"
	    : "=m"(dest[0]), "=m"(dest[8]), "=m"(dest[16]), "=m"(dest[24])
	    : "m"(src[0]), "m"(src[8]), "m"(src[16]), "m"(src[24])
	    : "mm0", "mm1", "mm2", "mm3", "st", "st(1)", "st(2)", "st(3)");
	src += 32;
	dest += 32;
    } while (--nBytes);
    simd::reset_mmx();
}
#endif // CPU_HAS_MMX

#if CPU_HAS_SSE
extern "C" void sse_copy (const char* src, size_t nBytes, char* dest)
{
    const size_t nHeadBytes = min (nBytes, Align(uintptr_t(src), 16U) - uintptr_t(src));
    for (uoff_t i = 0; i < nHeadBytes; ++ i)
	*dest++ = *src++;
    nBytes -= nHeadBytes;
    if (!(uintptr_t(dest) % 16)) {
	const size_t nMiddleBlocks = nBytes / 32;
	for (uoff_t i = 0; i < nMiddleBlocks; ++ i) {
	    prefetch (src + 512, 0, 0);
	    asm (
		"movaps\t%2, %%xmm0	\n\t"
		"movaps\t%3, %%xmm1	\n\t"
		"movntps\t%%xmm0, %0	\n\t"
		"movntps\t%%xmm1, %1"
		: "=m"(dest[0]), "=m"(dest[16])
		: "m"(src[0]), "m"(src[16])
		: "xmm0", "xmm1");
	    src += 32;
	    dest += 32;
	}
	nBytes %= 32;
    }
    for (uoff_t i = 0; i < nBytes; ++ i)
	*dest++ = *src++;
}
#endif // CPU_HAS_SSE
#endif // __i386__

extern "C" void memcpy_copy (const char* src, size_t nBytes, char* dest)
{
    memcpy (dest, src, nBytes);
}

template <typename CopyFunction>
void TestCopyFunction (const char* name, CopyFunction pfn)
{
    const uoff_t misalignment = 0;
    const uoff_t headBytes = 0;
    const uoff_t tailBytes = 0;

    const size_t nIter = 128;
    const size_t nBytes = 1024 * 1024 + misalignment;

    string buf1 (nBytes), buf2 (nBytes);
    iota (buf1.begin(), buf1.end(), '\x1');
    fill (buf2, 0);
    const clock_t first = clock();
    for (uoff_t i = 0; i < nIter; ++ i)
	(*pfn)(buf1.cdata() + headBytes, nBytes - headBytes - tailBytes, buf2.data() + headBytes + misalignment);
    clock_t last = clock();
    last += (last == first);
    const size_t mbps = nIter * CLOCKS_PER_SEC / (last - first);
    cout << name << " transfer rate is " << mbps << " Mbps, data is ";
    size_t nBad = 0;
    for (uoff_t i = headBytes; i < buf1.size() - tailBytes; ++ i)
	nBad += (buf1[i] != buf2[i + misalignment]);
    if (!nBad)
	cout << "GOOD" << endl;
    else {
	cout << "BAD" << endl;
	for (uoff_t i = headBytes; i < buf1.size() - tailBytes; ++ i)
	    if (buf1[i] != buf2[i + misalignment])
		cout << "\t\t" << i << "\tbuf1: " << (int) buf1[i] << ", buf2: " << (int) buf2[i + misalignment] << endl;
    }
    cout.flush();
}

//----------------------------------------------------------------------
// Fill functions
//----------------------------------------------------------------------

extern "C" void memset_fill (char* dest, size_t nBytes, char v)
{
    memset (dest, v, nBytes);
}

#if __i386__ || __x86_64__
extern "C" void stosb_fill (char* dest, size_t nBytes, char v)
{
    asm volatile (
	"cld\n\trep\n\tstosb\n\t"
	: "=&D"(dest)
	: "0"(dest), "a"(v), "c"(nBytes)
	: "memory");
}

extern "C" void stosd_fill (char* dest, size_t nBytes, char v)
{
    unsigned int lv;
    pack_type (v, lv);
    asm volatile (
	"cld\n\trep\n\tstosl\n\t"
	: "=&D"(dest)
	: "0"(dest), "a"(lv), "c"(nBytes / sizeof(lv))
	: "memory");
}

extern "C" void risc_fill (char* dest, size_t nBytes, char v)
{
    unsigned long lv;
    pack_type (v, lv);
    unsigned long* ldest ((unsigned long*) dest);
    nBytes /= sizeof(lv);
    do {
	*ldest++ = lv;
    } while (--nBytes);
}

extern "C" void unroll_fill (char* dest, size_t nBytes, char v)
{
    unsigned long lv;
    pack_type (v, lv);
    unsigned long* ldest ((unsigned long*) dest);
    nBytes /= 4 * sizeof(lv);
    do {
	ldest[0] = lv;
	ldest[1] = lv;
	ldest[2] = lv;
	ldest[3] = lv;
	ldest += 4;
    } while (--nBytes);
}

#if CPU_HAS_MMX
extern "C" void mmx_fill (char* dest, size_t nBytes, char v)
{
    prefetch (dest + 512, 1, 0);
    asm volatile (
	"movd %0, %%mm0		\n\t"
	"punpcklbw %%mm0, %%mm0	\n\t"
	"punpcklwd %%mm0, %%mm0	\n\t"
	"punpckldq %%mm0, %%mm0"
	::"r"(uint32_t(v))
	: "mm0", "st");
    const size_t nBlocks (nBytes / 32);
    for (uoff_t i = 0; i < nBlocks; ++ i) {
	asm volatile (
	    "movq %%mm0, %0	\n\t"
	    "movq %%mm0, %1	\n\t"
	    "movq %%mm0, %2	\n\t"
	    "movq %%mm0, %3"
	    : "=m"(dest[0]), "=m"(dest[8]), "=m"(dest[16]), "=m"(dest[24]));
	dest += 32;
    }
    simd::reset_mmx();
}
#endif // CPU_HAS_MMX
#endif // __i386__

template <typename FillFunction>
void TestFillFunction (const char* name, FillFunction pfn)
{
    const size_t nIter = 256;
    const size_t nBytes = 1024 * 1024;
    string buf1 (nBytes), buf2 (nBytes);
    iota (buf1.begin(), buf1.end(), '\x1');
    fill (buf2, 42);
    clock_t first = clock();
    for (uoff_t i = 0; i < nIter; ++ i)
	(*pfn)(buf1.data(), nBytes, char(42));
    clock_t last = clock();
    last += (last == first);
    const size_t mbps = nIter * CLOCKS_PER_SEC / (last - first);
    cout << name << " transfer rate is " << mbps << " Mbps, data is ";
    if (buf1 == buf2)
	cout << "GOOD" << endl;
    else
	cout << "BAD" << endl;
    cout.flush();
}

//----------------------------------------------------------------------

int main (void)
{
    cout << "Testing fill" << endl;
    cout << "---------------------------------------------------------" << endl;
    TestFillFunction ("fill_n\t\t", &fill_n<char*, char>);
#if __i386__ || __x86_64__
#if CPU_HAS_MMX && HAVE_INT64_T
    TestFillFunction ("mmx_fill\t", &mmx_fill);
#endif
    TestFillFunction ("stosb_fill\t", &stosb_fill);
    TestFillFunction ("stosd_fill\t", &stosd_fill);
    TestFillFunction ("unroll_fill\t", &unroll_fill);
    TestFillFunction ("risc_fill\t", &risc_fill);
#endif
    TestFillFunction ("memset_fill\t", &memset_fill);

    cout << endl;
    cout << "Testing copy" << endl;
    cout << "---------------------------------------------------------" << endl;
    TestCopyFunction ("copy_n\t\t", &copy_n<const char*, char*>);
#if __i386__ || __x86_64__
#if CPU_HAS_SSE
    TestCopyFunction ("sse_copy\t", &sse_copy);
#endif
#if CPU_HAS_MMX
    TestCopyFunction ("mmx_copy\t", &mmx_copy);
#endif
    TestCopyFunction ("movsb_copy\t", &movsb_copy);
    TestCopyFunction ("movsd_copy\t", &movsd_copy);
    TestCopyFunction ("risc_copy\t", &risc_copy);
    TestCopyFunction ("unroll_copy\t", &unroll_copy);
#endif
    TestCopyFunction ("memcpy_copy\t", &memcpy_copy);

    return (EXIT_SUCCESS);
}

