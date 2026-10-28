#pragma once
#include "core.h"
#include <QNetworkAccessManager>
#include <QUrl>
namespace moly {
struct HttpError : Error {
    int status; bool invalidRegistration;
    HttpError(const QString &message,int status,bool invalidRegistration=false):Error(message),status(status),invalidRegistration(invalidRegistration){}
};
struct Download { QString url,path,hash; QString algorithm="sha1"; qint64 size=-1; qint64 maximumSize=2ll*1024*1024*1024; };
class Network {
public:
    explicit Network(Context context = {}) : ctx(std::move(context)) {}
    QByteArray request(const QString &url,const QByteArray &body={},const QByteArray &type={},const QMap<QByteArray,QByteArray> &headers={},bool allowHttpError=false);
    QJsonDocument json(const QString &url,const QJsonObject &body={},const QMap<QByteArray,QByteArray> &headers={});
    QJsonObject form(const QString &url,const QMap<QString,QString> &fields,bool allowError=false);
    void downloads(const QList<Download> &items);
    Context ctx;
private:
    QNetworkAccessManager manager;
};
}
