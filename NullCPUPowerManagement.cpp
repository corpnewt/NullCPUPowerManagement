/*! @file       NullCPUPowerManagement.h
    @copyright  David Elliott
    @created    2007-11-26
    @rcsid      $Id: NullCPUPowerManagement.cpp 10 2008-06-29 19:34:05Z dfe $
 */

#include "NullCPUPowerManagement.h"

#include <IOKit/IOLib.h>
OSDefineMetaClassAndStructors(NullCPUPowerManagement, IOService);

#if 0

#define bit(n)		(1ULL << (n))
#define bitmask(h,l)	((bit(h)|(bit(h)-1)) & ~(bit(l)-1))
#define bitfield(x,h,l)	(((x) & bitmask(h,l)) >> l)

/*
 *	Memory mapped registers for the HPET
 */
typedef struct hpetReg {
	uint64_t	GCAP_ID;		/* General capabilities */
	uint64_t	rsv1;
	uint64_t	GEN_CONF;		/* General configuration */
	uint64_t	rsv2;
	uint64_t	GINTR_STA;		/* General Interrupt status */
	uint64_t	rsv3[25];
	uint64_t	MAIN_CNT;		/* Main counter */
	uint64_t	rsv4;
	uint64_t	TIM0_CONF;		/* Timer 0 config and cap */
#define			TIM_CONF 0
#define			Tn_INT_ENB_CNF 4
	uint64_t	TIM0_COMP;		/* Timer 0 comparator */
#define			TIM_COMP 8
	uint64_t	rsv5[2];
	uint64_t	TIM1_CONF;		/* Timer 1 config and cap */
	uint64_t	TIM1_COMP;		/* Timer 1 comparator */
	uint64_t	rsv6[2];
	uint64_t	TIM2_CONF;		/* Timer 2 config and cap */
	uint64_t	TIM2_COMP;		/* Timer 2 comparator */
	uint64_t	rsv7[2];
} hpetReg;
typedef struct 	hpetReg hpetReg_t;

struct hpetInfo
{
	uint64_t	hpetCvtt2n;
	uint64_t	hpetCvtn2t;
	uint64_t	tsc2hpet;
	uint64_t	hpet2tsc;
	uint64_t	bus2hpet;
	uint64_t	hpet2bus;
	uint32_t	rcbaArea;
	uint32_t	rcbaAreap;
};
typedef struct hpetInfo hpetInfo_t;

#define hpetAddr 	0xFED00000
#define hptcAE 		0x80

extern "C" void hpet_get_info(hpetInfo_t *info);

static inline bool hpetIsInvalid()
{
    hpetInfo_t hpetInfo;
    hpet_get_info(&hpetInfo);
    // The AppleIntelCPUPowerManagement will crash if rcbaArea and/or HPET is NULL
    // Ordinarily this can never happen but modified xnu can allow for this.
    if(hpetInfo.rcbaArea == 0)
    {
        IOLog("Forcing takeover of AppleIntelCPUPowerManagement resource due to lack of RCBA (no LPC?)\n");
        return true;
    }
    // Another case is that the LPC exists but the HPET isn't really valid.
    // That is to say that virtual hardware provides enough to get past xnu startup
    // but not enough to really make the HPET work.

    uint32_t hptc = *(uint32_t*)(hpetInfo.rcbaArea + 0x3404);
    if(!(hptc & hptcAE))
    {
        IOLog("Forcing takeover of AppleIntelCPUPowerManagement resource because HPET is not enabled\n");
        return true;
    }
    // Use the RCBA's HPTC to determine which of the four possible HPET physical addresses is used
    uint32_t hpetAreap = hpetAddr | ((hptc & 3) << 12);
    IOMemoryDescriptor *hpetMemDesc = IOMemoryDescriptor::withPhysicalAddress(hpetAreap, sizeof(hpetReg_t), kIODirectionIn);

    // grab the GCAP_ID (note offset is actually 0)
    uint64_t GCAP_ID;
    hpetMemDesc->readBytes(offsetof(hpetReg_t, GCAP_ID), &GCAP_ID, sizeof(GCAP_ID));

    // We're done with the memory descriptor now, so release it
    hpetMemDesc->release();
    hpetMemDesc = NULL;

    // Extract the VENDOR_ID_CAP field from the GCAP_ID and test it
    uint16_t vendorId = bitfield(GCAP_ID, 31, 16);
    if( (vendorId == 0x0000) || (vendorId == 0xffff))
    {
        IOLog("Forcing takeover of AppleIntelCPUPowerManagement resource due to bad HPET VENDOR_ID_CAP\n");
        return true;
    }
    else if(vendorId != 0x8086)
    {
        IOLog("WARNING: HPET is not Intel.  Going ahead and allowing AppleIntelCPUPowerManagement to start but beware it may behave strangely\n");
    }
    return false;
}
#endif

bool NullCPUPowerManagement::init(OSDictionary *properties)
{
    IOLog("NullCPUPowerManagement::init: properties=%p\n", properties);
    OSObject *matchCategoryValue;
    if(properties != NULL)
    {
        matchCategoryValue = properties->getObject(gIOMatchCategoryKey);
    }
    else
        matchCategoryValue = NULL;

    // Do the normal superclass init
    if(!IOService::init(properties))
        return false;

    if(matchCategoryValue != NULL)
    {
        // This is the secret sauce that causes this kext to become a valid potential
        // The trick is to set the name to the value of gIOMatchCategoryKey

        OSSymbol const *classNameToDisable = OSDynamicCast(OSSymbol, matchCategoryValue);
        if(classNameToDisable != NULL)
        {
            classNameToDisable->retain(); // retain
        }
        else
        {
            OSString *stringClassName = OSDynamicCast(OSString, matchCategoryValue);
            if(stringClassName != NULL)
                classNameToDisable = OSSymbol::withString(stringClassName); // retain (other path)
        }

        if(classNameToDisable != NULL)
        {
            setName(classNameToDisable, NULL /* All planes */);
            classNameToDisable->release(); // release either the retain or the withString call.
        }
        else
            IOLog("NullCPUPowerManagement::init: Failed to cast the IOMatchCategory value to something string-like\n");
    }
    else
        IOLog("NullCPUPowerManagement::init: Failed to retrieve IOMatchCategory from properties\n");

    // Succeed init in all cases
    return true;
}

IOService *NullCPUPowerManagement::probe(IOService *provider, SInt32 *score)
{
    if(IOService::probe(provider, score) == NULL)
        return NULL;
#if 0
    // If the hpet's invalid then succeed probe
    if(hpetIsInvalid())
    {
        IOLog("NullCPUPowerManagement::probe: Claiming match category with probe score %ld.\n", score!=NULL?*score:-1);
        return this;
    }
#endif
    char crap[20];
    if(PE_parse_boot_argn("-allowAppleCPUPM", crap, sizeof(crap)))
    {
        IOLog("NullCPUPowerManagement is respecting the -allowAppleCPUPM flag.\n");
        return NULL;
    }
    // Always succeed until we can figure out a better way of detecting a valid HPET.
    return this;
}

bool NullCPUPowerManagement::start(IOService *provider)
{
    IOLog("NullCPUPowerManagement::start\n");
    return IOService::start(provider);
}

void NullCPUPowerManagement::stop(IOService *provider)
{
    IOLog("NullCPUPowerManagement::stop\n");
    return IOService::stop(provider);
}
