//
// Copyright 2008 (C) Moriyoshi Koizumi. All rights reserved.
//
// This software is distributed under the Boost Software License, Version 1.0.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef BOOST_PHP_ERROR_HPP
#define BOOST_PHP_ERROR_HPP

#include <cstdarg>
#include <string>
#include <boost/static_assert.hpp>
#include <zend.h>
#include <main/php.h>

namespace boost { namespace php {
class error_info {
public:
    error_info(): type_(-1), filename_(), line_number_(0), message_() {}

    error_info(int type, const char* filename, const ::uint lineno,
            const ::std::string& message)
        : type_(type), filename_(filename), line_number_(lineno),
          message_(message) {}

    error_info(int type, const char* filename, const ::uint lineno,
            const char* format, va_list ap)
        : type_(type), filename_(filename), line_number_(lineno),
          message_(format_message(format, ap)) {}

    int type() const {
        return type_;
    }

    bool valid() const {
        return type_ >= 0;
    }

    const ::std::string& filename() const {
        return filename_;
    }

    const ::uint line_number() const {
        return line_number_;
    }

    const ::std::string& message() const {
        return message_;
    }

    error_info& operator=(const error_info& rhs) {
        type_ = rhs.type_;
        filename_ = rhs.filename_;
        line_number_ = rhs.line_number_;
        message_ = rhs.message_;
        return *this;
    }

private:
    static const ::std::string format_message(const char* format, va_list ap) {
        ::std::string retval;
        ::std::string::size_type s;
        BOOST_STATIC_ASSERT(sizeof(s) >= sizeof(int));
        retval.reserve(16);
        for (;;) {
            // XXX: how come it returns size_type by int?
            s = static_cast< ::std::string::size_type>(
                vsnprintf(&retval[0], retval.capacity(), format, ap));
            if (s < retval.capacity()) {
                break;
            }
            retval.reserve(s + 1);
            retval.resize(s);
        }
        return retval;
    }

protected:
    int type_;
    ::std::string filename_;
    ::uint line_number_;
    ::std::string message_;
};

namespace detail {
    class error_captor;
    static error_captor* current_error_captor;

    class error_captor {
    private:
        typedef void (*handler_type)(int, const char*, const ::uint,
                const char*, va_list);
    public:
        error_captor()
            : prev_(current_error_captor), old_handler_(::zend_error_cb) {

            current_error_captor = this;
            ::zend_error_cb = &error_captor::capture_handler;
        }

        ~error_captor() {
            ::zend_error_cb = old_handler_;
            current_error_captor = prev_;
        }

        const error_info& captured() {
            return captured_;
        }

        static void capture_handler(int type, const char* filename,
                const ::uint lineno, const char* format, va_list ap)
        {
            current_error_captor->captured_ = error_info(
                    type, filename, lineno, format, ap);
        }

    private:
        error_captor* prev_;
        ::boost::php::error_info captured_;
        handler_type old_handler_;
    };
} // namespace detail

} } // namespace boost::php

#define BOOST_PHP_BEGIN_CAPTURE_ERROR \
    { \
        ::boost::php::detail::error_captor __mozo_php_error_cap;

#define BOOST_PHP_END_CAPTURE_ERROR \
    }

#define BOOST_PHP_LAST_ERROR __mozo_php_error_cap.captured()

#undef slprintf
#undef vslprintf
#undef snprintf
#undef vsnprintf
#undef sprintf

#endif /* BOOST_PHP_ERROR_HPP */
