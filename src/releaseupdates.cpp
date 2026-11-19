#include "releaseupdates.h"
#include "core.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QVersionNumber>
#include <memory>

namespace moly {
static const QString repository = "https://github.com/MArkushark111/Moly-Launcher";
static QVersionNumber stableVersion(QString value) {
    if (value.startsWith('v', Qt::CaseInsensitive)) value.remove(0, 1);
    if (!QRegularExpression("^[0-9]+\\.[0-9]+\\.[0-9]+(?:\\.[0-9]+)?$").match(value).hasMatch())
        throw Error("The release tag must be a stable version such as v0.8.5.");
    const auto version = QVersionNumber::fromString(value);
    if (version.isNull()) throw Error("The release has an invalid version number.");
    return version;
}
static bool officialUrl(const QString &value, const QString &pathPrefix) {
    const QUrl url(value);
    return url.isValid() && url.scheme() == "https" && url.host() == "github.com"
        && url.userInfo().isEmpty() && url.port(-1) == -1 && url.query().isEmpty()
        && url.fragment().isEmpty() && url.path().startsWith("/MArkushark111/Moly-Launcher/" + pathPrefix);
}
QVariantMap parseLauncherRelease(const QJsonObject &release, const QString &currentVersion) {
    if (release["draft"].toBool() || release["prerelease"].toBool())
        throw Error("Only published stable releases are offered as launcher updates.");
    const auto tag = release["tag_name"].toString();
    QVersionNumber version;
    try { version = stableVersion(tag); }
    catch (const Error &) {
        // Older Moly releases use a generic tag ("Release") and a versioned title.
        const auto titleVersion = QRegularExpression("^(?:MolyLauncher|Moly Launcher)\\s+[vV]?([0-9]+\\.[0-9]+\\.[0-9]+(?:\\.[0-9]+)?)$",
            QRegularExpression::CaseInsensitiveOption).match(release["name"].toString().trimmed());
        if (!titleVersion.hasMatch()) throw;
        version = stableVersion(titleVersion.captured(1));
    }
    const auto page = release["html_url"].toString();
    if (!officialUrl(page, "releases/tag/")) throw Error("GitHub returned an unexpected release link.");
    QString installer;
    const auto filename = "MolyLauncher-" + version.toString() + "-Windows-x64-Setup.exe";
    for (const auto &value : release["assets"].toArray()) {
        const auto asset = value.toObject();
        const auto url = asset["browser_download_url"].toString();
        const auto name = asset["name"].toString();
        const bool versioned = name.compare(filename, Qt::CaseInsensitive) == 0;
        if ((versioned || name.compare("MolyInstall.exe", Qt::CaseInsensitive) == 0)
            && asset["state"].toString() == "uploaded" && asset["size"].toDouble() > 0
            && officialUrl(url, "releases/download/") && QUrl(url).path().endsWith('/' + asset["name"].toString())) {
            installer = url;
            if (versioned) break;
        }
    }
    return {{"available", QVersionNumber::compare(version, stableVersion(currentVersion)) > 0},
        {"version", version.toString()}, {"tag", tag}, {"name", release["name"].toString(tag).left(200)},
        {"notes", release["body"].toString().left(100000)}, {"url", page}, {"installerUrl", installer}};
}
ReleaseUpdates::ReleaseUpdates(QObject *parent) : QObject(parent), network_(this) {
    autoCheck_ = QSettings().value("updates/checkAutomatically", true).toBool();
}
QString ReleaseUpdates::currentVersion() const {
    return QCoreApplication::applicationVersion().isEmpty() ? "0.8.5" : QCoreApplication::applicationVersion();
}
void ReleaseUpdates::setAutoCheck(bool enabled) {
    if (autoCheck_ == enabled) return;
    autoCheck_ = enabled;
    QSettings().setValue("updates/checkAutomatically", enabled);
    emit changed();
}
void ReleaseUpdates::check() {
    if (checking_) return;
    checking_ = true;
    status_ = "Checking GitHub releases…";
    emit changed();
    QNetworkRequest request(QUrl("https://api.github.com/repos/MArkushark111/Moly-Launcher/releases/latest"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "MolyLauncher/" + currentVersion());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(20000);
    if (!etag_.isEmpty()) request.setRawHeader("If-None-Match", etag_);
    auto reply = network_.get(request);
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, this, [reply, bytes] {
        if (bytes->size() + reply->bytesAvailable() > 2 * 1024 * 1024) { reply->abort(); return; }
        bytes->append(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes] {
        checking_ = false;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        try {
            if (status == 304) {
                status_ = available() ? "Moly " + release_["version"].toString() + " is available." : "You’re up to date.";
            } else if (status == 404) {
                release_.clear(); etag_.clear(); status_ = "No published stable release is available yet.";
            } else {
                if (status == 403 || status == 429) throw Error("GitHub has limited update checks. Try again later.");
                if (reply->error() != QNetworkReply::NoError || status != 200)
                    throw Error("Could not check GitHub. Check your connection and try again.");
                if (bytes->size() + reply->bytesAvailable() > 2 * 1024 * 1024)
                    throw Error("GitHub’s release response was too large.");
                bytes->append(reply->readAll());
                QJsonParseError error;
                const auto document = QJsonDocument::fromJson(*bytes, &error);
                if (error.error != QJsonParseError::NoError || !document.isObject()) throw Error("GitHub returned invalid release information.");
                release_ = parseLauncherRelease(document.object(), currentVersion());
                etag_ = reply->rawHeader("ETag");
                status_ = available() ? "Moly " + release_["version"].toString() + " is available." : "You’re up to date.";
            }
        } catch (const std::exception &error) { status_ = QString::fromUtf8(error.what()); }
        reply->deleteLater();
        emit changed();
    });
}
bool ReleaseUpdates::openUpdate() {
    if (!available() || checking_) return false;
    const auto url = release_["installerUrl"].toString();
    if (url.isEmpty()) return openRelease();
    if (!officialUrl(url, "releases/download/")) return false;
    return QDesktopServices::openUrl(QUrl(url));
}
bool ReleaseUpdates::openRelease() {
    const auto url = release_.value("url", repository + "/releases").toString();
    if (url != repository + "/releases" && !officialUrl(url, "releases/tag/")) return false;
    return QDesktopServices::openUrl(QUrl(url));
}
}
