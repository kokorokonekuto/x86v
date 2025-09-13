#ifndef IS_X86_FEAT_HPP
# define IS_X86_FEAT_HPP

#if defined(__clang__)
// Assuming it's not an ancient version of Clang.
# include <cpuid.h>
#elif defined (__GNUC__) && !defined (__clang__)
# if __GNUC__ >= 4 && __GNUC_MINOR__ >= 8
#  include <cpuid.h>
# else
#  if defined (__i386__)
#   define __cpuid(level, eax, ebx, ecx, edx)				\
	do {								\
		__asm__ volatile (					\
			"cpuid\n\t"					\
			: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)	\
			: "0"(level));					\
	} while (0)
#  elif defined (__x86_64__)
#   define __cpuid(level, eax, ebx, ecx, edx)				\
	do {								\
		__asm__ volatile (					\
			"xchgq %%rbx, %q1\n\t"				\
			"cpuid\n\t"					\
			"xchgq %%rbx, %q1\n\t"				\
			: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)	\
			: "0"(level));					\
	} while (0)
#  else
#   error only for x86 and x86-64.
#  endif
# endif
#else
# error unsupported compiler.
#endif

// Some of the Intel specific features are also in AMD CPUs as
// well, but I can't test all of them without having the hardware.
enum Feature {
    // EAX, EDX
    FPU,
    VME,
    DE,
    PSE,
    TSC,
    MSR,
    PAE,
    MCE,
    CX8,
    APIC,
    SEP,
    MTRR,
    PGE,
    MCA,
    CMOV,
    PAT,
    PSE36,
    PSN,
    CLFSH,
    DS,
    ACPI,
    MMX,
    FXSR,
    SSE,
    SSE2,
    SS,
    HTT,
    TM,
    IA64,
    PBE,

    // EAX, ECX
    SSE3,
    PCLMULQDQ,
    DTES64,
    MONITOR,
    DS_CPL,
    VMX,
    SMX,
    EST,
    TM2,
    SSSE3,
    CNXT_ID,
    SDBG,
    FMA,
    CX16,
    XTPR,
    PDCM,
    PCID,
    DCA,
    SSE41,
    SSE42,
    X2APIC,
    MOVBE,
    POPCNT,
    TSC_DEADLINE,
    AES_NI,
    XSAVE,
    OSXSAVE,
    AVX,
    F16C,
    RDRND,
    HYPERVISOR,

    // EAX = 6 thermal and power management
    DTS,
    ARAT,
    PLN,
    ECMD,
    PTM,

    // EAX = 7, ECX = 0, extended features
    FSGSBASE,
    SGX,
    BMI1,
    HLE,
    AVX2,
    FDP_EXCPTN_ONLY,
    SMEP,
    BMI2,
    ERMS,
    INVPCID,
    RTM,
    PQM,
    RDT_M, // On AMD this is called PQM.
    MPX, // Intel specific memory protection extension.
    RDT_A,
    AVX512F,
    AVX512DQ,
    RDSEED,
    ADX, // Intel specific extension.
    SMAP,
    AVX512IFMA,
    CLFLUSHOPT,
    CLWB,
    PT, // Intel specific extension.
    AVX512PF,
    AVX512ER,
    AVX512CD,
    SHA,
    AVX512BW,
    AVX512VL,

    PREFETCHWT1,
    AVX512VBMI,
    UMIP,
    PKU,
    OSPKE,
    WAITPKG,
    AVX512VBMI2,
    CET_SS,
    GFNI,
    VAES,
    VPCLMULQDQ,
    AVX512VNNI,
    AVX512BITALG,
    TME_EN, // TODO: Is this Intel specific?
    AVX512VPOPCNTDQ,
    LA57,
    MAWAU, // Intel specific
    RDPID, // IA32
    KL,
    BUS_LOCK_DETECT,
    CLDEMOTE,
    MOVDIRI,
    MOVDIR64B,
    ENQCMD,
    SGX_LC, // Intel software-guard specific
    PKS,
    // TODO: Other instructions.
};

