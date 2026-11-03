pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Drift

PanelFrame {
    id: root
    border.color: Theme.c.propertiesPanel.border
    border.width: Theme.c.propertiesPanel.borderWidth
    radius: Theme.c.propertiesPanel.radius
    color: "transparent"

    readonly property var aiBridge: EditorState.aiBridge
    readonly property var autoEdit: EditorState.autoEditService
    readonly property var silenceSvc: EditorState.silenceService

    readonly property bool isBusy: (aiBridge && aiBridge.busy) || 
                                   (autoEdit && autoEdit.busy) || 
                                   (silenceSvc && silenceSvc.busy)

    property bool showKeyConfig: false

    ThemeSlot {
        slot: "AiAssistantPanelBackground"
        control: root
        anchors.fill: parent
        anchors.margins: root.border.width
        z: -1
        fallback: Component {
            Rectangle {
                color: Theme.c.propertiesPanel.background
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("Assistente Gemini & IA")
                font.bold: true
                font.pixelSize: 13
                color: Theme.c.appWindow.paletteText
            }

            Item { Layout.fillWidth: true }

            // Status chip
            Rectangle {
                radius: 10
                height: 20
                width: statusChipText.implicitWidth + 14
                color: root.isBusy 
                       ? Theme.c.accentColor 
                       : (aiBridge && aiBridge.hasApiKey && !aiBridge.useLocalFallback ? "#10B981" : "#64748B")

                Text {
                    id: statusChipText
                    anchors.centerIn: parent
                    text: root.isBusy 
                          ? qsTr("Pensando…") 
                          : (aiBridge && aiBridge.hasApiKey && !aiBridge.useLocalFallback ? qsTr("Gemini Online") : qsTr("Motor Local"))
                    font.pixelSize: 10
                    font.bold: true
                    color: "#FFFFFF"
                }
            }

            // Config API Key button
            Button {
                text: "⚙"
                font.pixelSize: 12
                implicitWidth: 26
                implicitHeight: 24
                onClicked: root.showKeyConfig = !root.showKeyConfig
            }
        }

        // API Key configuration dropdown drawer
        Rectangle {
            Layout.fillWidth: true
            visible: root.showKeyConfig
            radius: 6
            color: "#181B20"
            border.color: "#3B4252"
            border.width: 1
            implicitHeight: keyConfigCol.implicitHeight + 16

            ColumnLayout {
                id: keyConfigCol
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Text {
                    text: qsTr("Configuração da API do Gemini:")
                    font.bold: true
                    font.pixelSize: 11
                    color: "#88C0D0"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    TextField {
                        id: keyInputField
                        Layout.fillWidth: true
                        placeholderText: aiBridge && aiBridge.hasApiKey ? aiBridge.maskedApiKey : qsTr("Cole sua chave de API (GEMINI_API_KEY)…")
                        echoMode: TextInput.Password
                    }

                    Button {
                        text: qsTr("Salvar")
                        enabled: keyInputField.text.trim().length > 0
                        onClicked: {
                            if (aiBridge) {
                                aiBridge.setApiKey(keyInputField.text.trim())
                                keyInputField.text = ""
                                root.showKeyConfig = false
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        text: qsTr("Forçar modo local (sem gastar requisições de API)")
                        checked: aiBridge ? aiBridge.useLocalFallback : false
                        onToggled: if (aiBridge) aiBridge.useLocalFallback = checked
                    }
                }
            }
        }

        // Live Processing banner
        Rectangle {
            Layout.fillWidth: true
            visible: root.isBusy
            radius: 6
            color: "#1E293B"
            border.color: "#38BDF8"
            border.width: 1
            implicitHeight: busyRow.implicitHeight + 12

            RowLayout {
                id: busyRow
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                ProgressBar {
                    Layout.fillWidth: true
                    indeterminate: true
                }

                Text {
                    text: aiBridge ? aiBridge.statusText : qsTr("Processando…")
                    font.pixelSize: 11
                    color: "#38BDF8"
                }

                Button {
                    text: qsTr("Cancelar")
                    font.pixelSize: 10
                    onClicked: {
                        if (aiBridge) aiBridge.cancelCurrentOperation()
                        if (autoEdit) autoEdit.cancel()
                    }
                }
            }
        }

        // Conversation history list
        ScrollView {
            id: chatScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: chatListView
                width: parent.width
                spacing: 8
                model: aiBridge ? aiBridge.conversationHistory : []

                delegate: Rectangle {
                    width: chatListView.width
                    radius: 8
                    color: modelData.sender === "user" ? "#1E3A8A" : (modelData.sender === "system" ? "#1A202C" : "#181B20")
                    border.color: modelData.sender === "user" ? "#3B82F6" : (modelData.sender === "system" ? "#4A5568" : "#2E3440")
                    border.width: 1
                    implicitHeight: msgCol.implicitHeight + 14

                    ColumnLayout {
                        id: msgCol
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: modelData.sender === "user" ? qsTr("Você") : (modelData.sender === "gemini" ? qsTr("Gemini") : qsTr("Sistema"))
                                font.bold: true
                                font.pixelSize: 10
                                color: modelData.sender === "user" ? "#93C5FD" : (modelData.sender === "gemini" ? "#86EFAC" : "#CBD5E1")
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: modelData.timestamp || ""
                                font.pixelSize: 9
                                color: "#64748B"
                            }
                        }

                        Text {
                            text: modelData.text || ""
                            font.pixelSize: 11
                            color: "#F1F5F9"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }

                onCountChanged: {
                    Qt.callLater(function() {
                        chatListView.positionViewAtEnd()
                    })
                }
            }
        }

        // Plan Review Card (When Gemini or local engine presents a plan)
        Rectangle {
            Layout.fillWidth: true
            visible: aiBridge && aiBridge.hasPendingPlan
            radius: 8
            color: "#1E222B"
            border.color: "#10B981"
            border.width: 1.5
            implicitHeight: planCol.implicitHeight + 16

            ColumnLayout {
                id: planCol
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("📋 Plano Proposto pelo Gemini")
                        font.bold: true
                        font.pixelSize: 12
                        color: "#10B981"
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: qsTr("Requer Confirmação")
                        font.pixelSize: 10
                        color: "#F59E0B"
                    }
                }

                Text {
                    text: aiBridge ? aiBridge.planExplanation : ""
                    font.pixelSize: 11
                    color: "#E2E8F0"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        text: qsTr("✓ Aprovar e Executar")
                        Layout.fillWidth: true
                        onClicked: {
                            if (aiBridge) aiBridge.executePlan()
                        }
                    }

                    Button {
                        text: qsTr("✕ Descartar")
                        Layout.fillWidth: true
                        onClicked: {
                            if (aiBridge) aiBridge.rejectPlan()
                        }
                    }
                }
            }
        }

        // Quick Suggestions
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                text: qsTr("✂️ Cortar silêncios")
                font.pixelSize: 10
                onClicked: promptInput.text = qsTr("Remova os silêncios deste clipe mantendo a sincronia de áudio e vídeo.")
            }

            Button {
                text: qsTr("📝 Gerar legendas pt-BR")
                font.pixelSize: 10
                onClicked: promptInput.text = qsTr("Transcreva o áudio e crie legendas automáticas em português.")
            }

            Button {
                text: qsTr("🔊 -14 LUFS")
                font.pixelSize: 10
                onClicked: promptInput.text = qsTr("Normalize o áudio do clipe selecionado para -14 LUFS.")
            }
        }

        // Chat Input Row
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            TextField {
                id: promptInput
                Layout.fillWidth: true
                placeholderText: qsTr("Peça algo ao Gemini (ex: 'divida o clipe no segundo 2.5')…")
                enabled: !root.isBusy
                onAccepted: sendButton.clicked()
            }

            Button {
                id: sendButton
                text: qsTr("Enviar")
                enabled: promptInput.text.trim().length > 0 && !root.isBusy
                onClicked: {
                    if (aiBridge) {
                        aiBridge.sendMessage(promptInput.text.trim())
                        promptInput.text = ""
                    }
                }
            }
        }
    }
}
