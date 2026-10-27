#pragma once
#include "core.h"

namespace moly {
// Read-only discovery. Candidates carry metadata, never authentication or launch commands.
class InstanceImport {
public:
    static QStringList providers();
    static QStringList defaultRoots(const QString &provider);
    static QJsonArray scan(const QString &provider, const QStringList &roots, const Context &context = {});
    static QJsonObject describe(const QString &provider, const QString &folder);
    static QJsonObject copy(const QString &storage, const QJsonObject &candidate,
                            const QString &id, bool includeWorlds, const Context &context = {});
};
}
