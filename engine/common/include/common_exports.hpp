
#ifndef COMMON_API_H
#define COMMON_API_H

#ifdef ASH_COMMON_STATIC_DEFINE
#  define COMMON_API
#  define ASH_COMMON_NO_EXPORT
#else
#  ifndef COMMON_API
#    ifdef ash_common_EXPORTS
        /* We are building this library */
#      define COMMON_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define COMMON_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_COMMON_NO_EXPORT
#    define ASH_COMMON_NO_EXPORT 
#  endif
#endif

#ifndef ASH_COMMON_DEPRECATED
#  define ASH_COMMON_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_COMMON_DEPRECATED_EXPORT
#  define ASH_COMMON_DEPRECATED_EXPORT COMMON_API ASH_COMMON_DEPRECATED
#endif

#ifndef ASH_COMMON_DEPRECATED_NO_EXPORT
#  define ASH_COMMON_DEPRECATED_NO_EXPORT ASH_COMMON_NO_EXPORT ASH_COMMON_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_COMMON_NO_DEPRECATED
#    define ASH_COMMON_NO_DEPRECATED
#  endif
#endif

#endif /* COMMON_API_H */
