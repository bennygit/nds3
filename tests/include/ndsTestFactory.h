#include <nds3impl/factoryBaseImpl.h>
#include <nds3impl/logStreamGetterImpl.h>
#include <set>
#include <sstream>

namespace nds
{

namespace tests
{

class TestControlSystemFactoryImpl: public FactoryBaseImpl, public LogStreamGetterImpl
{
public:
    TestControlSystemFactoryImpl();

    virtual const std::string getName() const;

    virtual InterfaceBaseImpl* getNewInterface(const std::string& fullName);

    virtual void run(int argc,char *argv[]);

    virtual LogStreamGetterImpl* getLogStreamGetter();

    virtual void registerCommand(const BaseImpl& node,
                                 const std::string& command,
                                 const std::string& usage,
                                 const size_t numParameters, command_t commandFunction);

    virtual void deregisterCommand(const BaseImpl& node);

    void log(const std::string& logString, const logLevel_t logLevel);

    size_t countStringInLog(const std::string& string);

    static TestControlSystemFactoryImpl* getInstance();

protected:
    virtual std::ostream* createLogStream(const logLevel_t logLevel);

    std::multiset<std::string> m_logs;
    std::mutex m_logMutex;
};

class TestLogStreamBufferImpl: public std::stringbuf
{
public:
    TestLogStreamBufferImpl(const logLevel_t logLevel, TestControlSystemFactoryImpl* pFactory);

protected:
    virtual int sync();

    logLevel_t m_logLevel;
    TestControlSystemFactoryImpl* m_pFactory;
};

class TestLogStream: public std::ostream
{
public:
    TestLogStream(const logLevel_t logLevel, TestControlSystemFactoryImpl* pFactory);

protected:
    TestLogStreamBufferImpl m_buffer;

};

}

}
