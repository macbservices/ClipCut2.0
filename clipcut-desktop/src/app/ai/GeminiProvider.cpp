#include "app/ai/GeminiProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>

namespace drift::ai {

GeminiProvider::GeminiProvider(QObject *parent)
    : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(30000); // 30s timeout
    connect(&m_timeoutTimer, &QTimer::timeout, this, &GeminiProvider::onRequestTimeout);

    // Initial check from environment variable (without hardcoding)
    const QByteArray envKey = qgetenv("GEMINI_API_KEY");
    if (!envKey.isEmpty()) {
        m_apiKey = QString::fromUtf8(envKey).trimmed();
    }
}

GeminiProvider::~GeminiProvider()
{
    cancelRequest();
}

bool GeminiProvider::hasApiKey() const
{
    return !m_apiKey.trimmed().isEmpty();
}

void GeminiProvider::setModelName(const QString &model)
{
    if (m_modelName == model || model.trimmed().isEmpty())
        return;
    m_modelName = model.trimmed();
    emit modelNameChanged();
}

void GeminiProvider::setApiKey(const QString &key)
{
    const QString cleaned = key.trimmed();
    if (m_apiKey == cleaned)
        return;
    m_apiKey = cleaned;
    emit apiKeyChanged();
}

QString GeminiProvider::getMaskedApiKey() const
{
    if (m_apiKey.isEmpty())
        return QString();
    if (m_apiKey.length() <= 8)
        return QStringLiteral("••••••••");
    return m_apiKey.left(4) + QStringLiteral("••••••••") + m_apiKey.right(4);
}

void GeminiProvider::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void GeminiProvider::cancelRequest()
{
    m_timeoutTimer.stop();
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    setBusy(false);
}

void GeminiProvider::onRequestTimeout()
{
    if (m_currentReply) {
        m_currentReply->abort();
    }
    m_lastError = {408, tr("Tempo limite de conexão esgotado (30s) ao contatar a API do Gemini."), false, true};
    emit errorOccurred(m_lastError.message, 408);
    setBusy(false);
}

QString GeminiProvider::buildSystemInstruction() const
{
    return QStringLiteral(
        "Você é o assistente inteligente de edição audiovisual do Clip Cut 2.0.\n"
        "Seu papel é interpretar instruções em linguagem natural e convertê-las em um plano estruturado de edição em JSON.\n"
        "O plano deve utilizar estritamente as capacidades nativas do editor:\n"
        "- split_clip: divide um clipe em determinada coordenada temporal (at em segundos).\n"
        "- delete_clip: remove um clipe pelo índice.\n"
        "- remove_silence: detecta e remove trechos de silêncio (thresholdDb, minDuration, padding).\n"
        "- normalize_volume: normaliza o áudio para um volume LUFS alvo (padrão -14.0).\n"
        "- add_subtitles: gera e insere legendas sincronizadas (idioma pt-BR).\n"
        "- add_transition: aplica transição (crossfade, dissolve) entre clipes adjacentes.\n"
        "- export_video: configura parâmetros de exportação (formato, proporção 16:9, 9:16 ou 1:1).\n\n"
        "Responda EXCLUSIVAMENTE em formato JSON estrito, sem markdown ao redor, seguindo o esquema:\n"
        "{\n"
        "  \"schema_version\": \"2.0\",\n"
        "  \"request_id\": \"<uuid>\",\n"
        "  \"project_id\": \"<project_id>\",\n"
        "  \"explanation\": \"<explicação amigável em português do que será executado>\",\n"
        "  \"requires_confirmation\": true,\n"
        "  \"actions\": [\n"
        "    {\n"
        "      \"action\": \"<nome_da_ação>\",\n"
        "      \"track\": 0,\n"
        "      \"clip\": 0,\n"
        "      \"parameters\": { ... }\n"
        "    }\n"
        "  ]\n"
        "}"
    );
}

