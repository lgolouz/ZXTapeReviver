//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.3

Dialog {
    id: frequencyDialog

    property var frequency: 0

    visible: false
    title: "Measured frequency"
    standardButtons: StandardButton.Ok
    modality: Qt.WindowModal
    width: 200
    height: 120

    Text {
        id: textWithField
        text: "Measured frequency:"
    }

    TextField {
        id: textField
        anchors.top: textWithField.bottom
        anchors.topMargin: 5
        width: parent.width
        text: frequency
    }
}
