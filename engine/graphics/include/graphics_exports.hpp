
#ifndef GRAPHICS_API_H
#define GRAPHICS_API_H

#ifdef ASH_GRAPHICS_STATIC_DEFINE
#  define GRAPHICS_API
#  define ASH_GRAPHICS_NO_EXPORT
#else
#  ifndef GRAPHICS_API
#    ifdef ash_graphics_EXPORTS
        /* We are building this library */
#      define GRAPHICS_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define GRAPHICS_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_GRAPHICS_NO_EXPORT
#    define ASH_GRAPHICS_NO_EXPORT 
#  endif
#endif

#ifndef ASH_GRAPHICS_DEPRECATED
#  define ASH_GRAPHICS_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_GRAPHICS_DEPRECATED_EXPORT
#  define ASH_GRAPHICS_DEPRECATED_EXPORT GRAPHICS_API ASH_GRAPHICS_DEPRECATED
#endif

#ifndef ASH_GRAPHICS_DEPRECATED_NO_EXPORT
#  define ASH_GRAPHICS_DEPRECATED_NO_EXPORT ASH_GRAPHICS_NO_EXPORT ASH_GRAPHICS_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_GRAPHICS_NO_DEPRECATED
#    define ASH_GRAPHICS_NO_DEPRECATED
#  endif
#endif

#endif /* GRAPHICS_API_H */
