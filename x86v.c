#define _XOPEN_SOURCE   500
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <ctype.h>
#include <string.h>

/* cpuid.h was introduced around GCC 4.7
   https://gcc.gnu.org/legacy-ml/gcc-patches/2007-09/msg00324.html */
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 8
# include <cpuid.h>
#else
# if defined (__i386__)
#  define __cpuid(level, eax, ebx, ecx, edx)				\
	do {								\
		__asm__ volatile (					\
			"cpuid\n\t"					\
			: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)	\
			: "0"(level));					\
	} while (0)
# elif defined (__x86_64__)
#  define __cpuid(level, eax, ebx, ecx, edx)				\
	do {								\
		__asm__ volatile (					\
			"xchgq %%rbx, %q1\n\t"				\
			"cpuid\n\t"					\
			"xchgq %%rbx, %q1\n\t"				\
			: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)	\
			: "0"(level));					\
	} while (0)
# else
#  error x86v is only for x86 and x86-64.
# endif
#endif

enum {
	/* x86-64 v1 features. Available in edx register. */
	FPU = 0,
	CX8 = 8,
	/* Earlier versions of K5, syscall was 10. */
	SCE = 11,
	CMOV = 15,
	MMX = 23,
	FXSR = 24,
	SSE = 25,
	SSE2 = 26,

	/* x86-64 v2 features. */
        CMPXCHG16B = 13,
	POPCNT = 23,
	SSE3 = 0,
	SSE4_1 = 19,
	SSE4_2 = 20,
	SSSE3 = 9,
	/* Extended feature. */
	LAHF_SAHF = 0,

	/* x86-64 v3 features. */
	AVX = 28,
	/* leaf = 7 */
	AVX2 = 5,
	BMI1 = 3,
	BMI2 = 8,
	/* leaf = 1 */
	F16C = 29,
	FMA = 12,
	/* Extended feature. */ 
	LZCNT = 5,
	/* leaf = 1 */
	MOVBE = 22,
	/* TODO: OSXSAVE */
	OSXSAVE = 27,

	/* x86-64 v4 features. */
	AVX512F = 16,
	AVX512BW = 30,
	AVX512CD = 28,
	AVX512DQ = 17,
	AVX512VL = 31,
};

/* x86-64 CPU levels. */
#define CPU_VERSION_LEVEL_v1    (132)
#define CPU_VERSION_LEVEL_v2    (84)
#define CPU_VERSION_LEVEL_v3    (139)
#define CPU_VERSION_LEVEL_v4    (122)

/* Utility macros. */
#define ARRAY_SIZE(arr)    sizeof(arr)/sizeof(*arr)

#define CPU_FEAT_FOREACH(reg, level, alist, arr)			\
	do {								\
		size_t i;						\
		for (i = 0; i < ARRAY_SIZE(arr); i++) {			\
			if (cpu_has_feat(reg, arr[i].bit)) {		\
				level += arr[i].bit;			\
				tiny_alist_do_push_back(alist, arr[i].name); \
			}						\
		}							\
	} while (0)