void GeminiProvider::requestEditPlan(const QJsonObject &contextPayload, const QString &userInstruction)
{
    if (m_busy) {
        cancelRequest();
    }

    if (!hasApiKey()) {
        m_lastError = {401, tr("Chave de API do Gemini não configurada. Insira sua chave ou utilize a automação local da Fase 3."), false, false};
        emit errorOccurred(m_lastError.message, 401);
        return;
    }

    m_currentRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    setBusy(true);

    const QString urlStr = QString::asprintf(
        "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
        qPrintable(m_modelName), qPrintable(m_apiKey)
    );

    QNetworkRequest request{QUrl(urlStr)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    // Prepare contents
    const QString promptContent = QStringLiteral("Contexto do projeto:\n") +
                                  QString::fromUtf8(QJsonDocument(contextPayload).toJson(QJsonDocument::Compact)) +
                                  QStringLiteral("\n\nInstrução do usuário: ") + userInstruction;

    QJsonObject userContent;
    userContent[QStringLiteral("role")] = QStringLiteral("user");
    QJsonArray userParts;
    QJsonObject partText;
    partText[QStringLiteral("text")] = promptContent;
    userParts.append(partText);
    userContent[QStringLiteral("parts")] = userParts;

    QJsonObject systemPart;
    systemPart[QStringLiteral("text")] = buildSystemInstruction();
    QJsonObject systemInstructionObj;
    QJsonArray sysParts;
    sysParts.append(systemPart);
    systemInstructionObj[QStringLiteral("parts")] = sysParts;

    QJsonObject generationConfig;
    generationConfig[QStringLiteral("responseMimeType")] = QStringLiteral("application/json");

    QJsonObject rootPayload;
    QJsonArray contentsArray;
    contentsArray.append(userContent);
    rootPayload[QStringLiteral("contents")] = contentsArray;
    rootPayload[QStringLiteral("systemInstruction")] = systemInstructionObj;
    rootPayload[QStringLiteral("generationConfig")] = generationConfig;

    const QByteArray postData = QJsonDocument(rootPayload).toJson(QJsonDocument::Compact);

    m_currentReply = m_net.post(request, postData);
    connect(m_currentReply, &QNetworkReply::finished, this, &GeminiProvider::onReplyFinished);
    m_timeoutTimer.start();
}

void GeminiProvider::onReplyFinished()
{
    m_timeoutTimer.stop();
    if (!m_currentReply) {
        setBusy(false);
        return;
    }

    const int statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray responseBytes = m_currentReply->readAll();
    const QNetworkReply::NetworkError netErr = m_currentReply->error();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    setBusy(false);

    if (netErr != QNetworkReply::NoError || statusCode != 200) {
        GeminiError err;
        err.httpStatusCode = statusCode;

        if (statusCode == 429) {
            err.isQuotaExceeded = true;
            err.message = tr("Limite de cota ou requisições excedido na API do Gemini (HTTP 429). "
                             "O editor continuará operando normalmente através do motor de automação local da Fase 3.");
            m_lastError = err;
            emit quotaExceeded();
            emit errorOccurred(err.message, 429);
            return;
        }

        if (statusCode == 400 || statusCode == 401 || statusCode == 403) {
            err.message = tr("Falha de autenticação na API do Gemini (HTTP %1). Verifique se a chave de API é válida.").arg(statusCode);
        } else if (netErr == QNetworkReply::OperationCanceledError) {
            err.message = tr("Operação cancelada pelo usuário.");
        } else {
            err.isNetworkFailure = true;
            err.message = tr("Erro de comunicação com a API do Gemini (HTTP %1 / Código %2). Verifique sua conexão de rede.").arg(statusCode).arg(netErr);
        }

        m_lastError = err;
        emit errorOccurred(err.message, statusCode);
        return;
    }

    // Success 200 OK: Parse candidate JSON
    const QJsonDocument doc = QJsonDocument::fromJson(responseBytes);
    if (!doc.isObject()) {
        m_lastError = {200, tr("Resposta da API em formato JSON inválido"), false, false};
        emit errorOccurred(m_lastError.message, 200);
        return;
    }

    const QJsonObject obj = doc.object();
    const QJsonArray candidates = obj.value(QStringLiteral("candidates")).toArray();
    if (candidates.isEmpty()) {
        m_lastError = {200, tr("Nenhuma resposta gerada pelo modelo Gemini"), false, false};
        emit errorOccurred(m_lastError.message, 200);
        return;
    }

    const QJsonObject cand = candidates.first().toObject();
    const QJsonObject content = cand.value(QStringLiteral("content")).toObject();
    const QJsonArray parts = content.value(QStringLiteral("parts")).toArray();
    if (parts.isEmpty()) {
        m_lastError = {200, tr("Conteúdo vazio retornado pelo Gemini"), false, false};
        emit errorOccurred(m_lastError.message, 200);
        return;
    }

    const QString rawText = parts.first().toObject().value(QStringLiteral("text")).toString();
    emit rawTextResponseReceived(rawText);

    const QJsonObject plan = parsePlanFromResponse(rawText);
    if (plan.isEmpty() || !plan.contains(QStringLiteral("actions"))) {
        m_lastError = {200, tr("O plano retornado não contém ações válidas para a timeline"), false, false};
        emit errorOccurred(m_lastError.message, 200);
        return;
    }

    emit planReceived(plan, m_currentRequestId);
}

QJsonObject GeminiProvider::parsePlanFromResponse(const QString &responseText) const
{
    QString cleaned = responseText.trimmed();
    if (cleaned.startsWith(QLatin1String("```json"))) {
        cleaned = cleaned.mid(7);
        if (cleaned.endsWith(QLatin1String("```"))) {
            cleaned.chop(3);
        }
        cleaned = cleaned.trimmed();
    } else if (cleaned.startsWith(QLatin1String("```"))) {
        cleaned = cleaned.mid(3);
        if (cleaned.endsWith(QLatin1String("```"))) {
            cleaned.chop(3);
        }
        cleaned = cleaned.trimmed();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(cleaned.toUtf8());
    if (doc.isObject()) {
        return doc.object();
    }

    return QJsonObject();
}

} // namespace drift::ai
