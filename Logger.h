#pragma once

#include <iosfwd>
#include <inttypes.h>

#undef DECLARE_VARIABLE
#undef DECLARE_int32

#define DECLARE_VARIABLE(type, shorttype, name) \
    namespace fl##shorttype {                   \
        extern type FLAG_##name;                \
    }                                           \
    using fl##shorttype::FLAG_##name

#define DECLARE_INT(name) \
    DECLARE_VARIABLE(int32_t, I, name)

#define DECLARE_BOOL(name) \
    DECLARE_VARIABLE(bool, B, name)

#define DECLARE_CHARPTR(name) \
    DECLARE_VARIABLE(const char*, CPTR, name)

DECLARE_INT(SEVERITY);
DECLARE_INT(REFRESH_INTERVAL);
DECLARE_BOOL(STDERR);
DECLARE_CHARPTR(BASE_FILENAME);

#define LOG(severity) if (severity <= FLAG_SEVERITY) \
    buzz::LogMessage(severity, __FILE__, __LINE__).Stream()

typedef int LogSeverity;

const LogSeverity FATAL = 0, ERROR = 1, WARN = 2, INFO = 3,
                  DEBUG = 4, TRACE = 5, ALL = 6;
#include <assert.h>
#include <sstream>

namespace buzz
{
    namespace BaseLogger {
        class LogStreamBuf : public std::streambuf
        {
        public:
            LogStreamBuf(char* buf, size_t size)
            {
                assert(size > 2);
                setp(buf, buf + size - 1);
            }

            virtual int_type overflow(int_type ch) { return ch; }

            inline size_t pcount() const { return pptr() - pbase(); }
            inline char*  pbase()  const { return std::streambuf::pbase(); }
        };

        class LogStream : public std::ostream
        {
        public:
            LogStream(char* buf, size_t size)
                : std::ostream(NULL), m_streambuf(buf, size)
            {
                rdbuf(&m_streambuf);
            }

            inline char*  str()    const { return pbase(); }
            inline char*  pbase()  const { return m_streambuf.pbase(); }
            inline size_t pcount() const { return m_streambuf.pcount(); }

        private:
            LogStreamBuf m_streambuf;
        };
    }

    class LogMessage
    {
    public:
        LogMessage(LogSeverity severity, const char* file, int line);
        ~LogMessage();

        std::ostream& Stream();

        struct LogMessageData;
    private:
        LogMessageData* m_data;

        static const size_t kMaxLogMessageLen = 30000;
    };
}