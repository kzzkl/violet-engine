
#ifndef TASK_API_H
#define TASK_API_H

#ifdef ASH_TASK_STATIC_DEFINE
#  define TASK_API
#  define ASH_TASK_NO_EXPORT
#else
#  ifndef TASK_API
#    ifdef ash_task_EXPORTS
        /* We are building this library */
#      define TASK_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define TASK_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_TASK_NO_EXPORT
#    define ASH_TASK_NO_EXPORT 
#  endif
#endif

#ifndef ASH_TASK_DEPRECATED
#  define ASH_TASK_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_TASK_DEPRECATED_EXPORT
#  define ASH_TASK_DEPRECATED_EXPORT TASK_API ASH_TASK_DEPRECATED
#endif

#ifndef ASH_TASK_DEPRECATED_NO_EXPORT
#  define ASH_TASK_DEPRECATED_NO_EXPORT ASH_TASK_NO_EXPORT ASH_TASK_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_TASK_NO_DEPRECATED
#    define ASH_TASK_NO_DEPRECATED
#  endif
#endif

#endif /* TASK_API_H */
