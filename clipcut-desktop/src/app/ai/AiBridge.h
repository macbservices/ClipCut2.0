#pragma once

#include "app/ai/EditCommandExecutor.h"
#include "app/ai/GeminiProvider.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class AppController;

namespace drift::ai {

struct ConversationMessage {
    QString sender; // "user", "gemini", "system"
    QString text;
    QString timestamp;
    bool hasPlan = false;
    QJsonObject plan;
};

class AiBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)
    Q_PROPERTY(QString planExplanation READ planExplanation NOTIFY planChanged)
    Q_PROPERTY(QVariantList planActions READ planActions NOTIFY planChanged)
    Q_PROPERTY(bool hasPendingPlan READ hasPendingPlan NOTIFY planChanged)
    Q_PROPERTY(bool requiresConfirmation READ requiresConfirmation NOTIFY planChanged)
    Q_PROPERTY(bool vadAvailable READ vadAvailable CONSTANT)
    Q_PROPERTY(bool whisperAvailable READ whisperAvailable CONSTANT)

    // Phase 4 additions
    Q_PROPERTY(QVariantList conversationHistory READ conversationHistory NOTIFY conversationChanged)
    Q_PROPERTY(bool hasApiKey READ hasApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString maskedApiKey READ maskedApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(GeminiProvider *gemini READ gemini CONSTANT)
    Q_PROPERTY(bool useLocalFallback READ useLocalFallback WRITE setUseLocalFallback NOTIFY fallbackChanged)

public:
    explicit AiBridge(AppController *controller, QObject *parent = nullptr);
    ~AiBridge() override;

    bool busy() const { return m_busy || (m_gemini && m_gemini->busy()); }
    double progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }
    QString lastError() const { return m_lastError; }
    QString planExplanation() const { return m_planExplanation; }
    QVariantList planActions() const { return m_planActions; }
    bool hasPendingPlan() const { return m_hasPendingPlan; }
    bool requiresConfirmation() const { return m_requiresConfirmation; }
    bool vadAvailable() const;
    bool whisperAvailable() const;

    QVariantList conversationHistory() const { return m_conversationVariant; }
    bool hasApiKey() const { return m_gemini && m_gemini->hasApiKey(); }
    QString maskedApiKey() const { return m_gemini ? m_gemini->getMaskedApiKey() : QString(); }
    GeminiProvider *gemini() const { return m_gemini; }
    bool useLocalFallback() const { return m_useLocalFallback; }
    void setUseLocalFallback(bool fallback);

    // Context collection
    QJsonObject collectProjectContext() const;

    // Chat and Gemini commands
    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void setApiKey(const QString &key);
    Q_INVOKABLE void clearConversation();

    // Local / MCP Operations
    Q_INVOKABLE void analyzeSilence(double threshold = 0.02, double minDuration = 0.4,
                                   double padding = 0.1, const QString &method = QStringLiteral("energy"));
    Q_INVOKABLE void requestSilenceRemoval(double threshold = 0.02, double minDuration = 0.4,
                                          double padding = 0.1, const QString &method = QStringLiteral("energy"));
    Q_INVOKABLE void transcribeActiveClip(const QString &languageCode = QStringLiteral("pt"));
    Q_INVOKABLE void normalizeActiveClipAudio(double targetLufs = -14.0);
    Q_INVOKABLE void submitNaturalLanguagePrompt(const QString &prompt);
    Q_INVOKABLE void executePlan();
    Q_INVOKABLE void rejectPlan();
    Q_INVOKABLE void cancelCurrentOperation();

signals:
    void busyChanged();
    void progressChanged();
    void statusTextChanged();
    void errorOccurred(const QString &errorMessage);
    void planChanged();
    void operationFinished(const QString &message);
    void conversationChanged();
    void apiKeyChanged();
    void fallbackChanged();

private slots:
    void onGeminiPlanReceived(const QJsonObject &planJson, const QString &requestId);
    void onGeminiError(const QString &errorMessage, int statusCode);
    void onGeminiQuotaExceeded();

private:
    void setBusy(bool busy, const QString &status = {});
    void setProgress(double fraction);
    void setError(const QString &error);
    void clearPlan();
    void appendMessage(const QString &sender, const QString &text, bool hasPlan = false, const QJsonObject &plan = {});

    AppController *m_app = nullptr;
    GeminiProvider *m_gemini = nullptr;
    EditCommandExecutor *m_executor = nullptr;

    bool m_busy = false;
    double m_progress = 0.0;
    QString m_statusText;
    QString m_lastError;
    QString m_planExplanation;
    QVariantList m_planActions;
    QJsonObject m_currentPlanJson;
    bool m_hasPendingPlan = false;
    bool m_requiresConfirmation = false;
    bool m_cancelled = false;

    bool m_useLocalFallback = false;
    QList<ConversationMessage> m_messages;
    QVariantList m_conversationVariant;
};

} // namespace drift::ai
