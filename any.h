#pragma once

/* Reference from boost::any (http://www.boost.org/doc/libs/1_50_0/boost/any.hpp) */

#include <memory>
#include <typeinfo>
#include <type_traits>

#include <assert.h>

namespace buzz
{
    class any
    {
    public:
        any() : m_content(nullptr)
        { }

        template<typename T>
        any(const T& value) : m_content(new holder<T>(std::move(value)))
        { }

        any(const any& other) : m_content(other.m_content ? other.m_content->clone() : nullptr)
        { }

    public:
        any& swap(any& rhs)
        {
            std::swap(m_content, rhs.m_content);
            return *this;
        }

        any& operator=(any& rhs) { return rhs.swap(*this); }

        template<typename T>
        any& operator=(const T& rhs) { return any(rhs).swap(*this); }

    public:
        bool empty() const { return m_content != nullptr; }
        const std::type_info& type() { return m_content ? m_content->type() : typeid(void); }

    private:
        class placeholder;
        typedef std::unique_ptr<placeholder> placeholder_ptr;

        class placeholder
        {
        public:
            virtual ~placeholder() { }
        public:
            virtual placeholder_ptr clone() const = 0;
            virtual const std::type_info& type() const = 0;
        };

        template<typename T>
        class holder : public placeholder
        {
        public:
            holder(const T& value) : m_held(value)
            { }

        public:
            virtual const std::type_info& type() const { return typeid(T); }
            virtual placeholder_ptr clone() const { return placeholder_ptr(new holder<T>(m_held)); }

            T m_held;
        };

        placeholder_ptr m_content;

        template<typename T>
        friend T* any_cast(any*) noexcept;
    };

    class bad_any_cast : public std::bad_cast
    {
    public:
        virtual const char * what() const throw()
        {
            return "buzz::bad_any_cast: failed conversion using buzz::any_cast";
        }
    };

    template<typename T>
    T* any_cast(any* operand) noexcept
    {
        if (operand && typeid(T) == operand->type()) {
            auto t = operand->m_content.get();

            return &static_cast<any::holder<T> *>(t)->m_held;
        }

        return nullptr;
    }

    template<typename T>
    inline T* any_cast(const any* operand) noexcept
    {
        return any_cast<T>(const_cast<any*>(operand));
    }

    template<class T>
    T any_cast(any& operand)
    {
        using noref_t = typename std::remove_reference<T>::type;
        assert(std::is_reference<noref_t>::value == false);

        noref_t* result = any_cast<noref_t>(&operand);
        if (result) {
            return *result;
        }

        throw bad_any_cast();
    }

    template<class T>
    T any_cast(const any& operand)
    {
        using noref_t = typename std::remove_reference<T>::type;
        assert(std::is_reference<noref_t>::value == false);

        return any_cast<noref_t &>(const_cast<any&>(operand));
    }
}