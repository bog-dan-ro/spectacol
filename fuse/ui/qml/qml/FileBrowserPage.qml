// @scope main.qml

import QtQuick 2.0
import Qt.labs.settings 1.0

CoverFlowFiles {
    id: browser
    folder: fuse.dataPath

    Settings {
        category: "FileBrowser"
        property alias path: browser.folder
    }

    onFileSelected: {
        if (filePath)
            fuse.load(filePath);
        pageLoader.source = "";
    }
}
