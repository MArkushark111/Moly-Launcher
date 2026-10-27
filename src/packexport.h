#pragma once
#include "core.h"

namespace moly {
class PackExport {
  public:
    // Formats: mlypack (Moly portable instance), zip (same portable layout), mrpack.
    // Export never includes launcher accounts, machine-specific Java paths, or game binaries.
    static void write(const QString &root, const QJsonObject &instance, const QString &destination,
                      const QString &format, bool includeWorlds = false, Context context = {});
    // Accepts .mlypack and Moly portable ZIPs containing moly-pack.json. A fresh instance id
    // is assigned and the complete instance is committed by directory rename.
    static QJsonObject importMoly(const QString &root, const QString &archive, Context context = {});
};
} // namespace moly
