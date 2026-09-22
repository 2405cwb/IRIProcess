#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(XRIRICALCUMETHOD_LIB)
#  define XRIRICALCUMETHOD_EXPORT Q_DECL_EXPORT
# else
#  define XRIRICALCUMETHOD_EXPORT Q_DECL_IMPORT
# endif
#else
# define XRIRICALCUMETHOD_EXPORT
#endif
