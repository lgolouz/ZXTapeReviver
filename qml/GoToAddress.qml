//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
// YouTube channel: https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg
// YouTube channel e-mail: computerenthusiasttips@mail.ru
//
// Code modification and distribution of any kind is not allowed without direct
// permission of the Author.
//*******************************************************************************

import QtQuick 2.3
import QtQuick.Controls
import QtQuick.Dialogs

Dialog {
    id: gotoAddressDialog

    signal gotoAddress(int adr);

    visible: false
    title: "Go to address..."
    standardButtons: StandardButton.Ok | StandardButton.Cancel
    //modality: Qt.WindowModal
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
