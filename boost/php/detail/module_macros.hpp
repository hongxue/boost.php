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

#ifndef BOOST_PHP_MODULE_MACROS_HPP
#define BOOST_PHP_MODULE_MACROS_HPP

#include <boost/php/detail/object_retriever.hpp>
#include <boost/php/detail/module_hooks.hpp>

#if ZEND_MODULE_API_NO < 20090115
#define BOOST_PHP_MODULE_BUILD_ID
#else
#define BOOST_PHP_MODULE_BUILD_ID , ZEND_MODULE_BUILD_ID
#endif

#define BOOST_PHP_ADD_MODULE_VARIATION(__mod_name__) \
    namespace boost { namespace php { namespace _ { \
        struct __mod_name__ {}; \
    } } }

#define BOOST_PHP_ASSOCIATE_MODULE_WITH_CLASS(__mod_name__, __mod_klass__) \
    namespace boost { namespace php { namespace detail { \
    template<> struct module_class_of< ::boost::php::_::__mod_name__> { \
        typedef __mod_klass__ type; \
    }; \
    } } }

#define BOOST_PHP_MODULE_CLASS(__mod_name__) \
    ::boost::php::detail::module_class_of< ::boost::php::_::__mod_name__>::type
#define BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__) \
    ::boost::php::detail::module_class_of< ::boost::php::_::__mod_name__>::type::handler
#define BOOST_PHP_MODULE_HANDLER(__mod_name__) \
    BOOST_PP_CAT(__mod_name__, _globals)
#define BOOST_PHP_MODULE_VAR(__mod_name__) \
    BOOST_PP_CAT(__mod_name__, _module)
#define BOOST_PHP_MODULE_ENTRY_VAR(__mod_name__) \
    BOOST_PP_CAT(__mod_name__, _module_entry)
#define BOOST_PHP_TSRM_ID_VAR(__mod_name__) \
    BOOST_PP_CAT(BOOST_PHP_MODULE_HANDLER(__mod_name__), _id)
#define BOOST_PHP_MODULE_HANDLER_CTOR(__mod_name__) \
    BOOST_PP_CAT(BOOST_PHP_MODULE_HANDLER(__mod_name__), _ctor)
#define BOOST_PHP_MODULE_HANDLER_DTOR(__mod_name__) \
    BOOST_PP_CAT(BOOST_PHP_MODULE_HANDLER(__mod_name__), _dtor)
#define BOOST_PHP_MODULE_POST_RSHUTDOWN_FUNC(__mod_name__) \
    BOOST_PP_CAT(zm_post_deactivate_, __mod_name__)

#define BOOST_PHP_USE_MODULE_HANDLER_CTOR_AND_DTOR(__mod_name__) \
extern "C" { \
    static void BOOST_PHP_MODULE_HANDLER_CTOR(__mod_name__)(BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)* ptr TSRMLS_DC); \
    static void BOOST_PHP_MODULE_HANDLER_DTOR(__mod_name__)(BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)* ptr TSRMLS_DC); \
}

#ifdef ZTS

#define BOOST_PHP_MODULE_HEADER(__mod_name__, __version__) \
    { \
        STANDARD_MODULE_HEADER, \
        const_cast<char*>(#__mod_name__), \
        0, \
        ZEND_MINIT(__mod_name__), \
        ZEND_MSHUTDOWN(__mod_name__), \
        ZEND_RINIT(__mod_name__), \
        ZEND_RSHUTDOWN(__mod_name__), \
        ZEND_MINFO(__mod_name__), \
        const_cast<char*>(__version__), \
        sizeof(BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)), \
        &BOOST_PHP_TSRM_ID_VAR(__mod_name__), \
        NULL, \
        NULL, \
        &BOOST_PHP_MODULE_POST_RSHUTDOWN_FUNC(__mod_name__), \
        0, 0, NULL, 0 BOOST_PHP_MODULE_BUILD_ID \
    }

#define BOOST_PHP_DECLARE_MODULE_SLOT(__mod_name__) \
    extern "C" { ZEND_API ts_rsrc_id BOOST_PHP_TSRM_ID_VAR(__mod_name__); }