#define CPU_FEAT_ADD_OK(reg, level, bit, alist)			\
	do {							\
	        if (cpu_has_feat(reg, bit)) {				\
			level += bit;					\
		        tiny_alist_do_push_back(alist, #bit);		\
	        }							\
	} while (0)

#define CPU_TINY_ALIST_PRINT_FOREACH(alist)				\
	do {								\
		size_t i;						\
		char *p;						\
		for (i = 0; i < alist->idx; i++) {			\
			p = conv_to_lowercase(alist->p[i]);		\
			fprintf(stdout, "%s%c", p,			\
				(i == alist->idx - 1) ? '\0' : ' ');	\
			free(p);					\
		}							\
	} while (0)

/* Structure holding the bit value for a
   specific CPU feature. */
struct cpu_feat_bits {
	unsigned int bit;
	const char *name;
};

struct tiny_alist {
	char **p;
	size_t idx;
	size_t total_len;
	size_t extra_len;
};

static struct cpu_feat_bits cpu_feat_bits_v1[] = {
	{ .bit = FPU, .name = "fpu" },
	{ .bit = CX8, .name = "cx8" },
	{ .bit = SCE, .name = "sce" },
	{ .bit = CMOV, .name = "cmov" },
	{ .bit = MMX, .name = "mmx" },
	{ .bit = FXSR, .name = "fxsr" },
	{ .bit = SSE, .name = "sse" },
	{ .bit = SSE2, .name = "sse2" },
};

static struct cpu_feat_bits cpu_feat_bits_v2[] = {
	{ .bit = CMPXCHG16B, .name = "cmpxchg16b" },
	{ .bit = POPCNT, .name = "popcnt" },
	{ .bit = SSE3, .name = "sse3" },
	{ .bit = SSE4_1, .name = "sse4.1" },
	{ .bit = SSE4_2, .name = "sse4.2" },
	{ .bit = SSSE3, .name = "ssse3" },
};

static struct cpu_feat_bits cpu_feat_bits_v4[] = {
	{ .bit = AVX512F, .name = "avx512-f" },
	{ .bit = AVX512BW, .name = "avx512-bw" },
	{ .bit = AVX512CD, .name = "avx512-cd" },
	{ .bit = AVX512DQ, .name = "avx512-dq" },
	{ .bit = AVX512VL, .name = "avx512-vl" },
};

/* Array list. */
static void tiny_alist_do_init(struct tiny_alist *tiny_alist)
{
	if ((tiny_alist->p = calloc(1, sizeof(char *))) == NULL)
	        err(1, "calloc()");
	tiny_alist->total_len = tiny_alist->idx = 0;
	tiny_alist->extra_len = 0;
}

static void tiny_alist_do_push_back(struct tiny_alist *tiny_alist,
				    const char *elem)
{
	size_t elem_len;

do_alloc:
	elem_len = sizeof(char *);
	if (tiny_alist->extra_len <= tiny_alist->total_len) {
		tiny_alist->extra_len = (tiny_alist->total_len + elem_len) * 2;
		if ((tiny_alist->p = realloc(tiny_alist->p, tiny_alist->extra_len)) == NULL)
		        err(1, "realloc()");
	} else {
		tiny_alist->extra_len -= elem_len;
		if (tiny_alist->extra_len <= tiny_alist->total_len)
			goto do_alloc;
	}

	tiny_alist->p[tiny_alist->idx++] = (char *)elem;
	tiny_alist->total_len += elem_len;
}

static void tiny_alist_do_reset(struct tiny_alist *tiny_alist)
{
	tiny_alist->idx = tiny_alist->total_len = 0;
}

static void tiny_alist_do_free(struct tiny_alist *tiny_alist)
{
	if (tiny_alist->p) {
		tiny_alist->extra_len = tiny_alist->total_len = tiny_alist->idx = 0;
		free(tiny_alist->p);
		tiny_alist->p = NULL;
	}
}

static char *conv_to_lowercase(const char *s)
{
        char *dup, *a;

	if ((dup = strdup(s)) == NULL)
		err(1, "strdup()");
	for (a = dup; *s != '\0'; s++, a++)
		*a = tolower(*s);
	return (dup);
}

static inline int cpu_has_feat(unsigned int reg, unsigned int bit)
{
	return ((reg & (1 << bit)) != 0);
}

static int cpu_detect_version_level_v1(struct tiny_alist *tiny_alist)
{
	unsigned int eax, ebx, ecx, edx;
	size_t i, level;

        __cpuid(1, eax, ebx, ecx, edx);
	level = 0;

	for (i = 0; i < ARRAY_SIZE(cpu_feat_bits_v1); i++) {
		if (cpu_has_feat(edx, cpu_feat_bits_v1[i].bit)) {
			tiny_alist_do_push_back(tiny_alist, cpu_feat_bits_v1[i].name);
			level += cpu_feat_bits_v1[i].bit;
		}
	}

	return (level == CPU_VERSION_LEVEL_v1 ? CPU_VERSION_LEVEL_v1 : -1);
}

static int cpu_detect_version_level_v2(struct tiny_alist *tiny_alist)
{
	unsigned int eax, ebx, ecx, edx, level;

        __cpuid(1, eax, ebx, ecx, edx);
	level = 0;

	CPU_FEAT_FOREACH(ecx, level, tiny_alist, cpu_feat_bits_v2);

	__cpuid(0x80000001, eax, ebx, ecx, edx);
        if (cpu_has_feat(ecx, LAHF_SAHF))
		level += LAHF_SAHF;

	return (level == CPU_VERSION_LEVEL_v2 ? CPU_VERSION_LEVEL_v2 : -1);
}

static int cpu_detect_version_level_v3(struct tiny_alist *tiny_alist)
{
	unsigned int eax, ebx, ecx, edx, level;

	level = 0;

        __cpuid(1, eax, ebx, ecx, edx);
	CPU_FEAT_ADD_OK(ecx, level, AVX, tiny_alist);
	CPU_FEAT_ADD_OK(ecx, level, F16C, tiny_alist);
	CPU_FEAT_ADD_OK(ecx, level, FMA, tiny_alist);
	CPU_FEAT_ADD_OK(ecx, level, MOVBE, tiny_alist);
	CPU_FEAT_ADD_OK(ecx, level, OSXSAVE, tiny_alist);

	ecx = 0;
	__cpuid(7, eax, ebx, ecx, edx);
	CPU_FEAT_ADD_OK(ebx, level, AVX2, tiny_alist);
	CPU_FEAT_ADD_OK(ebx, level, BMI1, tiny_alist);
	CPU_FEAT_ADD_OK(ebx, level, BMI2, tiny_alist);

	__cpuid(0x80000001, eax, ebx, ecx, edx);
	CPU_FEAT_ADD_OK(ecx, level, LZCNT, tiny_alist);

	return (level == CPU_VERSION_LEVEL_v3 ? CPU_VERSION_LEVEL_v3 : -1);
}

static int cpu_detect_version_level_v4(struct tiny_alist *tiny_alist)
{
	unsigned int eax, ebx, ecx, edx, level;

	level = ecx = 0;

	__cpuid(7, eax, ebx, ecx, edx);
	CPU_FEAT_FOREACH(ebx, level, tiny_alist, cpu_feat_bits_v4);

        return (level == CPU_VERSION_LEVEL_v4 ? CPU_VERSION_LEVEL_v4 : -1);
}

static void cpu_print_version_levels(struct tiny_alist *tiny_alist)
{
 	if (cpu_detect_version_level_v1(tiny_alist) == CPU_VERSION_LEVEL_v1) {
		fprintf(stdout, "x86-64 v1 supported (");
		CPU_TINY_ALIST_PRINT_FOREACH(tiny_alist);
		fputs(")\n", stdout);
	}

	/* Reset the total size and the current index
	   of the array-list to continue pushing new
	   elements without performing another reallocation. */
	tiny_alist_do_reset(tiny_alist);
	if (cpu_detect_version_level_v2(tiny_alist) == CPU_VERSION_LEVEL_v2) {
		fprintf(stdout, "x86-64 v2 supported (");
		CPU_TINY_ALIST_PRINT_FOREACH(tiny_alist);
		fputs(")\n", stdout);
	}

	tiny_alist_do_reset(tiny_alist);
	if (cpu_detect_version_level_v3(tiny_alist) == CPU_VERSION_LEVEL_v3) {
		fprintf(stdout, "x86-64 v3 supported (");
		CPU_TINY_ALIST_PRINT_FOREACH(tiny_alist);
		fputs(")\n", stdout);
	}

	tiny_alist_do_reset(tiny_alist);
	if (cpu_detect_version_level_v4(tiny_alist) == CPU_VERSION_LEVEL_v4) {
		fprintf(stdout, "x86-64 v4 supported\n");
		CPU_TINY_ALIST_PRINT_FOREACH(tiny_alist);
		fputs(")\n", stdout);
	}
}

static void print_help(void)
{
	/* __progname is available on Linux and BSD. */
	extern const char *__progname;
	fprintf(stdout, "Just run the %s command. "
		"Use the '-h' flag to show this output.\n", __progname);
	exit(0);
}

int main(int argc, char **argv)
{
	struct tiny_alist tiny_alist;

	if (argc >= 2) {
		if (strcmp(argv[1], "-h") == 0)
			print_help();
		else
			errx(1, "error: invalid argument.");
	}

        tiny_alist_do_init(&tiny_alist);
	cpu_print_version_levels(&tiny_alist);
	tiny_alist_do_free(&tiny_alist);

}
