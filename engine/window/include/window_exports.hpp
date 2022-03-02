
#ifndef WINDOW_API_H
#define WINDOW_API_H

#ifdef ASH_WINDOW_STATIC_DEFINE
#  define WINDOW_API
#  define ASH_WINDOW_NO_EXPORT
#else
#  ifndef WINDOW_API
#    ifdef ash_window_EXPORTS
        /* We are building this library */
#      define WINDOW_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define WINDOW_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_WINDOW_NO_EXPORT
#    define ASH_WINDOW_NO_EXPORT 
#  endif
#endif

#ifndef ASH_WINDOW_DEPRECATED
#  define ASH_WINDOW_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_WINDOW_DEPRECATED_EXPORT
#  define ASH_WINDOW_DEPRECATED_EXPORT WINDOW_API ASH_WINDOW_DEPRECATED
#endif

#ifndef ASH_WINDOW_DEPRECATED_NO_EXPORT
#  define ASH_WINDOW_DEPRECATED_NO_EXPORT ASH_WINDOW_NO_EXPORT ASH_WINDOW_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_WINDOW_NO_DEPRECATED
#    define ASH_WINDOW_NO_DEPRECATED
#  endif
#endif

#endif /* WINDOW_API_H */
