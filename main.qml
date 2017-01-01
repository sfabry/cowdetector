import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0

Window {
    title: "Cow detector Debug"
    width: 400
    height: 600
    visible: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        RowLayout {
            Text { text: "Running GPIO : "; Layout.fillWidth: true }
            Led { on: runningGpio.on }
        }

        Text {
            text: "Box : " + box.name
            font.underline: true
            font.bold: true
        }

        RowLayout {
            Text { text: "Food distribution A : "; Layout.fillWidth: true }
            Led { on: box.food_A.on }
        }

        RowLayout {
            Text { text: "Food distribution B : "; Layout.fillWidth: true }
            Led { on: box.food_B.on }
        }

        RowLayout {
            Text { text: "Food calibration A : "; Layout.fillWidth: true }
            Button { id: calibButtonA; text: "Click to simulate" }
            Binding {
                target: box.calibrationButtonA
                property: "on"
                value: calibButtonA.pressed
            }
        }

        RowLayout {
            Text { text: "Food calibration B : "; Layout.fillWidth: true }
            Button { id: calibButtonB; text: "Click to simulate" }
            Binding {
                target: box.calibrationButtonB
                property: "on"
                value: calibButtonB.pressed
            }
        }

        RowLayout {
            TextField { id: rfidField }
            Item { Layout.fillWidth: true }
            Switch {
                onCheckedChanged: box.rfid.setDetected(checked ? rfidField.text : "")
            }
        }

        Item { Layout.fillHeight: true }
    }


}
