#include "Timestamp.h"

#include <sys/time.h>

using namespace buzz;

std::string Timestamp::ToString() const
{
    char buf[128] = { 0 };

    int64_t   sec = m_micro_seconds_since_epoch / kMicroSecondsPerSecond;
    int64_t u_sec = m_micro_seconds_since_epoch % kMicroSecondsPerSecond;

    int n = snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64, sec, u_sec);

    return buf;
}

std::string Timestamp::ToFormattedString(bool showMicroSecond) const
{
    return ToFormattedString(NULL, showMicroSecond);
}

std::string Timestamp::ToFormattedString(const char* fmt, bool showMicroSecond) const
{
    char buf[128] = { 0 };

    struct tm tm_time;
    time_t seconds = static_cast<time_t>(SecondsSinceEpoch());

    localtime_r(&seconds, &tm_time);

    int n = strftime(buf, sizeof(buf), fmt ? fmt : "%F %T", &tm_time);
    if (showMicroSecond) {
        int64_t u_sec = m_micro_seconds_since_epoch % kMicroSecondsPerSecond;
        snprintf(buf + n, sizeof(buf) - n, ".%06" PRId64, u_sec);
    }

    return buf;
}

Timestamp Timestamp::Now()
{
    struct ::timeval tv;
    ::gettimeofday(&tv, NULL);

    return Timestamp(tv.tv_sec * kMicroSecondsPerSecond + tv.tv_usec);
}

Timestamp Timestamp::Invalid()
{
    return Timestamp();
}