#define BOOST_PHP_USE_MODULE_SLOT_CONFIG(__mod_name__) \
    extern "C" { extern ZEND_API ts_rsrc_id BOOST_PHP_TSRM_ID_VAR(__mod_name__); }

#define BOOST_PHP_MODULE_SLOT(__mod_name__) \
    reinterpret_cast<BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)*>((*reinterpret_cast<void***>(tsrm_ls))[TSRM_UNSHUFFLE_RSRC_ID(BOOST_PHP_TSRM_ID_VAR(__mod_name__))])

#define BOOST_PHP_DECLARE_MINIT_FUNCTION(__mod_name__) \
    ZEND_MINIT_FUNCTION(__mod_name__) { \
        ts_allocate_id(&BOOST_PHP_TSRM_ID_VAR(__mod_name__), \
                sizeof(BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)), \
                (ts_allocate_ctor)&BOOST_PHP_MODULE_HANDLER_CTOR(__mod_name__),\
                (ts_allocate_dtor)&BOOST_PHP_MODULE_HANDLER_DTOR(__mod_name__));\
        try { \
            BOOST_PHP_MODULE_INVOKE_HOOKS(__mod_name__, initializers); \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__initialize(TSRMLS_C); \
        } catch (const ::std::exception& e) { \
            zend_error(E_WARNING, const_cast<char*>("%s"), e.what()); \
            return FAILURE; \
        } \
        return SUCCESS; \
    }

#define BOOST_PHP_DECLARE_MSHUTDOWN_FUNCTION(__mod_name__) \
    ZEND_MSHUTDOWN_FUNCTION(__mod_name__) { \
        ts_free_id(BOOST_PHP_TSRM_ID_VAR(__mod_name__)); \
        try { \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__finalize(TSRMLS_C); \
            BOOST_PHP_MODULE_INVOKE_HOOKS(__mod_name__, finalizers); \
        } catch (const ::std::exception& e) { \
            zend_error(E_WARNING, const_cast<char*>("%s"), e.what()); \
        } \
        return SUCCESS; \
    }

#else /* ZTS */

#define BOOST_PHP_MODULE_HEADER(__mod_name__, __version__) \
    { \
        STANDARD_MODULE_HEADER, \
        const_cast<char*>(#__mod_name__), \
        0, \
        ZEND_MINIT(__mod_name__), \
        ZEND_MSHUTDOWN(__mod_name__), \
        ZEND_RINIT(__mod_name__), \
        ZEND_RSHUTDOWN(__mod_name__), \
        ZEND_MINFO(__mod_name__), \
        const_cast<char*>(__version__), \
        sizeof(BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)), \
        &BOOST_PHP_MODULE_HANDLER(__mod_name__), \
        NULL, \
        NULL, \
        &BOOST_PHP_MODULE_POST_RSHUTDOWN_FUNC(__mod_name__), \
        0, 0, NULL, 0 BOOST_PHP_MODULE_BUILD_ID \
    }

#define BOOST_PHP_DECLARE_MODULE_SLOT(__mod_name__) \
    extern "C" { ZEND_API BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__) BOOST_PHP_MODULE_HANDLER(__mod_name__)( \
        BOOST_PHP_MODULE_VAR(&__mod_name__)); }

#define BOOST_PHP_USE_MODULE_SLOT_CONFIG(__mod_name__) \
    extern "C" { extern ZEND_API BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__) BOOST_PHP_MODULE_HANDLER(__mod_name__); }

#define BOOST_PHP_MODULE_SLOT(__mod_name__) \
    (&BOOST_PHP_MODULE_HANDLER(__mod_name__))

#define BOOST_PHP_DECLARE_MINIT_FUNCTION(__mod_name__) \
    ZEND_MINIT_FUNCTION(__mod_name__) { \
        BOOST_PHP_MODULE_HANDLER_CTOR(__mod_name__)(BOOST_PHP_MODULE_SLOT(__mod_name__)); \
        try { \
            BOOST_PHP_MODULE_INVOKE_HOOKS(__mod_name__, initializers); \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__initialize(); \
        } catch (const ::std::exception& e) { \
            zend_error(E_WARNING, const_cast<char*>("%s"), e.what()); \
            return FAILURE; \
        } \
        return SUCCESS; \
    }

