#include "app/ai/AiBridge.h"
#include "app/ai/EditCommandExecutor.h"
#include "app/ai/GeminiProvider.h"
#include "app/editor/AppController.h"
#include "app/mcp/McpController.h"
#include "core/project/Clip.h"
#include "core/project/Project.h"
#include "core/project/Track.h"
#include "engine/ml/speech/transcription/SileroVad.h"
#include "engine/ml/speech/transcription/WhisperTranscriber.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

namespace drift::ai {

AiBridge::AiBridge(AppController *controller, QObject *parent)
    : QObject(parent), m_app(controller)
{
    m_gemini = new GeminiProvider(this);
    m_executor = new EditCommandExecutor(m_app, this);

    connect(m_gemini, &GeminiProvider::busyChanged, this, &AiBridge::busyChanged);
    connect(m_gemini, &GeminiProvider::apiKeyChanged, this, &AiBridge::apiKeyChanged);
    connect(m_gemini, &GeminiProvider::planReceived, this, &AiBridge::onGeminiPlanReceived);
    connect(m_gemini, &GeminiProvider::errorOccurred, this, &AiBridge::onGeminiError);
    connect(m_gemini, &GeminiProvider::quotaExceeded, this, &AiBridge::onGeminiQuotaExceeded);

    // Initial greeting message in project conversation
    appendMessage(
        QStringLiteral("gemini"),
        tr("Olá! Sou o assistente de edição inteligente do Clip Cut 2.0.\n"
           "Posso analisar sua timeline, fatiar e remover silêncios, gerar legendas em português (pt-BR), "
           "normalizar o volume (-14 LUFS) e aplicar transições. Como posso ajudar com seu vídeo?")
    );
}

AiBridge::~AiBridge() = default;

bool AiBridge::vadAvailable() const
{
    return SileroVad::modelPresent();
}

bool AiBridge::whisperAvailable() const
{
    return WhisperTranscriber::modelPresent();
}

void AiBridge::setUseLocalFallback(bool fallback)
{
    if (m_useLocalFallback == fallback)
        return;
    m_useLocalFallback = fallback;
    emit fallbackChanged();
}

void AiBridge::setApiKey(const QString &key)
{
    if (m_gemini) {
        m_gemini->setApiKey(key);
    }
}

void AiBridge::setBusy(bool busy, const QString &status)
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

void AiBridge::setProgress(double fraction)
{
    m_progress = std::clamp(fraction, 0.0, 1.0);
    emit progressChanged();
}

void AiBridge::setError(const QString &error)
{
    m_lastError = error;
    emit errorOccurred(error);
}

void AiBridge::clearPlan()
{
    m_planExplanation.clear();
    m_planActions.clear();
    m_currentPlanJson = QJsonObject();
    m_hasPendingPlan = false;
    m_requiresConfirmation = false;
    emit planChanged();
}

void AiBridge::appendMessage(const QString &sender, const QString &text, bool hasPlan, const QJsonObject &plan)
{
    ConversationMessage msg;
    msg.sender = sender;
    msg.text = text;
    msg.timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"));
    msg.hasPlan = hasPlan;
    msg.plan = plan;

    m_messages.append(msg);

    m_conversationVariant.append(QVariantMap{
        {QStringLiteral("sender"), msg.sender},
        {QStringLiteral("text"), msg.text},
        {QStringLiteral("timestamp"), msg.timestamp},
        {QStringLiteral("hasPlan"), msg.hasPlan}
    });

    emit conversationChanged();
}

void AiBridge::clearConversation()
{
    m_messages.clear();
    m_conversationVariant.clear();
    appendMessage(QStringLiteral("gemini"), tr("Histórico limpo. Como posso ajudar?"));
}

QJsonObject AiBridge::collectProjectContext() const
{
    QJsonObject ctx;
    if (!m_app)
        return ctx;

    const drift::Project &proj = m_app->m_project;
    ctx[QStringLiteral("project_id")] = proj.id().isEmpty() ? QStringLiteral("proj-untitled") : proj.id();

    // Canvas
    QJsonObject canvasObj;
    canvasObj[QStringLiteral("width")] = proj.canvas().width();
    canvasObj[QStringLiteral("height")] = proj.canvas().height();
    canvasObj[QStringLiteral("fps")] = proj.canvas().fps();
    ctx[QStringLiteral("canvas")] = canvasObj;

    // Active selection
    const int actTrack = m_app->activeTrackIndex();
    const int actClip = m_app->activeClipIndex();
    ctx[QStringLiteral("active_track_index")] = actTrack;
    ctx[QStringLiteral("active_clip_index")] = actClip;

    // Tracks and clips
    QJsonArray tracksArray;
    for (int t = 0; t < proj.tracks().size(); ++t) {
        const drift::Track &track = proj.tracks().at(t);
        QJsonObject trackObj;
        trackObj[QStringLiteral("index")] = t;
        trackObj[QStringLiteral("id")] = track.id;
        trackObj[QStringLiteral("kind")] = track.kind == drift::Track::Kind::Video ? QStringLiteral("video") : (track.kind == drift::Track::Kind::Audio ? QStringLiteral("audio") : QStringLiteral("subtitles"));

        QJsonArray clipsArray;
        for (int c = 0; c < track.clips.size(); ++c) {
            const drift::Clip &clip = track.clips.at(c);
            QJsonObject clipObj;
            clipObj[QStringLiteral("index")] = c;
            clipObj[QStringLiteral("id")] = clip.id;
            clipObj[QStringLiteral("timeline_start_sec")] = drift::usToSeconds(clip.timelineStart);
            clipObj[QStringLiteral("timeline_duration_sec")] = drift::usToSeconds(clip.timelineDuration);
            clipObj[QStringLiteral("source_start_sec")] = drift::usToSeconds(clip.sourceStart);
            clipObj[QStringLiteral("source_duration_sec")] = drift::usToSeconds(clip.sourceDuration);
            clipObj[QStringLiteral("is_active")] = (t == actTrack && c == actClip);
            clipsArray.append(clipObj);
        }
        trackObj[QStringLiteral("clips")] = clipsArray;
        tracksArray.append(trackObj);
    }
    ctx[QStringLiteral("tracks")] = tracksArray;

    return ctx;
}

void AiBridge::sendMessage(const QString &text)
{
    const QString prompt = text.trimmed();
    if (prompt.isEmpty() || busy())
        return;

    // 1. Append user message to conversation history
    appendMessage(QStringLiteral("user"), prompt);

    // 2. Fallback check: if Gemini is disabled or has no key, use Phase 3 local analysis
    if (m_useLocalFallback || !hasApiKey()) {
        submitNaturalLanguagePrompt(prompt);
        return;
    }

    // 3. Send to GeminiProvider with minimal context
    setBusy(true, tr("Consultando raciocínio do Gemini…"));
    m_gemini->requestEditPlan(collectProjectContext(), prompt);
}

void AiBridge::onGeminiPlanReceived(const QJsonObject &planJson, const QString &requestId)
{
    Q_UNUSED(requestId)
    setBusy(false);

    QString schemaErr;
    if (!m_executor->validatePlanSchema(planJson, &schemaErr)) {
        appendMessage(QStringLiteral("gemini"), tr("Erro ao interpretar plano: %1").arg(schemaErr));
        return;
    }

    m_currentPlanJson = planJson;
    m_planExplanation = planJson.value(QStringLiteral("explanation")).toString();
    m_requiresConfirmation = planJson.value(QStringLiteral("requires_confirmation")).toBool(true);
    m_hasPendingPlan = true;

    m_planActions.clear();
    const QJsonArray actions = planJson.value(QStringLiteral("actions")).toArray();
    for (const auto &v : actions) {
        const QJsonObject act = v.toObject();
        m_planActions.append(QVariantMap{
            {QStringLiteral("action"), act.value(QStringLiteral("action")).toString()},
            {QStringLiteral("track"), act.value(QStringLiteral("track")).toInt()},
            {QStringLiteral("clip"), act.value(QStringLiteral("clip")).toInt()}
        });
    }

    emit planChanged();

    // Append to conversation
    appendMessage(
        QStringLiteral("gemini"),
        m_planExplanation + QStringLiteral("\n\n(Revise o plano abaixo e confirme a execução na timeline)"),
        true,
        planJson
    );
}

void AiBridge::onGeminiError(const QString &errorMessage, int statusCode)
{
    setBusy(false);
    setError(errorMessage);

    // Append system guidance in conversation
    appendMessage(
        QStringLiteral("system"),
        tr("⚠️ %1\nVocê pode continuar utilizando as ações rápidas do motor local (Cortar Silêncios, Legendas pt-BR, Normalização).").arg(errorMessage)
    );
}

void AiBridge::onGeminiQuotaExceeded()
{
    setUseLocalFallback(true);
}

void AiBridge::executePlan()
{
    if (!m_hasPendingPlan || !m_executor)
        return;

    setBusy(true, tr("Executando operações do plano na timeline…"));

    const ExecutionResult res = m_executor->executePlan(m_currentPlanJson);
    setBusy(false);

    if (res.ok) {
        appendMessage(QStringLiteral("system"), tr("✓ %1").arg(res.summary));
        emit operationFinished(res.summary);
    } else {
        const QString err = res.errorDetails.join(QLatin1Char('\n'));
        appendMessage(QStringLiteral("system"), tr("✕ Falha na execução:\n%1").arg(err));
        setError(res.summary);
    }

    clearPlan();
}

void AiBridge::rejectPlan()
{
    clearPlan();
    appendMessage(QStringLiteral("system"), tr("Plano de edição descartado. Nenhuma modificação foi aplicada à timeline."));
}

void AiBridge::cancelCurrentOperation()
{
    m_cancelled = true;
    if (m_gemini)
        m_gemini->cancelRequest();
    setBusy(false, tr("Operação cancelada"));
    appendMessage(QStringLiteral("system"), tr("Operação cancelada pelo usuário."));
}

// Local / Phase 3 fallbacks
void AiBridge::analyzeSilence(double threshold, double minDuration, double padding, const QString &method)
{
    if (!m_app || busy())
        return;

    const int track = m_app->activeTrackIndex();
    const int clip = m_app->activeClipIndex();
    if (track < 0 || clip < 0) {
        setError(tr("Selecione um clipe com áudio na timeline"));
        return;
    }

    setBusy(true, tr("Analisando trechos de silêncio (Motor Local)…"));
    clearPlan();

    McpController *mcp = m_app->mcp();
    if (!mcp) {
        setBusy(false);
        setError(tr("Controlador indisponível"));
        return;
    }

    const QJsonObject res = mcp->detectSilence(track, clip, 0, 0, threshold, minDuration, padding, method);
    setBusy(false);

    const QJsonArray ranges = res.value(QStringLiteral("ranges")).toArray();
    if (ranges.isEmpty()) {
        appendMessage(QStringLiteral("system"), tr("Nenhum silêncio significativo encontrado com o limiar configurado."));
        return;
    }

    double totalSec = 0.0;
    for (const auto &v : ranges) {
        const QJsonObject o = v.toObject();
        totalSec += (o.value(QStringLiteral("end")).toDouble() - o.value(QStringLiteral("start")).toDouble());
    }

    // Build structured 2.0 plan locally
    QJsonObject localPlan;
    localPlan[QStringLiteral("schema_version")] = QStringLiteral("2.0");
    localPlan[QStringLiteral("explanation")] = tr("Detectados %1 trechos de silêncio totalizando %2 s. Ação proposta: corte não-destrutivo na timeline com rampa anti-clique.")
                                                  .arg(ranges.size()).arg(totalSec, 0, 'f', 2);
    localPlan[QStringLiteral("requires_confirmation")] = true;

    QJsonArray actions;
    QJsonObject act;
    act[QStringLiteral("action")] = QStringLiteral("remove_silence");
    act[QStringLiteral("track")] = track;
    act[QStringLiteral("clip")] = clip;
    QJsonObject p;
    p[QStringLiteral("thresholdDb")] = -30.0;
    p[QStringLiteral("minDuration")] = minDuration;
    p[QStringLiteral("padding")] = padding;
    act[QStringLiteral("parameters")] = p;
    actions.append(act);
    localPlan[QStringLiteral("actions")] = actions;

    onGeminiPlanReceived(localPlan, QStringLiteral("local"));
}

void AiBridge::requestSilenceRemoval(double threshold, double minDuration, double padding, const QString &method)
{
    analyzeSilence(threshold, minDuration, padding, method);
}

void AiBridge::transcribeActiveClip(const QString &languageCode)
{
    if (!m_app || busy())
        return;

    const int track = m_app->activeTrackIndex();
    const int clip = m_app->activeClipIndex();
    if (track < 0 || clip < 0) {
        setError(tr("Selecione um clipe na timeline para transcrever"));
        return;
    }

    appendMessage(QStringLiteral("system"), tr("Iniciando transcrição local Whisper em português (pt-BR)…"));
    m_app->generateSubtitlesForClip(track, clip, languageCode);
}

void AiBridge::normalizeActiveClipAudio(double targetLufs)
{
    if (!m_app || busy())
        return;

    const int track = m_app->activeTrackIndex();
    const int clip = m_app->activeClipIndex();
    if (track < 0 || clip < 0) {
        setError(tr("Selecione um clipe para normalizar o volume"));
        return;
    }

    McpController *mcp = m_app->mcp();
    if (!mcp)
        return;

    const QJsonObject res = mcp->applyNormalizeGain(track, clip, targetLufs, -1.0, true);
    if (res.value(QStringLiteral("ok")).toBool(false)) {
        appendMessage(QStringLiteral("system"), tr("Áudio normalizado para %1 LUFS com sucesso.").arg(targetLufs, 0, 'f', 1));
    }
}

void AiBridge::submitNaturalLanguagePrompt(const QString &prompt)
{
    const QString lower = prompt.toLower();
    if (lower.contains(QLatin1String("silencio")) || lower.contains(QLatin1String("silêncio")) ||
        lower.contains(QLatin1String("corta silêncio")) || lower.contains(QLatin1String("remover silêncio"))) {
        analyzeSilence(0.02, 0.4, 0.1, QStringLiteral("energy"));
        return;
    }

    if (lower.contains(QLatin1String("legenda")) || lower.contains(QLatin1String("transcrever"))) {
        transcribeActiveClip(QStringLiteral("pt"));
        return;
    }

    if (lower.contains(QLatin1String("volume")) || lower.contains(QLatin1String("normalizar"))) {
        normalizeActiveClipAudio(-14.0);
        return;
    }

    // Generic response in chat
    appendMessage(
        QStringLiteral("gemini"),
        tr("Recebi sua instrução: \"%1\". Como a chave do Gemini não está configurada, você pode usar os botões de ação rápida abaixo para executar operações locais de áudio, silêncio e legendas.").arg(prompt)
    );
}

} // namespace drift::ai
