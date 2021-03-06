/*
 * Nominal Device Support v3 (NDS3)
 *
 * Copyright (c) 2015 Cosylab d.d.
 *
 * For more information about the license please refer to the license.txt
 * file included in the distribution.
 */

#include <memory>
#include <mutex>
#include <sstream>

#include "nds3/definitions.h"
#include "nds3/impl/nodeImpl.h"
#include "nds3/impl/stateMachineImpl.h"
#include "nds3/impl/factoryBaseImpl.h"

namespace nds
{

static std::mutex m_initializationMutex;

NodeImpl::NodeImpl(const std::string &name, const nodeType_t nodeType): BaseImpl(name), m_nodeType(nodeType)
{}

void NodeImpl::addChild(std::shared_ptr<BaseImpl> pChild)
{
    std::string name(pChild->getComponentName());

    if(m_children.find(name) != m_children.end())
    {
        std::ostringstream errorString;
        errorString << "A node with the name " << name << " has already been registered into " << getFullName() << std::endl;
        throw std::logic_error(errorString.str());
    }

    m_children[name] = pChild;

    std::shared_ptr<StateMachineImpl> stateMachine = std::dynamic_pointer_cast<StateMachineImpl>(pChild);
    if(stateMachine.get() != 0)
    {
        m_pStateMachine = stateMachine;

        // Add the state machine commands also to this node
        ///////////////////////////////////////////////////
        defineCommand("switchOn", "", 0, std::bind(&StateMachineImpl::commandSetState, m_pStateMachine, state_t::on, std::placeholders::_1));
        defineCommand("switchOff", "", 0, std::bind(&StateMachineImpl::commandSetState, m_pStateMachine, state_t::off, std::placeholders::_1));
        defineCommand("start", "", 0, std::bind(&StateMachineImpl::commandSetState, m_pStateMachine, state_t::running, std::placeholders::_1));
        defineCommand("stop", "", 0, std::bind(&StateMachineImpl::commandSetState, m_pStateMachine, state_t::on, std::placeholders::_1));
    }
}


void NodeImpl::initializeRootNode(void *pDeviceObject, FactoryBaseImpl &controlSystem)
{
    if(getParent().get() != 0)
    {
        throw std::logic_error("You can initialize only the root nodes");
    }

    std::lock_guard<std::mutex> serializeInitialization(m_initializationMutex);

    initialize(controlSystem);

    controlSystem.holdNode(pDeviceObject, std::static_pointer_cast<NodeImpl>(shared_from_this()) );
}

void NodeImpl::initialize(FactoryBaseImpl& controlSystem)
{
    BaseImpl::initialize(controlSystem);

    for(tChildren::iterator scanChildren(m_children.begin()), endScan(m_children.end()); scanChildren != endScan; ++scanChildren)
    {
        scanChildren->second->setParent(std::static_pointer_cast<NodeImpl>(shared_from_this()), m_nodeLevel);
    }
    for(tChildren::iterator scanChildren(m_children.begin()), endScan(m_children.end()); scanChildren != endScan; ++scanChildren)
    {
        scanChildren->second->initialize(controlSystem);
    }
}

void NodeImpl::deinitializeRootNode()
{
    if(getParent().get() != 0)
    {
        throw std::logic_error("You can call deinitialize only on root nodes");
    }

    std::lock_guard<std::mutex> serializeInitialization(m_initializationMutex);
    deinitialize();
}

void NodeImpl::deinitialize()
{
    BaseImpl::deinitialize();
    for(tChildren::iterator scanChildren(m_children.begin()), endScan(m_children.end()); scanChildren != endScan; ++scanChildren)
    {
        scanChildren->second->deinitialize();
    }
}

state_t NodeImpl::getLocalState() const
{
    if(m_pStateMachine.get() == 0)
    {
        return state_t::unknown;
    }
    return m_pStateMachine->getLocalState();
}

void NodeImpl::getGlobalState(timespec* pTimestamp, state_t* pState) const
{
    if(m_pStateMachine.get() == 0)
    {
        getChildrenState(pTimestamp, pState);
        return;
    }
    m_pStateMachine->getGlobalState(pTimestamp, pState);
}

void NodeImpl::getChildrenState(timespec* pTimestamp, state_t* pState) const
{
    *pState = state_t::unknown;
    for(tChildren::const_iterator scanChildren(m_children.begin()), endScan(m_children.end()); scanChildren != endScan; ++scanChildren)
    {
        if(scanChildren->second.get() != m_pStateMachine.get())
        {
            std::shared_ptr<NodeImpl> child = std::dynamic_pointer_cast<NodeImpl>(scanChildren->second);
            if(child.get() != 0)
            {
                timespec childTimestamp;
                state_t childState;
                child->getGlobalState(&childTimestamp, &childState);

                if(
                        ((int)*pState < (int)childState) ||
                        ((int)*pState == (int)childState &&
                         (pTimestamp->tv_sec < childTimestamp.tv_sec ||
                          (pTimestamp->tv_sec == childTimestamp.tv_sec && pTimestamp->tv_nsec < childTimestamp.tv_nsec))))
                {
                    *pTimestamp = childTimestamp;
                    *pState = childState;
                }
            }
        }
    }
}

void NodeImpl::setLogLevel(const logLevel_t logLevel)
{
    BaseImpl::setLogLevel(logLevel);

    for(tChildren::const_iterator scanChildren(m_children.begin()), endScan(m_children.end()); scanChildren != endScan; ++scanChildren)
    {
        scanChildren->second->setLogLevel(logLevel);
    }

}

std::string NodeImpl::buildFullExternalName(const FactoryBaseImpl& controlSystem) const
{
    return buildFullExternalName(controlSystem, false);
}

std::string NodeImpl::buildFullExternalNameFromPort(const FactoryBaseImpl& controlSystem) const
{
    return buildFullExternalName(controlSystem, true);
}

std::string NodeImpl::buildFullExternalName(const FactoryBaseImpl& controlSystem, const bool bStopAtPort) const
{
    std::shared_ptr<NodeImpl> temporaryPointer = m_pParent.lock();
    if(temporaryPointer == 0)
    {
        return controlSystem.getSeparator(0) + controlSystem.getRootNodeName(m_externalName);
    }

    std::string name;
    switch(m_nodeType)
    {
    case nodeType_t::generic:
        name = controlSystem.getGenericChannelName(m_externalName);
        break;
    case nodeType_t::inputChannel:
        name = controlSystem.getInputChannelName(m_externalName);
        break;
    case nodeType_t::outputChannel:
        name = controlSystem.getOutputChannelName(m_externalName);
        break;
    case nodeType_t::dataSourceChannel:
        name = controlSystem.getSourceChannelName(m_externalName);
        break;
    case nodeType_t::dataSinkChannel:
        name = controlSystem.getSinkChannelName(m_externalName);
        break;
    case nodeType_t::stateMachine:
        name = controlSystem.getStateMachineNodeName(m_externalName);
        break;
    }

    if(name.empty())
    {
        if(bStopAtPort)
        {
            return temporaryPointer->buildFullExternalNameFromPort(controlSystem);
        }
        else
        {
            return temporaryPointer->buildFullExternalName(controlSystem);
        }
    }

    if(bStopAtPort)
    {
        return temporaryPointer->buildFullExternalNameFromPort(controlSystem) + controlSystem.getSeparator(m_nodeLevel) + name;
    }
    return temporaryPointer->buildFullExternalName(controlSystem) + controlSystem.getSeparator(m_nodeLevel) + name;
}


}
