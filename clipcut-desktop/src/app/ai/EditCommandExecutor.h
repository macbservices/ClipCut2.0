#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

class AppController;

namespace drift::ai {

struct ExecutionResult {
    bool ok = false;
    int actionsExecuted = 0;
    int actionsFailed = 0;
    QStringList errorDetails;
    QString summary;
};

class EditCommandExecutor : public QObject
{
    Q_OBJECT

public:
    explicit EditCommandExecutor(AppController *app, QObject *parent = nullptr);
    ~EditCommandExecutor() override;

    // Schema and boundary validations
    bool validatePlanSchema(const QJsonObject &planJson, QString *outError = nullptr) const;
    bool validateActionAgainstTimeline(const QJsonObject &actionObj, QString *outError = nullptr) const;

    // Atomic execution
    ExecutionResult executePlan(const QJsonObject &planJson);

signals:
    void executionStarted(int totalActions);
    void actionExecuted(int index, const QString &actionName, bool success);
    void executionFinished(const ExecutionResult &result);

private:
    AppController *m_app = nullptr;
};

} // namespace drift::ai
