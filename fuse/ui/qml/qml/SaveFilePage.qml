import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

Rectangle {
    id: saveFilePage
    color: Qt.rgba(0, 0, 0, 0.75);

    property alias title: title.text
    property alias fileName: fileNameEdit.text
    property alias folder: pathEdit.text
    signal accpeted
    signal canceled

    GridLayout {
        columns: 2
        anchors.centerIn: parent

        FancyText {
            id: title
            Layout.alignment: Qt.AlignRight
        }

        FancyTextField {
            id: fileNameEdit
            implicitWidth: saveFilePage.width / 2
            Layout.fillWidth: true
            placeholderText: qsTr("Enter file name")
        }

        FancyText {
            text: qsTr("Path:")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Layout.fillWidth: true
            FancyTextField {
                id: pathEdit
                Layout.fillWidth: true
                text: folder
                placeholderText: folder === "" ? qsTr("Enter file path") : ""
            }
            Button {
                FileDialog {
                    id: browseFolder
                    selectFolder: true
                    selectExisting: false
                    onAccepted: pathEdit.text = fileUrl
                    folder: pathEdit.text
                }
                text: qsTr("Browse")
                onClicked: browseFolder.open()
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Save")
            onClicked: accpeted()
        }
        Button {
            text: qsTr("Cancel")
            onClicked: canceled()
        }
    }
}
