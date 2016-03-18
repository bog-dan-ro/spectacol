pragma Singleton
import Qt.labs.controls 1.0

Label {
    id: _label
    property alias scale12: _label.font.pixelSize
    property double scale14: scale12 * 14 / 12
    property double scale16: scale12 * 16 / 12
    property double scale20: scale12 * 20 / 12
    property double scale24: scale12 * 2 // 24 / 12
    property double scale34: scale12 * 20 / 12
}
