#include "app/ai/EditCommandExecutor.h"
#include "app/editor/AppController.h"
#include "app/mcp/McpController.h"
#include "app/mcp/tools/McpDispatcher.h"
#include "core/project/Clip.h"
#include "core/project/Project.h"

namespace drift::ai {

EditCommandExecutor::EditCommandExecutor(AppController *app, QObject *parent)
    : QObject(parent), m_app(app)
{
}

EditCommandExecutor::~EditCommandExecutor() = default;

bool EditCommandExecutor::validatePlanSchema(const QJsonObject &planJson, QString *outError) const
{
    const QString version = planJson.value(QStringLiteral("schema_version")).toString();
    if (version != QLatin1String("2.0")) {
        if (outError)
            *outError = tr("Versão de esquema incompatível: esperado '2.0', recebido '%1'").arg(version);
        return false;
    }

    if (!planJson.contains(QStringLiteral("actions")) || !planJson.value(QStringLiteral("actions")).isArray()) {
        if (outError)
            *outError = tr("O plano deve conter um array 'actions'");
        return false;
    }

    const QJsonArray actions = planJson.value(QStringLiteral("actions")).toArray();
    if (actions.isEmpty()) {
        if (outError)
            *outError = tr("Array 'actions' está vazio");
        return false;
    }

    return true;
}

bool EditCommandExecutor::validateActionAgainstTimeline(const QJsonObject &actionObj, QString *outError) const
{
    if (!m_app) {
        if (outError) *outError = tr("AppController indisponível");
        return false;
    }

    const QString actionName = actionObj.value(QStringLiteral("action")).toString();
    const int trackIdx = actionObj.value(QStringLiteral("track")).toInt(-1);
    const int clipIdx = actionObj.value(QStringLiteral("clip")).toInt(-1);
    const QJsonObject params = actionObj.value(QStringLiteral("parameters")).toObject();

    // Export doesn't require clip selection, but validate aspect ratio if provided
    if (actionName == QLatin1String("export_video")) {
        const QString ar = params.value(QStringLiteral("aspectRatio")).toString(QStringLiteral("16:9"));
        const QStringList validAr = {QStringLiteral("16:9"), QStringLiteral("9:16"), QStringLiteral("1:1")};
        if (!validAr.contains(ar)) {
            if (outError)
                *outError = tr("Proporção de aspecto '%1' inválida. Suportadas: 16:9, 9:16, 1:1").arg(ar);
            return false;
        }
        return true;
    }

    // Known actions whitelist
    const QStringList allowedActions = {
        QStringLiteral("split_clip"),
        QStringLiteral("delete_clip"),
        QStringLiteral("move_clip"),
        QStringLiteral("remove_silence"),
        QStringLiteral("normalize_volume"),
        QStringLiteral("add_subtitles"),
        QStringLiteral("add_transition")
    };

    if (!allowedActions.contains(actionName)) {
        if (outError)
            *outError = tr("Ação desconhecida '%1' rejeitada por segurança").arg(actionName);
        return false;
    }

    // Boundary check for track
    const auto &tracks = m_app->m_project.tracks();
    if (trackIdx < 0 || trackIdx >= tracks.size()) {
        if (outError)
            *outError = tr("Trilha inexistente: índice %1 fora dos limites (total: %2)").arg(trackIdx).arg(tracks.size());
        return false;
    }

    const auto &clips = tracks.at(trackIdx).clips;
    if (clipIdx < 0 || clipIdx >= clips.size()) {
        if (outError)
            *outError = tr("Clipe inexistente: índice %1 fora dos limites da trilha %2 (total: %3)").arg(clipIdx).arg(trackIdx).arg(clips.size());
        return false;
    }

    const Clip &targetClip = clips.at(clipIdx);
    const double clipDurSec = drift::usToSeconds(targetClip.timelineDuration);

    // Specific validation for split_clip
    if (actionName == QLatin1String("split_clip")) {
        const double splitAt = params.value(QStringLiteral("at")).toDouble(-1.0);
        if (splitAt <= 0.0 || splitAt >= clipDurSec) {
            if (outError)
                *outError = tr("Intervalo temporal inválido: ponto de corte (%1 s) fora dos limites do clipe (duração: %2 s)")
                                .arg(splitAt, 0, 'f', 2).arg(clipDurSec, 0, 'f', 2);
            return false;
        }
    }

    return true;
}

ExecutionResult EditCommandExecutor::executePlan(const QJsonObject &planJson)
{
    ExecutionResult result;
    if (!m_app) {
        result.errorDetails << tr("Controlador do editor indisponível");
        return result;
    }

    QString schemaErr;
    if (!validatePlanSchema(planJson, &schemaErr)) {
        result.errorDetails << schemaErr;
        return result;
    }

    const QJsonArray actions = planJson.value(QStringLiteral("actions")).toArray();
    emit executionStarted(actions.size());

    // 1. Dry-run validation of every action before mutating timeline
    for (int i = 0; i < actions.size(); ++i) {
        QString actionErr;
        const QJsonObject act = actions.at(i).toObject();
        if (!validateActionAgainstTimeline(act, &actionErr)) {
            result.errorDetails << tr("Ação %1 falhou na validação: %2").arg(i + 1).arg(actionErr);
            result.actionsFailed++;
        }
    }

    if (!result.errorDetails.isEmpty()) {
        result.ok = false;
        result.summary = tr("Execução cancelada: o plano contém parâmetros inválidos para o estado atual da timeline.");
        emit executionFinished(result);
        return result;
    }

    // 2. Take atomic snapshot for QUndoStack
    const drift::Project before = m_app->m_project.detachedCopy();
    const QString explanation = planJson.value(QStringLiteral("explanation")).toString(tr("Edição inteligente Gemini"));

    McpController *mcp = m_app->mcp();
    int executed = 0;

    for (int i = 0; i < actions.size(); ++i) {
        const QJsonObject act = actions.at(i).toObject();
        const QString name = act.value(QStringLiteral("action")).toString();
        const int track = act.value(QStringLiteral("track")).toInt();
        const int clip = act.value(QStringLiteral("clip")).toInt();
        const QJsonObject params = act.value(QStringLiteral("parameters")).toObject();

        bool stepOk = false;

        if (name == QLatin1String("split_clip")) {
            const double at = params.value(QStringLiteral("at")).toDouble();
            if (mcp && mcp->dispatcher()) {
                const QJsonObject res = mcp->dispatcher()->apply(QJsonObject{
                    {QStringLiteral("name"), QStringLiteral("split_clip")},
                    {QStringLiteral("arguments"), QJsonObject{
                        {QStringLiteral("track"), track},
                        {QStringLiteral("clip"), clip},
                        {QStringLiteral("at"), at}
                    }}
                });
                stepOk = res.value(QStringLiteral("ok")).toBool(false);
            }
        } else if (name == QLatin1String("delete_clip")) {
            m_app->deleteClip(track, clip);
            stepOk = true;
        } else if (name == QLatin1String("remove_silence")) {
            const double thresholdDb = params.value(QStringLiteral("thresholdDb")).toDouble(-30.0);
            const double minDuration = params.value(QStringLiteral("minDuration")).toDouble(0.5);
            const double padding = params.value(QStringLiteral("padding")).toDouble(0.1);
            const double linThresh = std::pow(10.0, thresholdDb / 20.0);
            if (mcp) {
                const QJsonObject res = mcp->removeSilence(track, clip, linThresh, minDuration, padding, QStringLiteral("energy"));
                stepOk = res.value(QStringLiteral("ok")).toBool(false);
            }
        } else if (name == QLatin1String("normalize_volume")) {
            const double targetLufs = params.value(QStringLiteral("targetLufs")).toDouble(-14.0);
            if (mcp) {
                const QJsonObject res = mcp->applyNormalizeGain(track, clip, targetLufs, -1.0, true);
                stepOk = res.value(QStringLiteral("ok")).toBool(false);
            }
        } else if (name == QLatin1String("add_subtitles")) {
            const QString lang = params.value(QStringLiteral("languageCode")).toString(QStringLiteral("pt"));
            m_app->generateSubtitlesForClip(track, clip, lang);
            stepOk = true;
        } else if (name == QLatin1String("add_transition")) {
            const QString kind = params.value(QStringLiteral("kind")).toString(QStringLiteral("crossfade"));
            const double duration = params.value(QStringLiteral("durationSec")).toDouble(0.5);
            m_app->addTransition(track, clip, kind, duration);
            stepOk = true;
        } else if (name == QLatin1String("export_video")) {
            const QString outputPath = params.value(QStringLiteral("outputPath")).toString();
            const QString presetId = params.value(QStringLiteral("presetId")).toString();
            const QString aspectRatio = params.value(QStringLiteral("aspectRatio")).toString(QStringLiteral("16:9"));

            QVariantMap settings;
            if (aspectRatio == QLatin1String("9:16")) {
                settings.insert(QStringLiteral("width"), 1080);
                settings.insert(QStringLiteral("height"), 1920);
            } else if (aspectRatio == QLatin1String("1:1")) {
                settings.insert(QStringLiteral("width"), 1080);
                settings.insert(QStringLiteral("height"), 1080);
            } else {
                settings.insert(QStringLiteral("width"), 1920);
                settings.insert(QStringLiteral("height"), 1080);
            }

            if (!outputPath.isEmpty()) {
                const QUrl outUrl = QUrl::fromLocalFile(outputPath);
                if (!presetId.isEmpty()) {
                    m_app->exportWithPreset(outUrl, presetId);
                } else {
                    m_app->exportWithSettings(outUrl, settings);
                }
            } else {
                m_app->exportProject(QUrl{});
            }
            stepOk = true;
        }

        if (stepOk) {
            executed++;
            emit actionExecuted(i, name, true);
        } else {
            result.actionsFailed++;
            result.errorDetails << tr("Ação %1 (%2) falhou na execução nativa").arg(i + 1).arg(name);
            emit actionExecuted(i, name, false);
        }
    }

    // 3. Push single undo command to QUndoStack
    if (executed > 0) {
        m_app->pushProjectEdit(before, QStringLiteral("Gemini: ") + explanation);
        m_app->finishEdit(QStringLiteral("Gemini: ") + explanation);
    }

    result.ok = (result.actionsFailed == 0);
    result.actionsExecuted = executed;
    result.summary = tr("Plano executado com sucesso: %1 operações aplicadas e salvas no histórico.").arg(executed);

    emit executionFinished(result);
    return result;
}

} // namespace drift::ai
