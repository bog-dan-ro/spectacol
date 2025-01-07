import QtQuick

Item {
    property string text

    signal triggered()

    function trigger() {
        triggered();
    }
}
