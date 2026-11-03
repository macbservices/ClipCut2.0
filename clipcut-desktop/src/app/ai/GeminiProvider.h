#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QTimer>

namespace drift::ai {

struct GeminiError {
    int httpStatusCode = 0;
    QString message;
    bool isQuotaExceeded = false;
    bool isNetworkFailure = false;
};

class GeminiProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool hasApiKey READ hasApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage NOTIFY errorOccurred)
    Q_PROPERTY(bool lastErrorWasQuota READ lastErrorWasQuota NOTIFY errorOccurred)

public:
    explicit GeminiProvider(QObject *parent = nullptr);
    ~GeminiProvider() override;

    bool busy() const { return m_busy; }
    bool hasApiKey() const;
    QString modelName() const { return m_modelName; }
    void setModelName(const QString &model);
    QString lastErrorMessage() const { return m_lastError.message; }
    bool lastErrorWasQuota() const { return m_lastError.isQuotaExceeded; }

    Q_INVOKABLE void setApiKey(const QString &key);
    Q_INVOKABLE QString getMaskedApiKey() const;
    Q_INVOKABLE void requestEditPlan(const QJsonObject &contextPayload, const QString &userInstruction);
    Q_INVOKABLE void cancelRequest();

signals:
    void busyChanged();
    void apiKeyChanged();
    void modelNameChanged();
    void planReceived(const QJsonObject &planJson, const QString &requestId);
    void rawTextResponseReceived(const QString &replyText);
    void errorOccurred(const QString &errorMessage, int statusCode);
    void quotaExceeded();

private slots:
    void onReplyFinished();
    void onRequestTimeout();

private:
    void setBusy(bool busy);
    QString buildSystemInstruction() const;
    QJsonObject parsePlanFromResponse(const QString &responseText) const;

    QNetworkAccessManager m_net;
    QNetworkReply *m_currentReply = nullptr;
    QTimer m_timeoutTimer;

    QString m_apiKey;
    QString m_modelName = QStringLiteral("gemini-2.5-flash");
    bool m_busy = false;
    GeminiError m_lastError;
    QString m_currentRequestId;
};

} // namespace drift::ai
