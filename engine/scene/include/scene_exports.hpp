
#ifndef SCENE_API_H
#define SCENE_API_H

#ifdef ASH_SCENE_STATIC_DEFINE
#  define SCENE_API
#  define ASH_SCENE_NO_EXPORT
#else
#  ifndef SCENE_API
#    ifdef ash_scene_EXPORTS
        /* We are building this library */
#      define SCENE_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define SCENE_API __declspec(dllimport)
#    endif
#  endif

#  ifndef ASH_SCENE_NO_EXPORT
#    define ASH_SCENE_NO_EXPORT 
#  endif
#endif

#ifndef ASH_SCENE_DEPRECATED
#  define ASH_SCENE_DEPRECATED __declspec(deprecated)
#endif

#ifndef ASH_SCENE_DEPRECATED_EXPORT
#  define ASH_SCENE_DEPRECATED_EXPORT SCENE_API ASH_SCENE_DEPRECATED
#endif

#ifndef ASH_SCENE_DEPRECATED_NO_EXPORT
#  define ASH_SCENE_DEPRECATED_NO_EXPORT ASH_SCENE_NO_EXPORT ASH_SCENE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ASH_SCENE_NO_DEPRECATED
#    define ASH_SCENE_NO_DEPRECATED
#  endif
#endif

#endif /* SCENE_API_H */
