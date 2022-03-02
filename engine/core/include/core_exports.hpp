
#ifndef CORE_API_H
#define CORE_API_H

#ifdef ASH_CORE_STATIC_DEFINE
#  define CORE_API
#  define ASH_CORE_NO_EXPORT
#else
#  ifndef CORE_API
#    ifdef ash_core_EXPORTS
        /* We are building this library */
#      define CORE_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define CORE_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_CORE_NO_EXPORT
#    define ASH_CORE_NO_EXPORT 
#  endif
#endif

#ifndef ASH_CORE_DEPRECATED
#  define ASH_CORE_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_CORE_DEPRECATED_EXPORT
#  define ASH_CORE_DEPRECATED_EXPORT CORE_API ASH_CORE_DEPRECATED
#endif

#ifndef ASH_CORE_DEPRECATED_NO_EXPORT
#  define ASH_CORE_DEPRECATED_NO_EXPORT ASH_CORE_NO_EXPORT ASH_CORE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_CORE_NO_DEPRECATED
#    define ASH_CORE_NO_DEPRECATED
#  endif
#endif

#endif /* CORE_API_H */