#define BOOST_PHP_DECLARE_MSHUTDOWN_FUNCTION(__mod_name__) \
    ZEND_MSHUTDOWN_FUNCTION(__mod_name__) { \
        try { \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__finalize(); \
            BOOST_PHP_MODULE_INVOKE_HOOKS(__mod_name__, finalizers); \
            BOOST_PHP_MODULE_HANDLER_DTOR(__mod_name__)(BOOST_PHP_MODULE_SLOT(__mod_name__)); \
        } catch (const ::std::exception& e) { \
            zend_error(E_WARNING, const_cast<char*>("%s"), e.what()); \
        } \
        return SUCCESS; \
    }

#endif /* ZTS */

#define BOOST_PHP_DECLARE_RINIT_FUNCTION(__mod_name__) \
    ZEND_RINIT_FUNCTION(__mod_name__) { \
        try { \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__activate(TSRMLS_C); \
        } catch (const ::std::exception& e) { \
            zend_error(E_WARNING, const_cast<char*>("%s"), e.what()); \
            return FAILURE; \
        } \
        return SUCCESS; \
    }

#define BOOST_PHP_DECLARE_RSHUTDOWN_FUNCTION(__mod_name__) \
    ZEND_RSHUTDOWN_FUNCTION(__mod_name__) { \
        try { \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__deactivate(TSRMLS_C); \
        } catch (const ::std::exception& e) { \
            zend_error(E_WARNING, const_cast<char*>("%s"), e.what()); \
            return FAILURE; \
        } \
        return SUCCESS; \
    }

#define BOOST_PHP_DECLARE_MINFO_FUNCTION(__mod_name__) \
    ZEND_MINFO_FUNCTION(__mod_name__) { \
        try { \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__display_info(TSRMLS_C); \
        } catch (const ::std::exception& e) { \
            zend_error(E_ERROR, const_cast<char*>("%s"), e.what()); \
            _zend_bailout(const_cast<char*>(__FILE__), __LINE__); \
        } \
    }

#define BOOST_PHP_DECLARE_POST_RSHUTDOWN_FUNCTION(__mod_name__) \
    int BOOST_PHP_MODULE_POST_RSHUTDOWN_FUNC(__mod_name__)() { \
        TSRMLS_FETCH(); \
        try { \
            BOOST_PHP_MODULE_SLOT(__mod_name__)->__post_deactivate(TSRMLS_C); \
        } catch (const ::std::exception& e) { \
            zend_error(E_WARNING, const_cast<char*>("%s"), e.what()); \
            return FAILURE; \
        } \
        return SUCCESS; \
    }

#define BOOST_PHP_MODULE_INVOKE_HOOKS(__mod_name__, __kind__) \
    BOOST_PHP_MODULE_EACH_HOOK( \
            ::boost::php::detail::module_hooks< \
                BOOST_PHP_MODULE_CLASS(__mod_name__) >::__kind__##_type, \
            BOOST_PHP_MODULE_HOOKS(__mod_name__)::singleton.__kind__, \
            (*_)(*BOOST_PHP_MODULE_SLOT(__mod_name__) TSRMLS_CC))

#define BOOST_PHP_MODULE_EACH_HOOK(__type__, __list__, __action__) \
    for (__type__::element_type* _ = __list__.first; _; _ = _->next) { __action__; }

#define BOOST_PHP_MODULE_HOOKS(__mod_name__) \
    ::boost::php::detail::module_hooks< BOOST_PHP_MODULE_CLASS(__mod_name__) >

#define BOOST_PHP_DECLARE_MODULE_HOOKS(__mod_name__) \
    namespace boost { namespace php { namespace detail { \
    template<> \
    module_hooks< BOOST_PHP_MODULE_CLASS(__mod_name__) > \
    module_hooks< BOOST_PHP_MODULE_CLASS(__mod_name__) >::singleton = module_hooks< BOOST_PHP_MODULE_CLASS(__mod_name__) >(); \
    } } }

