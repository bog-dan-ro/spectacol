import QtQuick

Item {
    property string text

    signal triggered()
    signal aboutToShow()
    signal desktopMenuClosed()

    function trigger() {
        if (enabled)
            triggered();
    }
}
