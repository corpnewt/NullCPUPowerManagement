/*! @file       NullCPUPowerManagement.h
    @copyright  David Elliott
    @created    2007-11-26
    @rcsid      $Id: NullCPUPowerManagement.h 5 2008-06-29 06:08:01Z dfe $
 */

#include <IOKit/IOService.h>

class NullCPUPowerManagement: public IOService
{
    OSDeclareDefaultStructors(NullCPUPowerManagement);
public:
    /*!
        Overridden to change the nub name in all planes from the class name to
        the value of the IOMatchCategory key in the personality dictionary.

        This works around a check in the kernel which refuses to consider
        an IOResources match where the nub name does not match the category.
     */
    virtual bool init(OSDictionary *properties = 0);

    /*!
        Overriden to determine whether or not this service should start and thus
        push out the real AppleIntelCPUPowerManagement service.
     */
    virtual IOService *probe(IOService *provider, SInt32 *score);

    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);
protected:
};
