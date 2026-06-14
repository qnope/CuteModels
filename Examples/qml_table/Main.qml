import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 820
    height: 480
    title: "CuteModel — BasicTableModel (QML)"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                Layout.fillWidth: false
                Layout.fillHeight: true
                Layout.preferredWidth: 170
                Layout.maximumWidth: 170
                Label { text: "Filter" }
                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Filter…"
                    onTextChanged: controller.setFilter(text)
                }
                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 220
                Label { text: "View" }
                TableView {
                    id: mainView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: controller.viewModel
                    selectionModel: controller.selectionModel
                    delegate: Rectangle {
                        required property int row
                        required property int column
                        implicitWidth: 160
                        implicitHeight: 36
                        border.width: 1
                        border.color: "#cccccc"
                        color: "white"
                        Text {
                            anchors.fill: parent
                            anchors.margins: 4
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            text: model.name
                        }
                        TapHandler {
                            onTapped: controller.selectIndex(mainView.modelIndex(Qt.point(column, row)))
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: false
                Layout.fillHeight: true
                Layout.preferredWidth: 220
                Layout.maximumWidth: 220
                Label { text: "Selected" }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: controller.selectedSummaries
                    delegate: Label { text: modelData }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            padding: 8
            background: Rectangle { border.width: 1; border.color: "#cccccc" }
            text: "Ref: " + (controller.currentSummary.length > 0 ? controller.currentSummary : "—")
        }
    }
}
