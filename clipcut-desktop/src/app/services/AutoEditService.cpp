#include "app/services/AutoEditService.h"
#include "app/services/SilenceDetectionService.h"
#include "app/editor/AppController.h"
#include "app/editor/sessions/SubtitleController.h"
#include "app/mcp/McpController.h"
#include "core/project/Transition.h"

#include <QtConcurrent>
#include <QFile>
#include <QTextStream>
#include <QTimer>

namespace drift::services {

AutoEditService::AutoEditService(AppController *app, SilenceDetectionService *silenceService, QObject *parent)
    : QObject(parent), m_app(app), m_silence(silenceService)
{
}

AutoEditService::~AutoEditService() = default;

void AutoEditService::setOptRemoveSilence(bool val)
{
    if (m_optRemoveSilence == val)
        return;
    m_optRemoveSilence = val;
    emit optionsChanged();
}

void AutoEditService::setOptTranscribe(bool val)
{
    if (m_optTranscribe == val)
        return;
    m_optTranscribe = val;
    emit optionsChanged();
}

void AutoEditService::setOptNormalizeAudio(bool val)
{
    if (m_optNormalizeAudio == val)
        return;
    m_optNormalizeAudio = val;
    emit optionsChanged();
}

void AutoEditService::setOptApplyTransitions(bool val)
{
    if (m_optApplyTransitions == val)
        return;
    m_optApplyTransitions = val;
    emit optionsChanged();
}

void AutoEditService::setOptTransitionKind(const QString &val)
{
    if (m_optTransitionKind == val)
        return;
    m_optTransitionKind = val;
    emit optionsChanged();
}

void AutoEditService::setOptTransitionDuration(double val)
{
    if (qFuzzyCompare(m_optTransitionDuration, val))
        return;
    m_optTransitionDuration = qBound(0.1, val, 2.0);
    emit optionsChanged();
}

void AutoEditService::setOptTargetLufs(double val)
{
    if (qFuzzyCompare(m_optTargetLufs, val))
        return;
    m_optTargetLufs = val;
    emit optionsChanged();
}

void AutoEditService::setOptLanguage(const QString &val)
{
    if (m_optLanguage == val)
        return;
    m_optLanguage = val;
    emit optionsChanged();
}

void AutoEditService::setSilenceThresholdDb(double val)
{
    if (qFuzzyCompare(m_silenceThresholdDb, val))
        return;
    m_silenceThresholdDb = val;
    if (m_silence)
        m_silence->setThresholdDb(val);
    emit optionsChanged();
}

void AutoEditService::setSilenceMinDuration(double val)
{
    if (qFuzzyCompare(m_silenceMinDuration, val))
        return;
    m_silenceMinDuration = val;
    if (m_silence)
        m_silence->setMinDuration(val);
    emit optionsChanged();
}

void AutoEditService::setBusy(bool busy, const QString &status)
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

void AutoEditService::setProgress(double fraction)
{
    m_progress = std::clamp(fraction, 0.0, 1.0);
    emit progressChanged();
}

void AutoEditService::setError(const QString &error)
{
    m_lastError = error;
    emit errorOccurred(error);
}

void AutoEditService::clearPlan()
{
    m_hasPlan = false;
    m_planExplanation.clear();
    m_planActions.clear();
    m_tasks.clear();
    emit planChanged();
}

void AutoEditService::generatePlanForActiveClip()
{
    if (!m_app)
        return;
    generatePlan(m_app->activeTrackIndex(), m_app->activeClipIndex());
}

void AutoEditService::generatePlan(int trackIndex, int clipIndex)
{
    if (!m_app || m_busy)
        return;

    if (trackIndex < 0 || clipIndex < 0) {
        setError(tr("Nenhum clipe selecionado. Selecione um clipe na timeline para planejar a edição automática."));
        return;
    }

    m_targetTrack = trackIndex;
    m_targetClip = clipIndex;
    m_cancelled = false;
    clearPlan();

    setBusy(true, tr("Analisando clipe e preparando plano de edição automática…"));
    setProgress(0.2);

    (void)QtConcurrent::run([this, trackIndex, clipIndex]() {
        // Run silence analysis if requested
        int silenceCount = 0;
        double silenceSec = 0.0;

        if (m_silence) {
            m_silence->setThresholdDb(m_silenceThresholdDb);
            m_silence->setMinDuration(m_silenceMinDuration);
        }

        McpController *mcp = m_app->mcp();
        if (mcp && m_optRemoveSilence) {
            const double linThresh = std::pow(10.0, m_silenceThresholdDb / 20.0);
            const QJsonObject silRes = mcp->detectSilence(trackIndex, clipIndex, 0, 0, linThresh, m_silenceMinDuration, 0.1, QStringLiteral("energy"));
            if (silRes.value(QStringLiteral("ok")).toBool(false)) {
                const QJsonArray r = silRes.value(QStringLiteral("ranges")).toArray();
                silenceCount = r.size();
                for (const auto &v : r) {
                    const QJsonObject o = v.toObject();
                    silenceSec += (o.value(QStringLiteral("end")).toDouble() - o.value(QStringLiteral("start")).toDouble());
                }
            }
        }

        QMetaObject::invokeMethod(this, [this, silenceCount, silenceSec, trackIndex, clipIndex]() {
            setBusy(false);
            if (m_cancelled)
                return;

            QStringList lines;
            lines << tr("=== PLANO DE EDIÇÃO AUTOMÁTICA ===");
            lines << tr("Clipe selecionado: Trilha %1, Clipe %2").arg(trackIndex + 1).arg(clipIndex + 1);

            m_planActions.clear();
            m_tasks.clear();

            if (m_optRemoveSilence) {
                if (silenceCount > 0) {
                    QString desc = tr("Remover %1 trechos de silêncio (total de %2 s) preservando a sincronia áudio/vídeo.")
                                       .arg(silenceCount)
                                       .arg(silenceSec, 0, 'f', 2);
                    lines << QStringLiteral("1. ") + desc;
                    m_tasks.append({QStringLiteral("silence"), tr("Corte de Silêncios"), desc, true});
                    m_planActions.append(QVariantMap{
                        {QStringLiteral("title"), tr("Remoção de Silêncios")},
                        {QStringLiteral("description"), desc},
                        {QStringLiteral("count"), silenceCount},
                        {QStringLiteral("seconds"), silenceSec}
                    });
                } else {
                    lines << tr("1. Silêncio: Nenhum trecho significativo detectado com limiar %1 dB.").arg(m_silenceThresholdDb);
                }
            }

            if (m_optNormalizeAudio) {
                QString desc = tr("Medição ITU-R BS.1770 e normalização da faixa para %1 LUFS.").arg(m_optTargetLufs, 0, 'f', 1);
                lines << QStringLiteral("2. ") + desc;
                m_tasks.append({QStringLiteral("audio"), tr("Normalização de Áudio"), desc, true});
                m_planActions.append(QVariantMap{
                    {QStringLiteral("title"), tr("Normalização de Áudio")},
                    {QStringLiteral("description"), desc},
                    {QStringLiteral("targetLufs"), m_optTargetLufs}
                });
            }

            if (m_optTranscribe) {
                QString desc = tr("Transcrição automática em português (pt-BR) com geração de legendas sincronizadas.");
                lines << QStringLiteral("3. ") + desc;
                m_tasks.append({QStringLiteral("transcribe"), tr("Legendas Automáticas"), desc, true});
                m_planActions.append(QVariantMap{
                    {QStringLiteral("title"), tr("Transcrição e Legendas")},
                    {QStringLiteral("description"), desc},
                    {QStringLiteral("lang"), m_optLanguage}
                });
            }

            if (m_optApplyTransitions) {
                QString desc = tr("Aplicação de transição suave ('%1', %2 s) entre os cortes.").arg(m_optTransitionKind).arg(m_optTransitionDuration, 0, 'f', 1);
                lines << QStringLiteral("4. ") + desc;
                m_tasks.append({QStringLiteral("transition"), tr("Transições"), desc, true});
                m_planActions.append(QVariantMap{
                    {QStringLiteral("title"), tr("Transições de Corte")},
                    {QStringLiteral("description"), desc},
                    {QStringLiteral("kind"), m_optTransitionKind}
                });
            }

            lines << QString();
            lines << tr("Revise os passos acima e clique em 'Aprovar e Executar' para aplicar na timeline.");

            m_planExplanation = lines.join(QLatin1Char('\n'));
            m_hasPlan = true;
            emit planChanged();
        });
    });
}

void AutoEditService::executePlan()
{
    if (!m_hasPlan || !m_app || m_targetTrack < 0 || m_targetClip < 0)
        return;

    setBusy(true, tr("Executando edição automática na timeline…"));
    setProgress(0.1);

    int executedCount = 0;

    // 1. Remover silêncios
    if (m_optRemoveSilence && m_silence) {
        setProgress(0.3);
        const double linThresh = std::pow(10.0, m_silenceThresholdDb / 20.0);
        McpController *mcp = m_app->mcp();
        if (mcp) {
            mcp->removeSilence(m_targetTrack, m_targetClip, linThresh, m_silenceMinDuration, 0.1, QStringLiteral("energy"));
            executedCount++;
        }
    }

    // 2. Normalizar áudio
    if (m_optNormalizeAudio) {
        setProgress(0.6);
        McpController *mcp = m_app->mcp();
        if (mcp) {
            mcp->applyNormalizeGain(m_targetTrack, m_targetClip, m_optTargetLufs, -1.0, true);
            executedCount++;
        }
    }

    // 3. Transcrever legendas
    if (m_optTranscribe) {
        setProgress(0.8);
        m_app->generateSubtitlesForClip(m_targetTrack, m_targetClip, m_optLanguage);
        executedCount++;
    }

    // 4. Aplicar transições
    if (m_optApplyTransitions) {
        setProgress(0.9);
        // Add transition if the clip has a touching partner
        if (m_app->hasTransitionPartner(m_targetTrack, m_targetClip)) {
            m_app->addTransition(m_targetTrack, m_targetClip, m_optTransitionKind, m_optTransitionDuration);
            executedCount++;
        }
    }

    setProgress(1.0);
    setBusy(false);
    clearPlan();

    const QString summary = tr("Edição automática concluída com sucesso (%1 operações aplicadas).").arg(executedCount);
    emit autoEditCompleted(summary);
}

void AutoEditService::rejectPlan()
{
    clearPlan();
}

void AutoEditService::cancel()
{
    m_cancelled = true;
    setBusy(false, tr("Operação cancelada pelo usuário"));
}

bool AutoEditService::exportSubtitlesToSrt(const QString &targetFilePath)
{
    if (!m_app || targetFilePath.isEmpty())
        return false;

    SubtitleController *subs = m_app->subtitles();
    if (!subs)
        return false;

    const int track = m_app->activeTrackIndex();
    const int clip = m_app->activeClipIndex();
    if (track < 0 || clip < 0)
        return false;

    const QString srtContent = subs->cuesText(track, clip, QStringLiteral("srt"), true);
    if (srtContent.trimmed().isEmpty())
        return false;

    QFile file(targetFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << srtContent;
    file.close();
    return true;
}

bool AutoEditService::applyDirectCut(int trackIndex, int clipIndex, double atSeconds)
{
    if (!m_app)
        return false;
    McpController *mcp = m_app->mcp();
    if (!mcp)
        return false;

    const QJsonObject res = mcp->dispatcher()->apply(QJsonObject{
        {QStringLiteral("name"), QStringLiteral("split_clip")},
        {QStringLiteral("arguments"), QJsonObject{
            {QStringLiteral("track"), trackIndex},
            {QStringLiteral("clip"), clipIndex},
            {QStringLiteral("at"), atSeconds}
        }}
    });

    return res.value(QStringLiteral("ok")).toBool(false);
}

bool AutoEditService::applyTransitionBetweenClips(int trackIndex, int leftClipIndex, const QString &kind, double durationSec)
{
    if (!m_app)
        return false;
    if (trackIndex < 0 || leftClipIndex < 0)
        return false;

    m_app->addTransition(trackIndex, leftClipIndex, kind, durationSec);
    return true;
}

bool AutoEditService::normalizeClipAudio(int trackIndex, int clipIndex, double targetLufs)
{
    if (!m_app)
        return false;
    McpController *mcp = m_app->mcp();
    if (!mcp)
        return false;

    const QJsonObject res = mcp->applyNormalizeGain(trackIndex, clipIndex, targetLufs, -1.0, true);
    return res.value(QStringLiteral("ok")).toBool(false);
}

} // namespace drift::services
