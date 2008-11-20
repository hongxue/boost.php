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

#ifndef BOOST_PHP_MODULE_HPP
#define BOOST_PHP_MODULE_HPP

#include <new>
#include <cstddef>
#include <boost/preprocessor/cat.hpp>

#include <zend.h>
#include <zend_API.h>
#include <zend_modules.h>
#include <zend_ini.h>

#include <boost/php/detail/tsrm_macros.hpp>
#include <boost/php/detail/module_macros.hpp>

namespace boost { namespace php {

namespace detail {
    class module_dependency_base {
    public:
        module_dependency_base(const char* _name, int _type)
            : next_(0) {
            wrapped_.name = const_cast<char*>(_name);
            wrapped_.rel = NULL;
            wrapped_.version = 0;
            wrapped_.type = _type; 
        }

        ~module_dependency_base() {
            if (next_) {
                delete next_;
            }
        }

        module_dependency_base& operator<(const char* version) {
            wrapped_.rel = const_cast<char*>("lt");
            wrapped_.version = const_cast<char*>(version);
            return *this;
        }

        module_dependency_base& operator<=(const char* version) {
            wrapped_.rel = const_cast<char*>("le");
            wrapped_.version = const_cast<char*>(version);
            return *this;
        }

        module_dependency_base& operator==(const char* version) {
            wrapped_.rel = const_cast<char*>("eq");
            wrapped_.version = const_cast<char*>(version);
            return *this;
        }

        module_dependency_base& operator>=(const char* version) {
            wrapped_.rel = const_cast<char*>("ge");
            wrapped_.version = const_cast<char*>(version);
            return *this;
        }

        module_dependency_base& operator>(const char* version) {
            wrapped_.rel = const_cast<char*>("gt");
            wrapped_.version = const_cast<char*>(version);
            return *this;
        }

        module_dependency_base& operator &&(const module_dependency_base& rhs) {
            module_dependency_base* last = this;
            while (last->next_) {
                last = last->next_;
            }
            last->next_ = new module_dependency_base(rhs);
            return *this;
        }

        operator const ::zend_module_dep&() const {
            return wrapped_;
        }

        operator ::zend_module_dep&() {
            return wrapped_;
        }

        operator ::zend_module_dep*() {
            return realize();
        }

    protected:
        ::std::size_t count() {
            ::std::size_t retval = 0;
            for (module_dependency_base* current = this; current;
                    current = current->next_) {
                ++retval;
            }
            return retval;
        }

        ::zend_module_dep* realize() {
            ::std::size_t n = count();
            zend_module_dep* realized = new zend_module_dep[n + 1];
            module_dependency_base* current = this;
            for (::std::size_t i = 0; i < n; ++i, current = current->next_) {
                realized[i] = current->wrapped_;
            }
            realized[n].name = NULL;
            return realized;
        }

    private:
        ::zend_module_dep wrapped_;
        module_dependency_base* next_;
        ::zend_module_dep* realized_;
    };

    template<int _type>
    struct module_dependency
            : public module_dependency_base {
        module_dependency(const char* name)
            : module_dependency_base(name, _type) {}
    };

    static void cleanup_zend_module_entry(zend_module_entry* entry)
    {
        delete[] entry->ini_entry;
        delete[] entry->deps; 
    }

    template<typename T_>
    struct module_class_of {
        typedef void type;
    };
} // namespace detail

class module {
public:
    typedef detail::module_dependency<MODULE_DEP_REQUIRED> requires;
    typedef detail::module_dependency<MODULE_DEP_CONFLICTS> conflicts;
    typedef detail::module_dependency<MODULE_DEP_OPTIONAL> recommends;

    class handler {
    public:
        handler(module* mod) throw()
            : module_(mod) {}

        ~handler() throw() {}

        void __initialize(TSRMLS_D) {}

        void __finalize(TSRMLS_D) {}

        void __activate(TSRMLS_D) {}

        void __deactivate(TSRMLS_D) {}

        void __post_deactivate(TSRMLS_D) {}

        void __display_info(TSRMLS_D) {}
    protected:
        module const* const module_;
    };

protected:
    zend_module_entry* entry_;

public:
    module(zend_module_entry* entry): entry_(entry) {}

    ~module() {
        detail::cleanup_zend_module_entry(entry_);
    }
};

} } // namespace boost::php

#endif /* BOOST_PHP_MODULE_HPP */
