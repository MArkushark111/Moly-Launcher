#pragma once
#include <QString>
namespace moly {
void registerMolyPackFiles(const QString &executable);
void setStartWithWindows(const QString &executable, bool enabled);
}
