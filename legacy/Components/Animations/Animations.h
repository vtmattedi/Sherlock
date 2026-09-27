#pragma once
#include <Animations/AnimCore.h>

//--------------------------------------------//
// Comment any modules to not compile it
#define INCLUDE_SOLID
#define INCLUDE_DRAWERHANDLES
#define INCLUDE_RAINBOW
//-------------------------------------------//
#ifdef INCLUDE_SOLID
#include <Animations/Solid.h>
#endif

#ifdef INCLUDE_DRAWERHANDLES
#include <Animations/DrawerHandles.h>
#endif

#ifdef INCLUDE_RAINBOW
#include <Animations/Rainbow.h>
#endif
