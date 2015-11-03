#include "../include/nds3/pvBase.h"
#include "../include/nds3impl/pvBaseImpl.h"

namespace nds
{

/*
 * Default constructor.
 *
 **********************/
PVBase::PVBase()
{

}


/*
 * Construct a PV with a reference to an already existing implementation.
 *
 ************************************************************************/
PVBase::PVBase(std::shared_ptr<PVBaseImpl> pvImpl): Base(std::static_pointer_cast<BaseImpl>(pvImpl))
{

}


/*
 * Set the enumeration strings
 *
 *****************************/
void PVBase::setEnumeration(const enumerationStrings_t &enumerations)
{
    std::static_pointer_cast<PVBaseImpl>(m_pImplementation)->setEnumeration(enumerations);
}


/*
 * Set the description
 *
 *********************/
void PVBase::setDescription(const std::string& description)
{
    std::static_pointer_cast<PVBaseImpl>(m_pImplementation)->setDescription(description);
}


/*
 * Set the external name
 *
 ***********************/
void PVBase::setInterfaceName(const std::string& interfaceName)
{
    std::static_pointer_cast<PVBaseImpl>(m_pImplementation)->setInterfaceName(interfaceName);
}


/*
 * Set the scan type
 *
 *******************/
void PVBase::setScanType(const scanType_t scanType, const double periodSeconds)
{
    std::static_pointer_cast<PVBaseImpl>(m_pImplementation)->setScanType(scanType, periodSeconds);
}


/*
 * Set the maximum number of elements that can be written or read from the PV
 *
 ****************************************************************************/
void PVBase::setMaxElements(const size_t maxElements)
{
    std::static_pointer_cast<PVBaseImpl>(m_pImplementation)->setMaxElements(maxElements);
}

}
