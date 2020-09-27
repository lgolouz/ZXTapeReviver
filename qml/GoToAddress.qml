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
    id: gotoAddressDialog

    signal gotoAddress(int adr);

    visible: false
    title: "Go to address..."
    standardButtons: StandardButton.Ok | StandardButton.Cancel
    modality: Qt.WindowModal
    width: 200
    height: 120

    Text {
        id: textWithField
        text: "Please enter address:"
    }

    TextField {
        id: textField
        anchors.top: textWithField.bottom
        width: parent.width
    }

    CheckBox {
        id: hexCheckbox

        anchors.top: textField.bottom
        anchors.topMargin: 3
        checked: true
        text: "Hexadecimal"
    }

    Text {
        id: conversionField

        function convertAddress(adr) {
            return hexCheckbox.checked ? parseInt(adr, 16) : "0x" + parseInt(adr, 10).toString(16).toUpperCase();
        }

        text: "(" + convertAddress(textField.text) + ")"
        anchors.left: hexCheckbox.right
        anchors.leftMargin: 3
        anchors.top: hexCheckbox.top
    }

    onAccepted: {
        gotoAddress(parseInt(textField.text, hexCheckbox.checked ? 16 : 10));
    }
}
