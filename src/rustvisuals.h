#pragma once
#include "core.h"
namespace moly {
class RustVisuals {
public:
    static QJsonObject read(const QString &game);
    static QJsonObject validate(const QJsonObject &values);
    static void apply(const QString &game, const QJsonObject &values, const Context &context = {});
};
}
