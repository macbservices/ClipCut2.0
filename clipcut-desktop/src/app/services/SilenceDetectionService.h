#pragma once

#include "core/base/Time.h"
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class AppController;

namespace drift::services {

struct SilenceInterval {
    double startSeconds = 0.0;
    double endSeconds = 0.0;
    double durationSeconds = 0.0;
    TimeUs startUs = 0;
    TimeUs endUs = 0;
};

class SilenceDetectionService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)
    
    // Configurable parameters
    Q_PROPERTY(double thresholdDb READ thresholdDb WRITE setThresholdDb NOTIFY paramsChanged)
    Q_PROPERTY(double minDuration READ minDuration WRITE setMinDuration NOTIFY paramsChanged)
    Q_PROPERTY(double paddingIn READ paddingIn WRITE setPaddingIn NOTIFY paramsChanged)
    Q_PROPERTY(double paddingOut READ paddingOut WRITE setPaddingOut NOTIFY paramsChanged)
    Q_PROPERTY(double minKeep READ minKeep WRITE setMinKeep NOTIFY paramsChanged)
    Q_PROPERTY(bool useVad READ useVad WRITE setUseVad NOTIFY paramsChanged)
    Q_PROPERTY(bool autoApply READ autoApply WRITE setAutoApply NOTIFY paramsChanged)

    // Results
    Q_PROPERTY(QVariantList detectedIntervals READ detectedIntervals NOTIFY intervalsChanged)
    Q_PROPERTY(double totalSilenceSeconds READ totalSilenceSeconds NOTIFY intervalsChanged)
    Q_PROPERTY(int intervalCount READ intervalCount NOTIFY intervalsChanged)

public:
    explicit SilenceDetectionService(AppController *app, QObject *parent = nullptr);
    ~SilenceDetectionService() override;

    bool busy() const { return m_busy; }
    double progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }
    QString lastError() const { return m_lastError; }

    double thresholdDb() const { return m_thresholdDb; }
    void setThresholdDb(double val);

    double minDuration() const { return m_minDuration; }
    void setMinDuration(double val);

    double paddingIn() const { return m_paddingIn; }
    void setPaddingIn(double val);

    double paddingOut() const { return m_paddingOut; }
    void setPaddingOut(double val);

    double minKeep() const { return m_minKeep; }
    void setMinKeep(double val);

    bool useVad() const { return m_useVad; }
    void setUseVad(bool val);

    bool autoApply() const { return m_autoApply; }
    void setAutoApply(bool val);

    QVariantList detectedIntervals() const { return m_intervalsVariant; }
    double totalSilenceSeconds() const { return m_totalSilenceSec; }
    int intervalCount() const { return m_intervals.size(); }
    const QList<SilenceInterval> &intervals() const { return m_intervals; }

    // Invocable actions
    Q_INVOKABLE void detectSilence(int trackIndex, int clipIndex);
    Q_INVOKABLE void detectSilenceInActiveClip();
    Q_INVOKABLE void detectSilenceFFmpeg(const QString &filePath, double thresholdDb, double minDurationSec);
    Q_INVOKABLE bool applyCuts(int trackIndex, int clipIndex);
    Q_INVOKABLE bool applyCutsToActiveClip();
    Q_INVOKABLE void clearResults();
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void statusTextChanged();
    void errorOccurred(const QString &errorMessage);
    void paramsChanged();
    void intervalsChanged();
    void detectionCompleted(int count, double totalSeconds);
    void cutsApplied(int cutsCount);

private:
    void setBusy(bool busy, const QString &status = {});
    void setProgress(double fraction);
    void setError(const QString &error);
    void updateIntervalsVariant();

    AppController *m_app = nullptr;
    bool m_busy = false;
    double m_progress = 0.0;
    QString m_statusText;
    QString m_lastError;

    // Config defaults
    double m_thresholdDb = -30.0;
    double m_minDuration = 0.5;
    double m_paddingIn = 0.1;
    double m_paddingOut = 0.1;
    double m_minKeep = 0.3;
    bool m_useVad = false;
    bool m_autoApply = false;

    // State
    QList<SilenceInterval> m_intervals;
    QVariantList m_intervalsVariant;
    double m_totalSilenceSec = 0.0;
    int m_lastTrack = -1;
    int m_lastClip = -1;
    bool m_cancelled = false;
};

} // namespace drift::services
