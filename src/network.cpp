#include "network.h"
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QTimer>
#include <QUrlQuery>
#include <memory>

namespace moly {
static QNetworkRequest makeRequest(const QString &url) {
    QUrl u(url);
    if (u.scheme() != "https" || u.host().isEmpty() || !u.userInfo().isEmpty())
        throw Error("Only secure HTTPS downloads are supported.");
    QNetworkRequest r(u);
    r.setHeader(QNetworkRequest::UserAgentHeader, "MolyLauncher/0.4.0 (Windows)");
    r.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    r.setMaximumRedirectsAllowed(8);
    r.setTransferTimeout(60000);
    return r;
}
QByteArray Network::request(const QString &url, const QByteArray &body, const QByteArray &type,
                            const QMap<QByteArray, QByteArray> &headers, bool allowError) {
    ctx.check();
    auto r = makeRequest(url);
    if(headers.contains("Authorization"))r.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::SameOriginRedirectPolicy);
    for (auto i = headers.begin(); i != headers.end(); ++i)
        r.setRawHeader(i.key(), i.value());
    if (!type.isEmpty())
        r.setHeader(QNetworkRequest::ContentTypeHeader, type);
    QNetworkReply *reply = type.isEmpty() ? manager.get(r) : manager.post(r, body);
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(100);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
        if (ctx.cancel && ctx.cancel->load())
            reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    QByteArray bytes = reply->readAll();
    auto error = reply->error();
    auto message = reply->errorString();
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();
    ctx.check();
    if (error != QNetworkReply::NoError && !(allowError && code >= 400))
        throw Error(QString("Request failed (%1): %2 — %3").arg(code).arg(QUrl(url).host(), message));
    if (bytes.size() > 64 * 1024 * 1024)
        throw Error("Service response exceeds size limit.");
    return bytes;
}
QJsonDocument Network::json(const QString &url, const QJsonObject &body,
                            const QMap<QByteArray, QByteArray> &headers) {
    auto bytes =
        request(url, body.isEmpty() ? QByteArray() : QJsonDocument(body).toJson(QJsonDocument::Compact),
                body.isEmpty() ? QByteArray() : QByteArray("application/json"), headers);
    QJsonParseError e;
    auto d = QJsonDocument::fromJson(bytes, &e);
    if (e.error != QJsonParseError::NoError)
        throw Error("The service returned invalid JSON: " + QUrl(url).host());
    return d;
}
QJsonObject Network::form(const QString &url, const QMap<QString, QString> &fields, bool allowError) {
    QByteArray encoded;
    for (auto i = fields.begin(); i != fields.end(); ++i) {
        if (!encoded.isEmpty())
            encoded += '&';
        encoded += QUrl::toPercentEncoding(i.key()) + '=' + QUrl::toPercentEncoding(i.value());
    }
    auto data = request(url, encoded, "application/x-www-form-urlencoded", {}, allowError);
    auto o = QJsonDocument::fromJson(data).object();
    if (o.isEmpty())
        throw Error("The login service returned an invalid response.");
    return o;
}
void Network::downloads(const QList<Download> &items) {
    QList<Download> todo;
    QSet<QString> paths;
    for (const auto &download : items) {
        ctx.check();
        if (paths.contains(download.path))
            continue;
        paths.insert(download.path);
        if (QFileInfo::exists(download.path) &&
            QFileInfo(download.path).size()<=download.maximumSize &&
            (download.size < 0 || QFileInfo(download.path).size() == download.size) &&
            (download.hash.isEmpty() ||
             fileHash(download.path, download.algorithm).compare(download.hash, Qt::CaseInsensitive) == 0))
            continue;
        makeRequest(download.url);
        todo.append(download);
    }
    if (todo.isEmpty())
        return;

    struct Active {
        Download download;
        std::shared_ptr<QSaveFile> file;
        std::shared_ptr<QCryptographicHash> hash;
        qint64 bytes = 0;
        qint64 responseTotal = -1;
    };
    QMap<QNetworkReply *, Active> active;
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(100);
    qsizetype next = 0, finished = 0;
    qint64 completedBytes = 0;
    qint64 knownTotal = 0;
    bool allSizesKnown = true;
    for (const auto &download : todo) {
        allSizesKnown = allSizesKnown && download.size >= 0;
        if (download.size >= 0)
            knownTotal += download.size;
    }
    // Keep a single measurement basis for the whole batch. Switching from a file count to
    // a byte denominator half way through would make the same progress bar jump backwards.
    const bool singleUnknownFile = !allSizesKnown && todo.size() == 1;
    qint64 singleResponseTotal = -1;
    QString failure;
    QString currentFile = QFileInfo(todo.first().path).fileName();
    QElapsedTimer progressClock;
    progressClock.start();
    QElapsedTimer transferClock;
    transferClock.start();
    const auto amount = [](qint64 bytes) {
        return QString::number(double(bytes) / (1024.0 * 1024.0), 'f', bytes < 10 * 1024 * 1024 ? 2 : 1) +
               " MiB";
    };
    const auto reportProgress = [&](bool force = false) {
        if ((!ctx.progress && !ctx.transfer) || (!force && progressClock.elapsed() < 100))
            return;
        progressClock.restart();
        qint64 received = completedBytes;
        for (const auto &item : active)
            received += item.bytes;
        const qint64 total = allSizesKnown ? knownTotal : singleUnknownFile ? singleResponseTotal : -1;
        if(ctx.transfer)ctx.transfer(received,total,double(received)*1000.0/qMax<qint64>(1,transferClock.elapsed()));
        if(!ctx.progress)return;
        if (total > 0) {
            // The label describes downloaded bytes, not an invented estimate for extraction,
            // metadata resolution, or the rest of the Minecraft installation.
            ctx.progress("Downloading " + currentFile + " · " + amount(received) + " / " + amount(total),
                         qMin(received, total), total);
        } else if (!singleUnknownFile) {
            ctx.progress(QString("Downloading files · %1 of %2 complete · %3 received")
                             .arg(finished)
                             .arg(todo.size())
                             .arg(amount(received)),
                         finished, todo.size());
        } else {
            ctx.progress("Downloading " + currentFile + " · " + amount(received) + " received", 0, 0);
        }
    };
    const auto drain = [&](QNetworkReply *reply, Active &item) {
        const auto bytes = reply->readAll();
        if (bytes.isEmpty())
            return true;
        item.bytes += bytes.size();
        if ((item.download.size >= 0 && item.bytes > item.download.size) ||
            item.bytes > item.download.maximumSize || item.file->write(bytes) != bytes.size()) {
            failure = "Download size or disk-write error: " + QFileInfo(item.download.path).fileName();
            return false;
        }
        item.hash->addData(bytes);
        return true;
    };

    std::function<void()> pump;
    pump = [&] {
        while (failure.isEmpty() && !(ctx.cancel && ctx.cancel->load()) && active.size() < 10 &&
               next < todo.size()) {
            const auto download = todo[next++];
            if (!QDir().mkpath(QFileInfo(download.path).absolutePath())) {
                failure = "Cannot create download folder: " + QFileInfo(download.path).absolutePath();
                break;
            }
            auto file = std::make_shared<QSaveFile>(download.path);
            if (!file->open(QIODevice::WriteOnly)) {
                failure = "Cannot write download: " + download.path;
                break;
            }
            const auto algorithm = download.algorithm == "sha512"   ? QCryptographicHash::Sha512
                                   : download.algorithm == "sha256" ? QCryptographicHash::Sha256
                                   : download.algorithm == "md5"    ? QCryptographicHash::Md5
                                                                    : QCryptographicHash::Sha1;
            auto request = makeRequest(download.url);
            // A known Content-Length must describe the same bytes that are saved and hashed.
            request.setRawHeader("Accept-Encoding", "identity");
            auto *reply = manager.get(request);
            active.insert(reply, {download, file, std::make_shared<QCryptographicHash>(algorithm), 0, -1});
            QObject::connect(reply, &QNetworkReply::downloadProgress, &loop,
                             [&, reply](qint64, qint64 total) {
                                 auto entry = active.find(reply);
                                 if (entry == active.end())
                                     return;
                                 entry->responseTotal = total;
                                 if (singleUnknownFile && total >= 0)
                                     singleResponseTotal = total;
                             });
            QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&, reply] {
                auto entry = active.find(reply);
                if (entry == active.end())
                    return;
                currentFile = QFileInfo(entry->download.path).fileName();
                // abort() can synchronously emit finished(), so do not access entry afterwards.
                if (!drain(reply, entry.value()))
                    reply->abort();
            });
            QObject::connect(reply, &QNetworkReply::finished, &loop, [&, reply] {
                auto entry = active.find(reply);
                if (entry == active.end())
                    return;
                drain(reply, entry.value());
                auto item = active.take(reply);
                if (reply->error() != QNetworkReply::NoError) {
                    if (failure.isEmpty())
                        failure = "Download failed: " + QFileInfo(item.download.path).fileName() + " — " +
                                  reply->errorString();
                } else if ((item.download.size >= 0 && item.bytes != item.download.size) ||
                           (!item.download.hash.isEmpty() &&
                            QString::fromLatin1(item.hash->result().toHex())
                                    .compare(item.download.hash, Qt::CaseInsensitive) != 0)) {
                    failure = "Checksum or size mismatch: " + QFileInfo(item.download.path).fileName();
                } else if (failure.isEmpty() && !(ctx.cancel && ctx.cancel->load()) && !item.file->commit()) {
                    failure = "Cannot finish download: " + item.download.path;
                }
                reply->deleteLater();
                ++finished;
                completedBytes += item.bytes;
                currentFile = QFileInfo(item.download.path).fileName();
                if (singleUnknownFile && failure.isEmpty())
                    singleResponseTotal = item.bytes;
                if (failure.isEmpty())
                    reportProgress(finished == todo.size());
                pump();
                if (active.isEmpty())
                    loop.quit();
            });
        }
    };
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
        if (!failure.isEmpty() || (ctx.cancel && ctx.cancel->load())) {
            const auto replies = active.keys();
            for (auto *reply : replies)
                reply->abort();
            if (active.isEmpty())
                loop.quit();
        } else {
            reportProgress();
        }
    });
    pump();
    reportProgress(true);
    if (!active.isEmpty()) {
        timer.start();
        loop.exec();
    }
    if(ctx.transfer)ctx.transfer(completedBytes,allSizesKnown?knownTotal:singleUnknownFile?singleResponseTotal:-1,0);
    ctx.check();
    if (!failure.isEmpty())
        throw Error(failure);
}
} // namespace moly
