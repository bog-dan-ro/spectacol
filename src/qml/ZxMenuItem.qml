import QtQuick

Item {
    property string text

    signal triggered()
    signal aboutToShow()

    function trigger() {
        if (enabled)
            triggered();
    }
}
