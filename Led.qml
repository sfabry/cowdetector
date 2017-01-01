import QtQuick 2.0

Rectangle {
    id: root
    property bool on: false
    width: 30
    height: width
    radius: width / 2
    border.width: 2
    border.color: "black"
    color: root.on ? "red" : "grey"
    Behavior on color {
        ColorAnimation {
            duration: 200
        }
    }
}
