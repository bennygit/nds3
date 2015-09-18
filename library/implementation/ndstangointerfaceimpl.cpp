#include "ndstangointerfaceimpl.h"
#include "ndstangofactoryimpl.h"
#include "ndspvbaseimpl.h"
#include "ndsnodeimpl.h"

#include <cstdio>
namespace nds
{

TangoInterfaceImpl::TangoInterfaceImpl(const string &portName, NdsDevice* pDevice): InterfaceBaseImpl(portName), m_pDevice(pDevice)
{

}

void TangoInterfaceImpl::registerPV(std::shared_ptr<PVBaseImpl> pv)
{
    Tango::CmdArgType tangoType;
    bool bArray(false);
    switch(pv->getDataType())
    {
    case dataType_t::dataInt8:
        tangoType = Tango::DEV_UCHAR;
        break;
    case dataType_t::dataUint8:
        tangoType = Tango::DEV_UCHAR;
        break;
    case dataType_t::dataInt32:
        tangoType = Tango::DEV_LONG;
        break;
    case dataType_t::dataUint32:
        tangoType = Tango::DEV_ULONG;
        break;
    case dataType_t::dataFloat64:
        tangoType = Tango::DEV_DOUBLE;
        break;
    case dataType_t::dataInt8Array:
        tangoType = Tango::DEV_UCHAR;
        bArray = true;
        break;
    case dataType_t::dataUint8Array:
        tangoType = Tango::DEV_UCHAR;
        bArray = true;
        break;
    case dataType_t::dataInt32Array:
        tangoType = Tango::DEV_LONG;
        bArray = true;
        break;
    case dataType_t::dataFloat64Array:
        tangoType = Tango::DEV_DOUBLE;
        bArray = true;
        break;
    case dataType_t::dataString:
        tangoType = Tango::DEV_STRING;
        break;
    }
    std::cout << pv->getFullName() << std::endl;

    recordType_t pvType = pv->getType();
    Tango::AttrWriteType writeType;
    if(((int)pvType & 0x00c0) == 0x0080)
    {
        writeType = writeType = Tango::READ;
    }
    else if(((int)pvType & 0x00c0) == 0x0040)
    {
        writeType = writeType = Tango::WRITE;
    }
    else if(((int)pvType & 0x00c0) == 0x00c0)
    {
        throw;
    }

    if(bArray)
    {
        m_pDevice->add_attribute(new NdsAttributeSpectrum(pv->getFullName(), pv, tangoType, writeType, pv->getMaxElements()));
    }
    else
    {
        m_pDevice->add_attribute(new NdsAttributeScalar(pv->getFullName(), pv, tangoType, writeType));
    }

    // Find the root node and store it in the device
    std::shared_ptr<NodeImpl> rootNode;
    for(rootNode = pv->getParent(); rootNode->getParent() != 0; rootNode = rootNode->getParent())
    {}
    m_pDevice->setRootNode(rootNode);

}

void TangoInterfaceImpl::registrationTerminated()
{

}

void TangoInterfaceImpl::push(std::shared_ptr<PVBaseImpl> pv, const timespec& timestamp, const std::int32_t& value)
{

}

void TangoInterfaceImpl::push(std::shared_ptr<PVBaseImpl> pv, const timespec& timestamp, const double& value)
{

}

void TangoInterfaceImpl::push(std::shared_ptr<PVBaseImpl> pv, const timespec& timestamp, const std::vector<std::int32_t> & value)
{

}

NdsDevice::NdsDevice(Tango::DeviceClass* pClass, string &parameter): TANGO_BASE_CLASS(pClass, parameter.c_str()), m_pClass(pClass), m_parameter(parameter)
{
    init_device();
}

void NdsDevice::init_device()
{
    TangoFactoryImpl::getInstance().setLastCreatedDevice(this);
    TangoFactoryImpl::getInstance().createDriver(m_pClass->get_name(), m_parameter);
}

void NdsDevice::setRootNode(std::shared_ptr<NodeImpl> pRootNode)
{
    m_pRootNode = pRootNode;
}

Tango::DevState NdsDevice::get_state()
{
    state_t deviceState = m_pRootNode->getGlobalState();


    switch(deviceState)
    {
    case state_t::unknown:
        return Tango::UNKNOWN;
    case state_t::fault:
        return Tango::FAULT;
    case state_t::initializing:
        return Tango::INIT;
    case state_t::off:
        return Tango::OFF;
    case state_t::on:
        return Tango::ON;
    case state_t::running:
        return Tango::RUNNING;
    case state_t::starting:
        return Tango::ON;
    case state_t::stopping:
        return Tango::ON;
    case state_t::switchingOff:
        return Tango::OFF;
    }
}

/*
 *
 * TANGO ATTRIBUTE
 *
 *
 *****************/

NdsAttributeBase::NdsAttributeBase(std::shared_ptr<PVBaseImpl> pv): m_pPV(pv)
{

}

void NdsAttributeBase::setAttributeProperties(Tango::Attr& attr)
{
    Tango::UserDefaultAttrProp properties;
    properties.set_description(m_pPV->getDescription().c_str());
    properties.set_label(m_pPV->getComponentName().c_str());

    attr.set_default_properties(properties);

}

/*
 * Constructor
 *
 *************/
NdsAttributeScalar::NdsAttributeScalar(const std::string& name, std::shared_ptr<PVBaseImpl> pPV, Tango::CmdArgType dataType, Tango::AttrWriteType writeType) :
    NdsAttributeBase(pPV), Tango::Attr(name.c_str(), dataType, writeType)
{
    setAttributeProperties(*this);
}

/*
 * Read a scalar value
 *
 *********************/
template<typename NdsType_t, typename TangoType_t>
void NdsAttributeScalar::readValue(Tango::Attribute &att)
{
    NdsType_t value;
    timespec timestamp;
    m_pPV->read(&timestamp, &value);
    TangoType_t returnValue = (TangoType_t)value;
    timeval tangoTime(NDSTimeToTangoTime(timestamp));
    att.set_value_date_quality(&returnValue, tangoTime, Tango::ATTR_VALID, 1, 0, false);

    std::cout << m_pPV->getFullName() << " " << att.get_name() << " " << returnValue << "\n";
}

template<typename NdsType_t, typename TangoType_t>
void NdsAttributeScalar::writeValue(Tango::Attribute &att, const TangoType_t& value)
{
    std::cout << "WRITE " << m_pPV->getFullName() << " " << att.get_name() << " " << value << "\n";
    Tango::TimeVal timestamp(att.get_date());
    timespec ndsTime;
    ndsTime.tv_sec = timestamp.tv_sec;
    ndsTime.tv_nsec = timestamp.tv_nsec;

    NdsType_t ndsValue = (NdsType_t)value;

    m_pPV->write(ndsTime, ndsValue);
}

void NdsAttributeScalar::read(Tango::DeviceImpl *dev,Tango::Attribute &att)
{
    long dataType = att.get_data_type();

    switch(dataType)
    {
    case Tango::DEV_LONG:
        readValue<std::int32_t, Tango::DevLong>(att);
        break;
    case Tango::DEV_DOUBLE:
        readValue<double, Tango::DevDouble>(att);
        break;
    case Tango::DEV_ULONG:
        readValue<std::uint32_t, Tango::DevULong>(att);
        break;
    }
}

void NdsAttributeScalar::write(Tango::DeviceImpl *dev,Tango::WAttribute &att)
{
    long dataType = att.get_data_type();

    switch(dataType)
    {
    case Tango::DEV_LONG:
        writeValue<std::int32_t, Tango::DevLong>(att, (*att.get_long_value())[0]);
        break;
    case Tango::DEV_DOUBLE:
        writeValue<double, Tango::DevDouble>(att, (*att.get_double_value())[0]);
        break;
    case Tango::DEV_ULONG:
        writeValue<std::uint32_t, Tango::DevULong>(att, (*att.get_ulong_value())[0]);
        break;
    }
}

bool NdsAttributeScalar::is_allowed(Tango::DeviceImpl *dev,Tango::AttReqType ty)
{
    return true;
}

NdsAttributeSpectrum::NdsAttributeSpectrum(const string &name, std::shared_ptr<PVBaseImpl> pPV, Tango::CmdArgType dataType, Tango::AttrWriteType writeType, size_t maxLength):
    NdsAttributeBase(pPV), Tango::SpectrumAttr(name.c_str(), dataType, writeType, maxLength)
{
    setAttributeProperties(*this);
}

void NdsAttributeSpectrum::read(Tango::DeviceImpl *dev,Tango::Attribute &att)
{
}

void NdsAttributeSpectrum::write(Tango::DeviceImpl *dev,Tango::WAttribute &att)
{
}

bool NdsAttributeSpectrum::is_allowed(Tango::DeviceImpl *dev,Tango::AttReqType ty)
{
    return true;
}

timeval NdsAttributeBase::NDSTimeToTangoTime(const timespec& time)
{
    timeval tangoTime;
    tangoTime.tv_sec = time.tv_sec;
    tangoTime.tv_usec = time.tv_nsec / 1000;
    return tangoTime;
}

timespec NdsAttributeBase::TangoTimeToNDSTime(const timeval& time)
{
    timespec ndsTime;
    ndsTime.tv_sec = time.tv_sec;
    ndsTime.tv_nsec = time.tv_usec * 1000;
    return ndsTime;
}

NdsDeviceClass::NdsDeviceClass(std::string& name, allocateDriver_t allocateDriverFunction, deallocateDriver_t deallocateDriverFunction):
    Tango::DeviceClass(name),
    m_allocateDriverFunction(allocateDriverFunction), m_deallocateDriverFunction(deallocateDriverFunction)
{

}

void NdsDeviceClass::command_factory()
{
    /*
    command_list.push_back(new OnClass());
    command_list.push_back(new OffClass());
    command_list.push_back(new StartClass());
    command_list.push_back(new StopClass());
    command_list.push_back(new ResetClass());

    */
}

void NdsDeviceClass::attribute_factory(vector<Tango::Attr *> &attributes)
{
}

void NdsDeviceClass::device_factory(const Tango::DevVarStringArray* dev_list)
{
    for (unsigned long i=0 ; i  < dev_list->length() ; i++)
    {
        std::cout << "Device name : " << (*dev_list)[i].in() << endl;
        std::string parameter((*dev_list)[i]);
        NdsDevice* pNewDevice(new NdsDevice(this, parameter));
        device_list.push_back(pNewDevice);

        //	Check before if database used.
        if ((Tango::Util::_UseDb == true) && (Tango::Util::_FileDb == false))
            export_device(pNewDevice);
        else
            export_device(pNewDevice, pNewDevice->get_name().c_str());

    }

}


}
