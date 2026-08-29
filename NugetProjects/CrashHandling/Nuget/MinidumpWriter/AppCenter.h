#pragma once
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QObject>
#include <QList>
#include <QDir>
#include <QUrl>

// TODO: add namespace to avoid conflicts with StacktraceRestorer nuget
struct StackFrame {
    QString fileName;
    QString className;
    QString methodName;
    int lineNumber;

    QJsonObject ToJsonObject();
};


class AppCenter : public QObject {
    Q_OBJECT;

    AppCenter(QObject* parent = nullptr);
public:
    ~AppCenter() = default;

    static AppCenter& GetInstance();

    void SetApplicationData(const QString& appSecret, const QString& instId, const QString& version);
    void SendCrashReport(const QString& exceptionMessage, QList<StackFrame> stackFrames, QString rawStackTrace = "", QList<QDir> attachmentDirs = {});

signals:
    void SendInternal(QJsonObject payload); // TODO: use private signals
    void ReportSendingStatus(bool success);

private:
    const size_t initializedThreadId;
    const QString appLaunchTimestamp;
    QNetworkAccessManager* manager;

    QString appSecret;
    QString instId = "00000000-0000-0000-0000-000000000001"; // use default id before we set it up manually
    QString version;
};