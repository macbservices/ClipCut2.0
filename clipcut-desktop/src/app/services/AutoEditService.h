#pragma once

#include "core/base/Time.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class AppController;

namespace drift::services {

class SilenceDetectionService;

struct AutoEditTask {
    QString id;
    QString title;
    QString description;
    bool enabled = true;
};

class AutoEditService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)

    Q_PROPERTY(bool hasPlan READ hasPlan NOTIFY planChanged)
    Q_PROPERTY(QString planExplanation READ planExplanation NOTIFY planChanged)
    Q_PROPERTY(QVariantList planActions READ planActions NOTIFY planChanged)

    // Option flags selectable by user
    Q_PROPERTY(bool optRemoveSilence READ optRemoveSilence WRITE setOptRemoveSilence NOTIFY optionsChanged)
    Q_PROPERTY(bool optTranscribe READ optTranscribe WRITE setOptTranscribe NOTIFY optionsChanged)
    Q_PROPERTY(bool optNormalizeAudio READ optNormalizeAudio WRITE setOptNormalizeAudio NOTIFY optionsChanged)
    Q_PROPERTY(bool optApplyTransitions READ optApplyTransitions WRITE setOptApplyTransitions NOTIFY optionsChanged)
    Q_PROPERTY(QString optTransitionKind READ optTransitionKind WRITE setOptTransitionKind NOTIFY optionsChanged)
    Q_PROPERTY(double optTransitionDuration READ optTransitionDuration WRITE setOptTransitionDuration NOTIFY optionsChanged)
    Q_PROPERTY(double optTargetLufs READ optTargetLufs WRITE setOptTargetLufs NOTIFY optionsChanged)
    Q_PROPERTY(QString optLanguage READ optLanguage WRITE setOptLanguage NOTIFY optionsChanged)

    // Silence parameters
    Q_PROPERTY(double silenceThresholdDb READ silenceThresholdDb WRITE setSilenceThresholdDb NOTIFY optionsChanged)
    Q_PROPERTY(double silenceMinDuration READ silenceMinDuration WRITE setSilenceMinDuration NOTIFY optionsChanged)

public:
    explicit AutoEditService(AppController *app, SilenceDetectionService *silenceService, QObject *parent = nullptr);
    ~AutoEditService() override;

    bool busy() const { return m_busy; }
    double progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }
    QString lastError() const { return m_lastError; }

    bool hasPlan() const { return m_hasPlan; }
    QString planExplanation() const { return m_planExplanation; }
    QVariantList planActions() const { return m_planActions; }

    bool optRemoveSilence() const { return m_optRemoveSilence; }
    void setOptRemoveSilence(bool val);

    bool optTranscribe() const { return m_optTranscribe; }
    void setOptTranscribe(bool val);

    bool optNormalizeAudio() const { return m_optNormalizeAudio; }
    void setOptNormalizeAudio(bool val);

    bool optApplyTransitions() const { return m_optApplyTransitions; }
    void setOptApplyTransitions(bool val);

    QString optTransitionKind() const { return m_optTransitionKind; }
    void setOptTransitionKind(const QString &val);

    double optTransitionDuration() const { return m_optTransitionDuration; }
    void setOptTransitionDuration(double val);

    double optTargetLufs() const { return m_optTargetLufs; }
    void setOptTargetLufs(double val);

    QString optLanguage() const { return m_optLanguage; }
    void setOptLanguage(const QString &val);

    double silenceThresholdDb() const { return m_silenceThresholdDb; }
    void setSilenceThresholdDb(double val);

    double silenceMinDuration() const { return m_silenceMinDuration; }
    void setSilenceMinDuration(double val);

    // Invocables
    Q_INVOKABLE void generatePlanForActiveClip();
    Q_INVOKABLE void generatePlan(int trackIndex, int clipIndex);
    Q_INVOKABLE void executePlan();
    Q_INVOKABLE void rejectPlan();
    Q_INVOKABLE void cancel();

    // Direct automated actions
    Q_INVOKABLE bool exportSubtitlesToSrt(const QString &targetFilePath);
    Q_INVOKABLE bool applyDirectCut(int trackIndex, int clipIndex, double atSeconds);
    Q_INVOKABLE bool applyTransitionBetweenClips(int trackIndex, int leftClipIndex, const QString &kind, double durationSec);
    Q_INVOKABLE bool normalizeClipAudio(int trackIndex, int clipIndex, double targetLufs);

signals:
    void busyChanged();
    void progressChanged();
    void statusTextChanged();
    void errorOccurred(const QString &errorMessage);
    void planChanged();
    void optionsChanged();
    void autoEditCompleted(const QString &summary);

private:
    void setBusy(bool busy, const QString &status = {});
    void setProgress(double fraction);
    void setError(const QString &error);
    void clearPlan();

    AppController *m_app = nullptr;
    SilenceDetectionService *m_silence = nullptr;

    bool m_busy = false;
    double m_progress = 0.0;
    QString m_statusText;
    QString m_lastError;

    bool m_hasPlan = false;
    QString m_planExplanation;
    QVariantList m_planActions;
    QList<AutoEditTask> m_tasks;

    int m_targetTrack = -1;
    int m_targetClip = -1;

    // Options
    bool m_optRemoveSilence = true;
    bool m_optTranscribe = true;
    bool m_optNormalizeAudio = true;
    bool m_optApplyTransitions = false;
    QString m_optTransitionKind = QStringLiteral("crossfade");
    double m_optTransitionDuration = 0.5;
    double m_optTargetLufs = -14.0;
    QString m_optLanguage = QStringLiteral("pt");
    double m_silenceThresholdDb = -30.0;
    double m_silenceMinDuration = 0.5;

    bool m_cancelled = false;
};

} // namespace drift::services