#define BOOST_PHP_USE_MODULE_SLOT(__mod_name__, __mod_klass__) \
    BOOST_PHP_ASSOCIATE_MODULE_WITH_CLASS(__mod_name__, __mod_klass__) \
    BOOST_PHP_USE_MODULE_SLOT_CONFIG(__mod_name__) \

#define BOOST_PHP_USE_INIT_FUNCTIONS(__mod_name__) \
    extern "C" { \
        static ZEND_MINIT_FUNCTION(__mod_name__); \
        static ZEND_MSHUTDOWN_FUNCTION(__mod_name__); \
        static ZEND_MINFO_FUNCTION(__mod_name__); \
        static ZEND_RINIT_FUNCTION(__mod_name__); \
        static ZEND_RSHUTDOWN_FUNCTION(__mod_name__); \
        static int BOOST_PHP_MODULE_POST_RSHUTDOWN_FUNC(__mod_name__)(); \
    }

#define BOOST_PHP_DECLARE_INIT_FUNCTIONS(__mod_name__, __mod_klass__) \
    extern "C" { \
        static void \
        BOOST_PHP_MODULE_HANDLER_CTOR(__mod_name__)(BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)* ptr TSRMLS_DC) \
        { \
            new(ptr) BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)(&BOOST_PHP_MODULE_VAR(__mod_name__)); \
        } \
        static void \
        BOOST_PHP_MODULE_HANDLER_DTOR(__mod_name__)(BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)* ptr TSRMLS_DC) \
        { \
            ptr->~handler(); \
        } \
        BOOST_PHP_DECLARE_MINIT_FUNCTION(__mod_name__); \
        BOOST_PHP_DECLARE_MSHUTDOWN_FUNCTION(__mod_name__); \
        BOOST_PHP_DECLARE_MINFO_FUNCTION(__mod_name__); \
        BOOST_PHP_DECLARE_RINIT_FUNCTION(__mod_name__); \
        BOOST_PHP_DECLARE_RSHUTDOWN_FUNCTION(__mod_name__); \
        BOOST_PHP_DECLARE_POST_RSHUTDOWN_FUNCTION(__mod_name__); \
    } \
    BOOST_PHP_DECLARE_MODULE_HOOKS(__mod_name__);

#define BOOST_PHP_DECLARE_HANDLER_RETRIEVER(__mod_name__) \
    namespace boost { namespace php { \
        template<> \
        BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)* \
        object_retriever<BOOST_PHP_MODULE_HANDLER_CLASS(__mod_name__)>:: \
                operator()(INTERNAL_FUNCTION_PARAMETERS) const { \
            return BOOST_PHP_MODULE_SLOT(__mod_name__); \
        } \
    } }

#define BOOST_PHP_MODULE(__mod_name__, __version__, __mod_klass__) \
    BOOST_PHP_ADD_MODULE_VARIATION(__mod_name__); \
    BOOST_PHP_USE_MODULE_SLOT(__mod_name__, __mod_klass__); \
    BOOST_PHP_USE_INIT_FUNCTIONS(__mod_name__); \
    BOOST_PHP_USE_MODULE_HANDLER_CTOR_AND_DTOR(__mod_name__); \
    extern "C" { zend_module_entry BOOST_PHP_MODULE_ENTRY_VAR(__mod_name__) = \
            BOOST_PHP_MODULE_HEADER(__mod_name__, __version__); } \
    static __mod_klass__ BOOST_PHP_MODULE_VAR(__mod_name__)( \
            &BOOST_PHP_MODULE_ENTRY_VAR(__mod_name__)); \
    BOOST_PHP_DECLARE_MODULE_SLOT(__mod_name__); \
    BOOST_PHP_DECLARE_INIT_FUNCTIONS(__mod_name__, __mod_klass__); \
    BOOST_PHP_DECLARE_HANDLER_RETRIEVER(__mod_name__)

#endif /* BOOST_PHP_MODULE_MACROS_HPP */
