#pragma once

#include <string>
#include <inttypes.h>

namespace buzz
{
    class Timestamp
    {
    public:
        Timestamp() : m_micro_seconds_since_epoch(0)
        { }

        explicit Timestamp(int64_t microseconds)
            : m_micro_seconds_since_epoch(microseconds)
        { }

        inline int64_t SecondsSinceEpoch() const
        {
            return m_micro_seconds_since_epoch / kMicroSecondsPerSecond;
        }

        inline int64_t MicroSecondsSinceEpoch() const
        {
            return m_micro_seconds_since_epoch;
        }

        inline bool Valid() const { return  m_micro_seconds_since_epoch > 0; }

        std::string ToString() const;

        std::string ToFormattedString(bool showMicroSecond) const;
        std::string ToFormattedString(const char* fmt = NULL, bool showMicroSecond = true) const;

        static Timestamp Now();
        static Timestamp Invalid();

        static const int kMicroSecondsPerSecond = 1000 * 1000;

    private:
        int64_t m_micro_seconds_since_epoch;
    };

    inline bool operator==(const Timestamp& lhs, const Timestamp& rhs)
    {
        return lhs.MicroSecondsSinceEpoch() == rhs.MicroSecondsSinceEpoch();
    }

    inline bool operator<(const Timestamp& lhs, const Timestamp& rhs)
    {
        return lhs.MicroSecondsSinceEpoch() < rhs.MicroSecondsSinceEpoch();
    }

    inline bool operator<=(const Timestamp& lhs, const Timestamp& rhs)
    {
        return lhs.MicroSecondsSinceEpoch() <= rhs.MicroSecondsSinceEpoch();
    }

    inline bool operator>(const Timestamp& lhs, const Timestamp& rhs)
    {
        return lhs.MicroSecondsSinceEpoch() > rhs.MicroSecondsSinceEpoch();
    }

    inline bool operator>=(const Timestamp& lhs, const Timestamp& rhs)
    {
        return lhs.MicroSecondsSinceEpoch() >= rhs.MicroSecondsSinceEpoch();
    }

    inline double TimeDifference(Timestamp high, Timestamp low)
    {
        int64_t diff = high.MicroSecondsSinceEpoch() - low.MicroSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }

    inline Timestamp AddTime(Timestamp timestamp, double seconds)
    {
        int64_t t = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timestamp.MicroSecondsSinceEpoch() + t);
    }
}
