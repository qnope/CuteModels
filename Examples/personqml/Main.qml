import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 640
    height: 440
    visible: true
    title: qsTr("BasicListModel — QML")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                TextField {
                    id: filterField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Filter by name...")
                    onTextChanged: controller.filterText = text
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 0

                    ListView {
                        id: listView
                        anchors.fill: parent
                        clip: true
                        model: controller.listModel
                        delegate: ItemDelegate {
                            id: rowDelegate
                            required property int index
                            required property string name
                            required property int age
                            width: ListView.view.width
                            text: name + " (" + age + ")"
                            property bool rowSelected: controller.selectedRows.indexOf(index) >= 0
                            highlighted: rowSelected
                            font.bold: controller.currentRow === index
                            background: Rectangle {
                                color: rowDelegate.rowSelected
                                       ? rowDelegate.palette.highlight
                                       : (rowDelegate.hovered ? rowDelegate.palette.alternateBase
                                                              : "transparent")
                            }
                            onClicked: controller.selectRow(index)
                        }
                    }
                }
            }

            Frame {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                padding: 0

                ListView {
                    id: selectionView
                    anchors.fill: parent
                    clip: true
                    model: controller.selectionListModel
                    delegate: Label {
                        required property string name
                        required property string email
                        width: ListView.view.width
                        padding: 6
                        text: name + " — " + email
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true

            GridLayout {
                anchors.fill: parent
                columns: 2
                columnSpacing: 12

                Label { text: qsTr("Current element"); font.bold: true; Layout.columnSpan: 2 }

                Label { text: qsTr("Name:") }
                Label { text: controller.currentRef && controller.currentRef.valid ? controller.currentRef.name : "—" }

                Label { text: qsTr("Age:") }
                Label { text: controller.currentRef && controller.currentRef.valid ? controller.currentRef.age : "—" }

                Label { text: qsTr("Email:") }
                Label { text: controller.currentRef && controller.currentRef.valid ? controller.currentRef.email : "—" }
            }
        }
    }
}