struct CpuidRegisters {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
};

struct IsX86Feat {
private:
    struct CpuidRegisters regs[3];    
    const unsigned int IDX0 = 0;
    const unsigned int IDX1 = 1;
    const unsigned int IDX2 = 2;

    inline bool check(unsigned int reg, unsigned int bit) const {
	return ((reg & (1 << bit)) != 0);
    }
public:
    IsX86Feat() {
        regs[0] = { .eax = 0, .ebx = 0, .ecx = 0, .edx = 0 };
	regs[1] = { .eax = 0, .ebx = 0, .ecx = 0, .edx = 0 };
	regs[2] = { .eax = 0, .ebx = 0, .ecx = 0, .edx = 0 };

        __cpuid(1, regs[IDX0].eax, regs[IDX0].ebx,
		regs[IDX0].ecx, regs[IDX0].edx);
	__cpuid(6, regs[IDX1].eax, regs[IDX1].ebx,
		regs[IDX1].ecx, regs[IDX1].edx);
	__cpuid(7, regs[IDX2].eax, regs[IDX2].ebx,
		regs[IDX2].ecx, regs[IDX2].edx);
    }

    inline bool is_vendor_intel() const {
	unsigned int eax, ebx, ecx, edx;

	__cpuid(0, eax, ebx, ecx, edx);
	return (ebx == 0x756e6547 && edx == 0x49656e69 &&
		ecx == 0x6c65746e);
    }

    inline bool is_vendor_amd() const {
	return (!is_vendor_intel());
    }

