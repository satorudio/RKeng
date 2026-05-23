#pragma once
// RKExport.h — минимальный хедер только с макросом RK_API.
// Включается в src/ заголовки вместо тяжёлого IScenePlugin.h.

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(RK_ENGINE_BUILD) || defined(RK_SCENE_BUILD)
#    ifdef __GNUC__
#      define RK_API __attribute__((dllexport))
#    else
#      define RK_API __declspec(dllexport)
#    endif
#  else
#    ifdef __GNUC__
#      define RK_API __attribute__((dllimport))
#    else
#      define RK_API __declspec(dllimport)
#    endif
#  endif
#else
#  define RK_API __attribute__((visibility("default")))
#endif
