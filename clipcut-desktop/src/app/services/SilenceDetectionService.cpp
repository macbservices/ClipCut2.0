#include "app/services/SilenceDetectionService.h"
#include "app/editor/AppController.h"
#include "app/mcp/McpController.h"
#include "core/base/Time.h"

#include <QtConcurrent>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <cmath>

namespace drift::services {

SilenceDetectionService::SilenceDetectionService(AppController *app, QObject *parent)
    : QObject(parent), m_app(app)
{
}

SilenceDetectionService::~SilenceDetectionService() = default;

void SilenceDetectionService::setThresholdDb(double val)
{
    if (qFuzzyCompare(m_thresholdDb, val))
        return;
    m_thresholdDb = val;
    emit paramsChanged();
}

void SilenceDetectionService::setMinDuration(double val)
{
    if (qFuzzyCompare(m_minDuration, val))
        return;
    m_minDuration = qMax(0.1, val);
    emit paramsChanged();
}

void SilenceDetectionService::setPaddingIn(double val)
{
    if (qFuzzyCompare(m_paddingIn, val))
        return;
    m_paddingIn = qMax(0.0, val);
    emit paramsChanged();
}

void SilenceDetectionService::setPaddingOut(double val)
{
    if (qFuzzyCompare(m_paddingOut, val))
        return;
    m_paddingOut = qMax(0.0, val);
    emit paramsChanged();
}

void SilenceDetectionService::setMinKeep(double val)
{
    if (qFuzzyCompare(m_minKeep, val))
        return;
    m_minKeep = qMax(0.05, val);
    emit paramsChanged();
}

void SilenceDetectionService::setUseVad(bool val)
{
    if (m_useVad == val)
        return;
    m_useVad = val;
    emit paramsChanged();
}

void SilenceDetectionService::setAutoApply(bool val)
{
    if (m_autoApply == val)
        return;
    m_autoApply = val;
    emit paramsChanged();
}

void SilenceDetectionService::setBusy(bool busy, const QString &status)
{
    if (m_busy == busy && m_statusText == status)
        return;
    m_busy = busy;
    m_statusText = status;
    if (!busy)
        m_progress = 0.0;
    emit busyChanged();
    emit statusTextChanged();
    emit progressChanged();
}

void SilenceDetectionService::setProgress(double fraction)
{
    m_progress = std::clamp(fraction, 0.0, 1.0);
    emit progressChanged();
}

void SilenceDetectionService::setError(const QString &error)
{
    m_lastError = error;
    emit errorOccurred(error);
}

void SilenceDetectionService::clearResults()
{
    m_intervals.clear();
    m_intervalsVariant.clear();
    m_totalSilenceSec = 0.0;
    m_lastTrack = -1;
    m_lastClip = -1;
    emit intervalsChanged();
}

void SilenceDetectionService::updateIntervalsVariant()
{
    m_intervalsVariant.clear();
    m_totalSilenceSec = 0.0;

    for (int i = 0; i < m_intervals.size(); ++i) {
        const SilenceInterval &inter = m_intervals.at(i);
        m_totalSilenceSec += inter.durationSeconds;
        m_intervalsVariant.append(QVariantMap{
            {QStringLiteral("index"), i + 1},
            {QStringLiteral("start"), inter.startSeconds},
            {QStringLiteral("end"), inter.endSeconds},
            {QStringLiteral("duration"), inter.durationSeconds},
            {QStringLiteral("formattedStart"), QString::asprintf("%.2f s", inter.startSeconds)},
            {QStringLiteral("formattedDuration"), QString::asprintf("%.2f s", inter.durationSeconds)}
        });
    }

    emit intervalsChanged();
}

void SilenceDetectionService::detectSilenceInActiveClip()
{
    if (!m_app)
        return;
    detectSilence(m_app->activeTrackIndex(), m_app->activeClipIndex());
}

void SilenceDetectionService::detectSilence(int trackIndex, int clipIndex)
{
    if (!m_app || m_busy)
        return;

    if (trackIndex < 0 || clipIndex < 0) {
        setError(tr("Nenhum clipe selecionado na timeline"));
        return;
    }

    m_lastTrack = trackIndex;
    m_lastClip = clipIndex;
    m_cancelled = false;
    clearResults();

    setBusy(true, tr("Analisando silêncios no áudio…"));
    setProgress(0.1);

    // Threshold conversion: -30 dB -> ~0.0316
    const double linearThreshold = std::pow(10.0, m_thresholdDb / 20.0);
    const QString method = m_useVad ? QStringLiteral("vad") : QStringLiteral("energy");
    const double minDur = m_minDuration;
    const double pad = m_paddingIn;

    (void)QtConcurrent::run([this, trackIndex, clipIndex, linearThreshold, minDur, pad, method]() {
        McpController *mcp = m_app->mcp();
        if (!mcp) {
            QMetaObject::invokeMethod(this, [this]() {
                setBusy(false);
                setError(tr("Controlador MCP indisponível"));
            });
            return;
        }

        const QJsonObject res = mcp->detectSilence(trackIndex, clipIndex, 0, 0, linearThreshold, minDur, pad, method);

        QMetaObject::invokeMethod(this, [this, res, trackIndex, clipIndex]() {
            setBusy(false);
            if (m_cancelled)
                return;

            if (!res.value(QStringLiteral("ok")).toBool(false) && res.contains(QStringLiteral("error"))) {
                setError(res.value(QStringLiteral("error")).toString());
                return;
            }

            const QJsonArray ranges = res.value(QStringLiteral("ranges")).toArray();
            m_intervals.clear();

            for (const QJsonValue &v : ranges) {
                const QJsonObject r = v.toObject();
                SilenceInterval inter;
                inter.startSeconds = r.value(QStringLiteral("start")).toDouble();
                inter.endSeconds = r.value(QStringLiteral("end")).toDouble();
                inter.durationSeconds = inter.endSeconds - inter.startSeconds;
                inter.startUs = drift::secondsToUs(inter.startSeconds);
                inter.endUs = drift::secondsToUs(inter.endSeconds);

                if (inter.durationSeconds > 0.0)
                    m_intervals.append(inter);
            }

            updateIntervalsVariant();
            emit detectionCompleted(m_intervals.size(), m_totalSilenceSec);

            if (m_autoApply && !m_intervals.isEmpty()) {
                applyCuts(trackIndex, clipIndex);
            }
        });
    });
}

void SilenceDetectionService::detectSilenceFFmpeg(const QString &filePath, double thresholdDb, double minDurationSec)
{
    if (filePath.isEmpty() || m_busy)
        return;

    m_cancelled = false;
    clearResults();
    setBusy(true, tr("Executando filtro silencedetect do FFmpeg…"));

    (void)QtConcurrent::run([this, filePath, thresholdDb, minDurationSec]() {
        QProcess proc;
        QStringList args;
        args << QStringLiteral("-i") << filePath
             << QStringLiteral("-af")
             << QString::asprintf("silencedetect=noise=%.1fdB:d=%.2f", thresholdDb, minDurationSec)
             << QStringLiteral("-f") << QStringLiteral("null")
             << QStringLiteral("-");

        proc.start(QStringLiteral("ffmpeg"), args);
        if (!proc.waitForStarted(3000)) {
            QMetaObject::invokeMethod(this, [this]() {
                setBusy(false);
                setError(tr("Não foi possível iniciar processo do FFmpeg"));
            });
            return;
        }

        proc.waitForFinished(-1);
        const QString stderrOutput = QString::fromUtf8(proc.readAllStandardError());

        // Parse silencedetect lines:
        // silence_start: 1.234
        // silence_end: 2.567 | silence_duration: 1.333
        QList<SilenceInterval> parsed;
        const QRegularExpression reStart(QStringLiteral("silence_start:\\s*([0-9.]+)"));
        const QRegularExpression reEnd(QStringLiteral("silence_end:\\s*([0-9.]+)\\s*\\|\\s*silence_duration:\\s*([0-9.]+)"));

        double currentStart = -1.0;
        const QStringList lines = stderrOutput.split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            auto mS = reStart.match(line);
            if (mS.hasMatch()) {
                currentStart = mS.captured(1).toDouble();
            }
            auto mE = reEnd.match(line);
            if (mE.hasMatch()) {
                double endSec = mE.captured(1).toDouble();
                double durSec = mE.captured(2).toDouble();
                if (currentStart >= 0.0) {
                    SilenceInterval inter;
                    inter.startSeconds = currentStart;
                    inter.endSeconds = endSec;
                    inter.durationSeconds = durSec;
                    inter.startUs = drift::secondsToUs(currentStart);
                    inter.endUs = drift::secondsToUs(endSec);
                    parsed.append(inter);
                    currentStart = -1.0;
                }
            }
        }

        QMetaObject::invokeMethod(this, [this, parsed]() {
            setBusy(false);
            if (m_cancelled)
                return;

            m_intervals = parsed;
            updateIntervalsVariant();
            emit detectionCompleted(m_intervals.size(), m_totalSilenceSec);
        });
    });
}

