#include "Logger.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <fcntl.h>
#include <assert.h>

#include <mutex>
#include <memory>
#include <sstream>
#include <iomanip>

#undef DECLARE_VARIABLE

#define DECLARE_VARIABLE(type, shorttype, value, name) \
    namespace fl##shorttype {                          \
        type FLAG_##name(value);                        \
    }                                                  \
    using fl##shorttype::FLAG_##name

#define DEFINE_INT(name, value)\
    DECLARE_VARIABLE(int32_t, I, value, name)

#define DEFINE_BOOL(name, value)\
    DECLARE_VARIABLE(bool, B, value, name)

#define DEFINE_CHARPTR(name, value)\
    DECLARE_VARIABLE(const char* , CPTR, value ? value : "", name)

DEFINE_INT(SEVERITY, 6);
DEFINE_INT(REFRESH_INTERVAL, 30);
DEFINE_BOOL(STDERR, false);
DEFINE_CHARPTR(BASE_FILENAME, 0);

namespace buzz
{
    using namespace BaseLogger;

    static bool g_stop_writing = false;

    class LogFileObject
    {
    public:
        LogFileObject(LogSeverity severity, const char* baseFilename);
        ~LogFileObject() { if (m_file) { ::fclose(m_file); } }

        void Write(bool force_flush, Timestamp timestamp, const char* msg, size_t msg_size);

        inline void Flush()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            FlushUnlocked();
        }

    private:
        FILE*       m_file;
        LogSeverity m_severity;

        std::string m_base_filename;
        bool        m_base_filename_selected;

        size_t m_file_length;

        Timestamp m_next_flush_time;
        Timestamp now_last_rotate_time;
        
        size_t  m_bytes_since_cache_flush;
        int32_t kRotateInterval = 60 * 60 * 24;

        std::mutex m_mutex;

        void FlushUnlocked();

        bool CreateLogfile(const std::string& time_pid_string);
    };

    LogFileObject::LogFileObject(LogSeverity severity, const char* baseFilename)
        : m_file(NULL),
        m_severity(severity),
        m_base_filename(baseFilename != NULL ? baseFilename : ""),
        m_base_filename_selected(baseFilename != NULL),
        m_file_length(0),
        m_next_flush_time(),
        now_last_rotate_time(0),
        m_bytes_since_cache_flush(0)
    { }

    void LogFileObject::Write(
        bool force_flush, Timestamp timestamp, const char* msg, size_t msg_size)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        int64_t now_second  = timestamp.SecondsSinceEpoch();
        int64_t last_rotate = now_last_rotate_time.SecondsSinceEpoch();

        if (FLAG_STDERR) {
            m_file = stderr;
        } else if (m_file == NULL || now_second / kRotateInterval != last_rotate / kRotateInterval) {

            std::ostringstream time_pid_stream;
            time_pid_stream << timestamp.ToFormattedString("%Y%m%d.%H%M%S", false) << '.'
                            << ::getpid() << ".log";

            const std::string& time_pid_string = time_pid_stream.str();
            if (!CreateLogfile(time_pid_string)) {
                fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n", time_pid_string.c_str());
                return;
            }
        }

        if (g_stop_writing == false) {
            errno = 0;
            fwrite(msg, 1, msg_size, m_file);
            if (errno == ENOSPC) {
                g_stop_writing = true;
            }
            else {
                m_file_length += msg_size;
                m_bytes_since_cache_flush += msg_size;
            }

            if (force_flush || m_bytes_since_cache_flush >= 1000000 ||
                Timestamp::Now() >= m_next_flush_time) {
                FlushUnlocked();
            }
        }
    }

    void LogFileObject::FlushUnlocked()
    {
        if (m_file) {
            ::fflush(m_file);
            m_bytes_since_cache_flush = 0;
        }

        m_next_flush_time = AddTime(Timestamp::Now(), static_cast<double>(FLAG_REFRESH_INTERVAL));
    }

    bool LogFileObject::CreateLogfile(const std::string& time_pid_string)
    {
        std::string string_filename = m_base_filename + time_pid_string;
        const char* filename = string_filename.c_str();

        int fd = ::open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd == -1) return false;

        if (m_file) ::fclose(m_file);

        m_file = fdopen(fd, "a");
        if (m_file) {
            now_last_rotate_time = Timestamp::Now();
            m_bytes_since_cache_flush = m_file_length = 0;

            return true;
        }

        ::close(fd);
        return false;
    }

    LogFileObject& GetLogFileObject()
    {
        static std::mutex m_mutex;
        static std::unique_ptr<LogFileObject> g_LogFileObjectPtr;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (g_LogFileObjectPtr == NULL) {
            g_LogFileObjectPtr.reset(new LogFileObject(FLAG_SEVERITY, FLAG_BASE_FILENAME));
        }

        return *g_LogFileObjectPtr;
    }

    struct LogMessage::LogMessageData
    {
        LogMessageData(LogSeverity severity)
            : m_stream(m_messageText, kMaxLogMessageLen),
            m_severity(severity),
            m_timestamp(Timestamp::Now())
        { }

        ~LogMessageData() { if (m_severity == FATAL) abort(); }

        inline char*  data() { return m_stream.pbase(); }
        inline size_t size() { return m_stream.pcount(); }

        char m_messageText[kMaxLogMessageLen + 1];

        LogStream   m_stream;
        LogSeverity m_severity;
        Timestamp   m_timestamp;
    };

    LogMessage::LogMessage(LogSeverity severity, const char* file, int line)
        : m_data(new LogMessageData(severity))
    {
        assert(severity >= FATAL && severity <= ALL);

        static std::string levelMaps[] = {
            "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", "ALL" };

        Stream() << std::left << std::setw(5) << levelMaps[severity]
                 << ' '
                 << m_data->m_timestamp.ToFormattedString(false)
                 << ' '
                 << std::setw(-7) << CurrentThread::threadId() << std::setfill('0')
                 << ' '
                 << file << ':' << line << ' ';
    }

    LogMessage::~LogMessage()
    {
        m_data->m_messageText[m_data->size()] = '\n';

        GetLogFileObject().Write(true, m_data->m_timestamp, m_data->m_messageText, m_data->size() + 1);

        delete m_data;
    }

    std::ostream& LogMessage::Stream() { return m_data->m_stream; }
}