    inline bool has(Feature type) const {
	switch (type) {
	case Feature::FPU:    return check(regs[IDX0].edx, 0);
	case Feature::VME:    return check(regs[IDX0].edx, 1);
	case Feature::DE:     return check(regs[IDX0].edx, 2);
	case Feature::PSE:    return check(regs[IDX0].edx, 3);
	case Feature::TSC:    return check(regs[IDX0].edx, 4);
	case Feature::MSR:    return check(regs[IDX0].edx, 5);
	case Feature::PAE:    return check(regs[IDX0].edx, 6);
	case Feature::MCE:    return check(regs[IDX0].edx, 7);
	case Feature::CX8:    return check(regs[IDX0].edx, 8);
	case Feature::APIC:   return check(regs[IDX0].edx, 9);
	case Feature::SEP:    return check(regs[IDX0].edx, 11);
	case Feature::MTRR:   return check(regs[IDX0].edx, 12);
	case Feature::PGE:    return check(regs[IDX0].edx, 13);
	case Feature::MCA:    return check(regs[IDX0].edx, 14);
	case Feature::CMOV:   return check(regs[IDX0].edx, 15);
	case Feature::PAT:    return check(regs[IDX0].edx, 16);
	case Feature::PSE36:  return check(regs[IDX0].edx, 17);
	case Feature::CLFSH:  return check(regs[IDX0].edx, 19);
	case Feature::DS:     return check(regs[IDX0].edx, 21);
	case Feature::ACPI:   return check(regs[IDX0].edx, 22);
	case Feature::MMX:    return check(regs[IDX0].edx, 23);
	case Feature::FXSR:   return check(regs[IDX0].edx, 24);
	case Feature::SSE:    return check(regs[IDX0].edx, 25);
	case Feature::SSE2:   return check(regs[IDX0].edx, 26);
	case Feature::SS:     return check(regs[IDX0].edx, 27);
	case Feature::HTT:    return check(regs[IDX0].edx, 28);
	case Feature::TM:     return check(regs[IDX0].edx, 29);
	case Feature::IA64:   return check(regs[IDX0].edx, 30);
	case Feature::PBE:    return check(regs[IDX0].edx, 31);
        case Feature::PSN:    return is_vendor_intel() ?
		check(regs[IDX0].edx, 18) : false;

	case Feature::SSE3: return check(regs[IDX0].ecx, 0);
	case Feature::PCLMULQDQ: return check(regs[IDX0].ecx, 1);
	case Feature::DTES64:  return check(regs[IDX0].ecx, 2);
	case Feature::MONITOR:  return check(regs[IDX0].ecx, 3);
	case Feature::DS_CPL:  return check(regs[IDX0].ecx, 4);
	case Feature::VMX:  return check(regs[IDX0].ecx, 5);
	case Feature::SMX:  return check(regs[IDX0].ecx, 6);
	case Feature::EST:  return check(regs[IDX0].ecx, 7);
	case Feature::TM2:  return check(regs[IDX0].ecx, 8);
	case Feature::SSSE3:  return check(regs[IDX0].ecx, 9);
	case Feature::CNXT_ID:  return check(regs[IDX0].ecx, 10);
	case Feature::SDBG:  return check(regs[IDX0].ecx, 11);
	case Feature::FMA:  return check(regs[IDX0].ecx, 12);
	case Feature::CX16:  return check(regs[IDX0].ecx, 13);
	case Feature::XTPR:  return check(regs[IDX0].ecx, 14);
	case Feature::PDCM:  return check(regs[IDX0].ecx, 15);
	case Feature::PCID: return check(regs[IDX0].ecx, 17);
	case Feature::DCA: return check(regs[IDX0].ecx, 18);
	case Feature::SSE41: return check(regs[IDX0].ecx, 19);
	case Feature::SSE42: return check(regs[IDX0].ecx, 20);
	case Feature::X2APIC: return check(regs[IDX0].ecx, 21);
	case Feature::MOVBE: return check(regs[IDX0].ecx, 22);
	case Feature::POPCNT: return check(regs[IDX0].ecx, 23);
	case Feature::TSC_DEADLINE: return check(regs[IDX0].ecx, 24);
	case Feature::AES_NI: return check(regs[IDX0].ecx, 25);
	case Feature::XSAVE: return check(regs[IDX0].ecx, 26);
	case Feature::OSXSAVE: return check(regs[IDX0].ecx, 27);
	case Feature::AVX: return check(regs[IDX0].ecx, 28);
	case Feature::F16C: return check(regs[IDX0].ecx, 29);
	case Feature::RDRND: return check(regs[IDX0].ecx, 30);
	case Feature::HYPERVISOR: return check(regs[IDX0].ecx, 31);

	    // EAX = 6, thermal and power management
	case Feature::DTS: return check(regs[IDX1].eax, 0);
	    // On Pentium 4, this bit is used to specify Operating Point Protection.
	case Feature::ARAT: return check(regs[IDX1].eax, 2);
	case Feature::PLN: return check(regs[IDX1].eax, 4);
	case Feature::ECMD: return check(regs[IDX1].eax, 5);
	case Feature::PTM: return check(regs[IDX1].eax, 6);

	    // EAX = 7, ECX = 0, extended features
	case Feature::FSGSBASE: return check(regs[IDX2].ebx, 0);
	case Feature::SGX: return check(regs[IDX2].ebx, 2);
	case Feature::BMI1: return check(regs[IDX2].ebx, 3);
	case Feature::HLE: return check(regs[IDX2].ebx, 4);
	case Feature::AVX2: return check(regs[IDX2].ebx, 5);
	case Feature::FDP_EXCPTN_ONLY: return check(regs[IDX2].ebx, 6);
	case Feature::SMEP: return check(regs[IDX2].ebx, 7);
	case Feature::BMI2: return check(regs[IDX2].ebx, 8);
	case Feature::ERMS: return check(regs[IDX2].ebx, 9);
	case Feature::INVPCID: return check(regs[IDX2].ebx, 10);
	case Feature::RTM: return check(regs[IDX2].ebx, 11);
	case Feature::PQM: // AMD uses PQM
	case Feature::RDT_M: return check(regs[IDX2].ebx, 12);
	case Feature::MPX:
	    return is_vendor_intel() ? check(regs[IDX2].ebx, 14) : false;
	case Feature::RDT_A: return check(regs[IDX2].ebx, 15);
	case Feature::AVX512F: return check(regs[IDX2].ebx, 16);
	case Feature::AVX512DQ: return check(regs[IDX2].ebx, 17);
	case Feature::RDSEED: return check(regs[IDX2].ebx, 18);
	case Feature::ADX:
	    return is_vendor_intel() ? check(regs[IDX2].ebx, 19) : false;
	case Feature::SMAP: return check(regs[IDX2].ebx, 20);
	case Feature::AVX512IFMA: return check(regs[IDX2].ebx, 21);
	case Feature::CLFLUSHOPT: return check(regs[IDX2].ebx, 23);
	case Feature::CLWB: return check(regs[IDX2].ebx, 24);
	case Feature::PT: return is_vendor_intel() ? check(regs[IDX2].ebx, 25) : false;
	case Feature::AVX512PF: return check(regs[IDX2].ebx, 26);
	case Feature::AVX512ER: return check(regs[IDX2].ebx, 27);
	case Feature::AVX512CD: return check(regs[IDX2].ebx, 28);
	case Feature::SHA: return check(regs[IDX2].ebx, 29);
	case Feature::AVX512BW: return check(regs[IDX2].ebx, 30);
	case Feature::AVX512VL: return check(regs[IDX2].ebx, 31);

	case Feature::PREFETCHWT1: return check(regs[IDX2].ecx, 0);
	case Feature::AVX512VBMI: return check(regs[IDX2].ecx, 1);
	case Feature::UMIP: return check(regs[IDX2].ecx, 2);
	case Feature::PKU: return check(regs[IDX2].ecx, 3);
	case Feature::OSPKE: return check(regs[IDX2].ecx, 4);
	case Feature::WAITPKG: return check(regs[IDX2].ecx, 5);
	case Feature::AVX512VBMI2: return check(regs[IDX2].ecx, 6);
	case Feature::CET_SS: return check(regs[IDX2].ecx, 7);
	case Feature::GFNI: return check(regs[IDX2].ecx, 8);
	case Feature::VAES: return check(regs[IDX2].ecx, 9);
	case Feature::VPCLMULQDQ: return check(regs[IDX2].ecx, 10);
	case Feature::AVX512VNNI: return check(regs[IDX2].ecx, 11);
	case Feature::AVX512BITALG: return check(regs[IDX2].ecx, 12);
	case Feature::TME_EN: return check(regs[IDX2].ecx, 13);
	case Feature::AVX512VPOPCNTDQ: return check(regs[IDX2].ecx, 14);
	case Feature::LA57: return check(regs[IDX2].ecx, 16);
	case Feature::MAWAU: return is_vendor_intel() ? check(regs[IDX2].ecx, 17) : false;
	case Feature::RDPID: return check(regs[IDX2].ecx, 22);
	case Feature::KL: return check(regs[IDX2].ecx, 23);
	case Feature::BUS_LOCK_DETECT: return check(regs[IDX2].ecx, 24);
	case Feature::CLDEMOTE: return check(regs[IDX2].ecx, 25);
	case Feature::MOVDIRI: return check(regs[IDX2].ecx, 27);
	case Feature::MOVDIR64B: return check(regs[IDX2].ecx, 28);
	case Feature::ENQCMD: return check(regs[IDX2].ecx, 29);
	case Feature::SGX_LC: return is_vendor_intel() ? check(regs[IDX2].ecx, 30) : false;
	case Feature::PKS: return check(regs[IDX2].ecx, 31);
	    // It shouldn't be reachable as we can't stop
	    // someone to pass values explicitly casted with
	    // Feature enum type.
	default: __builtin_abort();
        }
    }
};

#endif
