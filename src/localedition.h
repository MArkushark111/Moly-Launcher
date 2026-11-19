#pragma once
#include "core.h"
namespace moly {
constexpr bool localEdition() {
#ifdef MOLY_LOCAL_EDITION
    return true;
#else
    return false;
#endif
}
void seedLocalLibrary(const QString &source,const QString &destination,const Context &context={});
}