bool SilenceDetectionService::applyCutsToActiveClip()
{
    if (!m_app)
        return false;
    return applyCuts(m_app->activeTrackIndex(), m_app->activeClipIndex());
}

bool SilenceDetectionService::applyCuts(int trackIndex, int clipIndex)
{
    if (!m_app || m_intervals.isEmpty())
        return false;

    McpController *mcp = m_app->mcp();
    if (!mcp) {
        setError(tr("Controlador MCP indisponível"));
        return false;
    }

    setBusy(true, tr("Aplicando cortes de silêncio na timeline…"));

    const double linearThreshold = std::pow(10.0, m_thresholdDb / 20.0);
    const QString method = m_useVad ? QStringLiteral("vad") : QStringLiteral("energy");

    const QJsonObject res = mcp->removeSilence(trackIndex, clipIndex, linearThreshold, m_minDuration, m_paddingIn, method);

    setBusy(false);

    if (res.value(QStringLiteral("ok")).toBool(false)) {
        const int cuts = m_intervals.size();
        clearResults();
        emit cutsApplied(cuts);
        return true;
    }

    setError(res.value(QStringLiteral("error")).toString(tr("Erro ao aplicar cortes na timeline")));
    return false;
}

void SilenceDetectionService::cancel()
{
    m_cancelled = true;
    setBusy(false, tr("Operação cancelada"));
}

} // namespace drift::services
