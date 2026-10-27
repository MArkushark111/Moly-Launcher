#pragma once
#include "network.h"
namespace moly {
class Secrets {
public:
    static void put(const QString &key,const QByteArray &value);
    static QByteArray get(const QString &key);
    static void remove(const QString &key);
};
struct LoginResult { QJsonObject account; QString minecraftToken; };
class Accounts {
public:
    explicit Accounts(Context context = {}) : net(context) {}
    LoginResult signIn(const QString &clientId,const std::function<void(QString,QString)> &codeReady);
    LoginResult refresh(const QString &clientId,const QJsonObject &account);
private:
    Network net;
    LoginResult exchange(const QJsonObject &oauth);
};
}
