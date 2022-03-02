
#ifndef ECS_API_H
#define ECS_API_H

#ifdef ASH_ECS_STATIC_DEFINE
#  define ECS_API
#  define ASH_ECS_NO_EXPORT
#else
#  ifndef ECS_API
#    ifdef ash_ecs_EXPORTS
        /* We are building this library */
#      define ECS_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define ECS_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_ECS_NO_EXPORT
#    define ASH_ECS_NO_EXPORT 
#  endif
#endif

#ifndef ASH_ECS_DEPRECATED
#  define ASH_ECS_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_ECS_DEPRECATED_EXPORT
#  define ASH_ECS_DEPRECATED_EXPORT ECS_API ASH_ECS_DEPRECATED
#endif

#ifndef ASH_ECS_DEPRECATED_NO_EXPORT
#  define ASH_ECS_DEPRECATED_NO_EXPORT ASH_ECS_NO_EXPORT ASH_ECS_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_ECS_NO_DEPRECATED
#    define ASH_ECS_NO_DEPRECATED
#  endif
#endif

#endif /* ECS_API_H */
