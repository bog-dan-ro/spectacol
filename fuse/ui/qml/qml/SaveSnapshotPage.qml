// @scope main.qml

import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

SaveFilePage {
    id: saveSnapshotPage
    title: qsTr("Snapshot name:")
    folder: fuse.dataPath + "/Snapshots/"
    onAccpeted: {
        pageLoader.source = "";
    }
    onCanceled: pageLoader.source = "";

    Settings {
        category: "snapshot"
        property alias folder: saveSnapshotPage.folder
    }
}
