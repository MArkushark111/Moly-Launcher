#pragma once
#include "core.h"
#include <QDateTime>
#include <QFutureWatcher>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class Launcher : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList instances READ instances NOTIFY changed)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY changed)
    Q_PROPERTY(QString selectedId READ selectedId WRITE selectInstance NOTIFY changed)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY changed)
    Q_PROPERTY(QVariantList accounts READ accounts NOTIFY changed)
    Q_PROPERTY(QVariantMap activeAccount READ activeAccount NOTIFY changed)
    Q_PROPERTY(QVariantList versions READ versions NOTIFY catalogChanged)
    Q_PROPERTY(QStringList loaderVersions READ loaderVersions NOTIFY catalogChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QVariantMap pendingPlan READ pendingPlan NOTIFY planChanged)
    Q_PROPERTY(QVariantList jobs READ jobs NOTIFY jobsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(double progress READ progress NOTIFY statusChanged)
    Q_PROPERTY(QString taskInstanceId READ taskInstanceId NOTIFY statusChanged)
    Q_PROPERTY(QString taskOperation READ taskOperation NOTIFY statusChanged)
    Q_PROPERTY(double downloadSpeed READ downloadSpeed NOTIFY statusChanged)
    Q_PROPERTY(qint64 downloadedBytes READ downloadedBytes NOTIFY statusChanged)
    Q_PROPERTY(qint64 downloadTotal READ downloadTotal NOTIFY statusChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchStateChanged)
    Q_PROPERTY(bool loadingMore READ loadingMore NOTIFY searchStateChanged)
    Q_PROPERTY(bool canLoadMore READ canLoadMore NOTIFY searchStateChanged)
    Q_PROPERTY(bool hasMoreResults READ canLoadMore NOTIFY searchStateChanged)
    Q_PROPERTY(bool searchLoading READ searchLoading NOTIFY searchStateChanged)
    Q_PROPERTY(QString searchError READ searchError NOTIFY searchStateChanged)
    Q_PROPERTY(int resultTotal READ resultTotal NOTIFY searchStateChanged)
    Q_PROPERTY(bool canRetry READ canRetry NOTIFY statusChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logsChanged)
    Q_PROPERTY(QString dataRoot READ dataRoot NOTIFY changed)
    Q_PROPERTY(QString loginCode READ loginCode NOTIFY loginChanged)
    Q_PROPERTY(bool curseConnected READ curseConnected NOTIFY changed)
    Q_PROPERTY(int memoryTotal READ memoryTotal CONSTANT)
    Q_PROPERTY(QString installTargetId READ installTargetId NOTIFY changed)
    Q_PROPERTY(QString installError READ installError NOTIFY installStateChanged)
    Q_PROPERTY(QVariantList installVersions READ installVersions NOTIFY installStateChanged)
    Q_PROPERTY(bool loadingInstallChoices READ loadingInstallChoices NOTIFY installStateChanged)
    Q_PROPERTY(bool installAutomatic READ installAutomatic NOTIFY installStateChanged)
    Q_PROPERTY(QVariantList importCandidates READ importCandidates NOTIFY importScanChanged)
    Q_PROPERTY(bool importScanning READ importScanning NOTIFY importScanChanged)
    Q_PROPERTY(QString importScanError READ importScanError NOTIFY importScanChanged)
  public:
    explicit Launcher(QString root, QObject *parent = nullptr);
    ~Launcher() override;
    QVariantList instances() const;
    QVariantMap selected() const;
    QString selectedId() const {
        return selection;
    }
    QVariantMap settings() const {
        return config.toVariantMap();
    }
    QVariantList accounts() const {
        return accountList.toVariantList();
    }
    QVariantMap activeAccount() const;
    QVariantList versions() const {
        return versionList.toVariantList();
    }
    QStringList loaderVersions() const {
        return loaderList;
    }
    QVariantList results() const {
        return searchResults.toVariantList();
    }
    QVariantMap pendingPlan() const {
        return review.toVariantMap();
    }
    QVariantList jobs() const {
        return jobList;
    }
    bool busy() const {
        return working;
    }
    QString status() const {
        return statusText;
    }
    double progress() const {
        return progressValue;
    }
    QString taskInstanceId() const {
        return activeTaskInstanceId;
    }
    QString taskOperation() const {
        return activeTaskOperation;
    }
    double downloadSpeed() const { return transferSpeed; }
    qint64 downloadedBytes() const { return transferBytes; }
    qint64 downloadTotal() const { return transferTotal; }
    bool searching() const {
        return searchBusy;
    }
    bool loadingMore() const {
        return searchMore;
    }
    bool canLoadMore() const {
        return searchHasMore;
    }
    bool searchLoading() const {
        return searchBusy || searchMore;
    }
    QString searchError() const {
        return searchErrorText;
    }
    int resultTotal() const {
        return searchTotal;
    }
    bool canRetry() const {
        return lastTaskFailed && bool(retryWork);
    }
    QString logText() const;
    QString dataRoot() const {
        return root;
    }
    QString loginCode() const {
        return deviceCode;
    }
    bool curseConnected() const;
    int memoryTotal() const {
        return int(moly::physicalMemoryMb());
    }
    QString installTargetId() const;
    QString installError() const {
        return installErrorText;
    }
    QVariantList installVersions() const {
        return installVersionList.toVariantList();
    }
    bool loadingInstallChoices() const { return choicesLoading; }
    bool installAutomatic() const { return automaticInstall; }
    QVariantList importCandidates() const { return localCandidates.toVariantList(); }
    bool importScanning() const { return localScanBusy; }
    QString importScanError() const { return localScanError; }
    Q_INVOKABLE void scanLocalInstances(const QString &provider, const QString &folder = {});
    Q_INVOKABLE void importLocalInstance(const QString &candidateId, const QVariantMap &overrides, bool includeWorlds = true);
    Q_INVOKABLE void moveInstance(const QString &id, const QString &beforeId, bool after = false);
    Q_INVOKABLE void registerPackFiles();
    Q_INVOKABLE void loadInstallChoices(const QVariantMap &project, const QString &instanceId = QString());
    Q_INVOKABLE void selectInstance(const QString &id);
    Q_INVOKABLE void refreshVersions(bool snapshots = false);
    Q_INVOKABLE void refreshLoaders(const QString &version, const QString &loader);
    Q_INVOKABLE void createInstance(const QString &name, const QString &version, const QString &loader,
                                    const QString &loaderVersion, bool autoInstall = true);
    Q_INVOKABLE void saveInstance(const QVariantMap &values);
    Q_INVOKABLE void setInstanceIcon(const QString &url);
    Q_INVOKABLE void resetInstanceIcon();
    Q_INVOKABLE void setInstanceBackground(const QString &url);
    Q_INVOKABLE void resetInstanceBackground();
    Q_INVOKABLE void refreshPackIcons();
    Q_INVOKABLE void duplicateInstance();
    Q_INVOKABLE void deleteInstance();
    Q_INVOKABLE void installSelected();
    Q_INVOKABLE void play(bool offline = false);
    Q_INVOKABLE void playTarget(const QString &instanceId, const QString &type, const QString &value);
    Q_INVOKABLE void applySkin(const QString &path, const QString &variant);
    Q_INVOKABLE void stopGame();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void retry();
    Q_INVOKABLE void openFolder(const QString &part = QString());
    Q_INVOKABLE void openLink(const QString &url);
    Q_INVOKABLE void saveSettings(const QVariantMap &values);
    Q_INVOKABLE void setAccentColor(const QString &color);
    Q_INVOKABLE void setReducedMotion(bool enabled);
    Q_INVOKABLE void setUiPreferences(const QVariantMap &values);
    Q_INVOKABLE void clearCompletedJobs();
    Q_INVOKABLE void dismissJob(const QString &id);
    Q_INVOKABLE void saveCurseKey(const QString &key);
    Q_INVOKABLE void addOfflineAccount(const QString &name);
    Q_INVOKABLE void signIn();
    Q_INVOKABLE void selectAccount(const QString &id);
    Q_INVOKABLE void removeAccount(const QString &id);
    Q_INVOKABLE void search(const QString &provider, const QString &type, const QString &query,
                            const QString &category = QString());
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void loadMoreResults();
    Q_INVOKABLE void beginInstall(const QVariantMap &project, const QString &instanceId = QString(),
                                  const QString &versionId = QString(), bool automatic = false);
    Q_INVOKABLE void planInstall(const QVariantMap &project);
    Q_INVOKABLE void checkUpdates();
    Q_INVOKABLE void dismissPlan();
    Q_INVOKABLE void resolveFile(int index, const QString &url);
    Q_INVOKABLE void applyPlan();
    Q_INVOKABLE void importPack(const QString &url, bool prepareGame = true);
    Q_INVOKABLE void importPrism(const QString &url);
    Q_INVOKABLE QVariantList contentFiles(const QString &type) const;
    Q_INVOKABLE void importFile(const QString &url, const QString &type);
    Q_INVOKABLE void toggleFile(const QString &path);
    Q_INVOKABLE void removeFile(const QString &path);
    Q_INVOKABLE void backupWorlds();
    Q_INVOKABLE QStringList backups() const;
    Q_INVOKABLE void restoreBackup(const QString &name);
    Q_INVOKABLE void saveLog(const QString &url);
    Q_INVOKABLE void exportContentList(const QString &url);
    Q_INVOKABLE void exportInstance(const QString &url, const QString &format, bool includeWorlds = false);
    Q_INVOKABLE bool isInstanceRunning(const QString &id) const { return processes.contains(id); }
    Q_INVOKABLE QVariantList worldList() const;
    Q_INVOKABLE void relocate(const QString &url);
  signals:
    void changed();
    void catalogChanged();
    void resultsChanged();
    void searchStateChanged();
    void jobsChanged();
    void busyChanged();
    void statusChanged();
    void logsChanged();
    void planChanged();
    void loginChanged();
    void notice(QString message, bool error);
    void navigate(QString page);
    void gameStarted();
    void gameEnded();
    void installStateChanged();
    void installFailed(QString message);
    void installCompleted(QString message);
    void importScanChanged();

  private:
    QString root, selection, statusText = "Ready when you are", deviceCode, installErrorText;
    QString activeTaskInstanceId, activeTaskOperation;
    QString retryInstanceId, retryOperation;
    QJsonObject config, review;
    QJsonObject pendingInstance;
    QFutureWatcher<QJsonObject> localScanWatcher;
    std::atomic_bool localScanCancelled = false;
    QJsonArray localCandidates;
    QString localScanError, localScanProvider;
    QStringList localScanRoots;
    quint64 localScanGeneration = 0;
    bool localScanBusy = false, localScanPending = false;
    void startLocalScan();
    QJsonArray accountList, versionList, searchResults, installVersionList;
    QString searchProvider, searchType, searchQuery, searchCategory, searchErrorText;
    QJsonObject searchInstance;
    quint64 searchGeneration = 0;
    bool pendingSearch = false, lastSearchAppend = false;
    static constexpr int SearchPageSize = 30;
    int searchOffset = 0, searchTotal = 0;
    bool searchBusy = false, searchMore = false, searchHasMore = false;
    QList<QJsonObject> instanceList;
    QStringList loaderList;
    QVariantList jobList;
    bool working = false;
    bool lastTaskFailed = false;
    double progressValue = 0;
    double transferSpeed = 0;
    qint64 transferBytes = 0, transferTotal = -1;
    QJsonObject launchTarget;
    std::atomic_bool cancelled = false;
    QFutureWatcher<QJsonObject> watcher;
    QFutureWatcher<QJsonArray> iconWatcher;
    QFutureWatcher<QJsonObject> choicesWatcher;
    std::atomic_bool choicesCancelled = false;
    quint64 choicesGeneration = 0;
    bool choicesLoading = false, choicesPending = false, automaticInstall = false;
    QJsonObject choicesProject, choicesInstance;
    QMap<QString, QJsonArray> choicesCache;
    void startInstallChoices();
    std::atomic_bool iconCancelled = false;
    bool iconsAgain = false;
    std::function<QJsonObject(moly::Context)> retryWork;
    std::function<void(QJsonObject)> retryDone;
    QString retryTitle;
    QMap<QString, QProcess *> processes;
    QMap<QString, QStringList> logLines;
    QMap<QString, QString> partialLines;
    QMap<QString, QStringList> processSecrets;
    QMap<QString, QDateTime> startedAt;
    quint64 taskGeneration = 0;
    QTimer logTimer;
    bool dirtyLogs = false;
    void load();
    void persist();
    void reloadInstances();
    QJsonObject current() const;
    QString instanceDir(const QJsonObject &i) const;
    void storeInstance(const QJsonObject &i);
    bool editable();
    void task(const QString &title, std::function<QJsonObject(moly::Context)> work,
              std::function<void(QJsonObject)> done = {}, const QString &instanceId = {},
              const QString &operation = {});
    void appendLog(const QString &id, const QString &text);
    void launchProcess(QJsonObject instance, QJsonObject installation, QJsonObject account, QString token);
    void enrichReview(QJsonObject plan, const QString &targetId = QString());
    void setInstallError(const QString &message);
    void startSearch(bool append);
};
