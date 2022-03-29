
#ifndef PHYSICS_API_H
#define PHYSICS_API_H

#ifdef ASH_PHYSICS_STATIC_DEFINE
#  define PHYSICS_API
#  define ASH_PHYSICS_NO_EXPORT
#else
#  ifndef PHYSICS_API
#    ifdef ash_physics_EXPORTS
        /* We are building this library */
#      define PHYSICS_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define PHYSICS_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_PHYSICS_NO_EXPORT
#    define ASH_PHYSICS_NO_EXPORT 
#  endif
#endif

#ifndef ASH_PHYSICS_DEPRECATED
#  define ASH_PHYSICS_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_PHYSICS_DEPRECATED_EXPORT
#  define ASH_PHYSICS_DEPRECATED_EXPORT PHYSICS_API ASH_PHYSICS_DEPRECATED
#endif

#ifndef ASH_PHYSICS_DEPRECATED_NO_EXPORT
#  define ASH_PHYSICS_DEPRECATED_NO_EXPORT ASH_PHYSICS_NO_EXPORT ASH_PHYSICS_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_PHYSICS_NO_DEPRECATED
#    define ASH_PHYSICS_NO_DEPRECATED
#  endif
#endif

#endif /* PHYSICS_API_H */
