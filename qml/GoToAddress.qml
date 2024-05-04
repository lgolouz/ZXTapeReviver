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

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Window {
    id: gotoAddressDialog

    signal gotoAddress(int adr);

    visible: false
    title: Translations.id_goto_address_window_header
    //standardButtons: StandardButton.Ok | StandardButton.Cancel
    modality: Qt.WindowModal
    width: containerItem.childrenRect.width + 2 * margin_size
    height: containerItem.childrenRect.height + 2 * margin_size

    readonly property int margin_size: 10

    Item {
        id: containerItem
        anchors {
            fill: parent
            topMargin: gotoAddressDialog.margin_size
            leftMargin: gotoAddressDialog.margin_size
        }

        Text {
            id: textWithField
            text: Translations.id_please_enter_address
        }

        TextField {
            id: textField
            anchors.top: textWithField.bottom
            width: 250
        }

        CheckBox {
            id: hexCheckbox

            anchors.top: textField.bottom
            anchors.topMargin: 3
            checked: true
            text: Translations.id_hexadecimal
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

        Button {
            id: okButton
            anchors.right: textField.right
            anchors.top: conversionField.bottom
            anchors.topMargin: gotoAddressDialog.margin_size
            text: Translations.id_goto_button_text
            onClicked: {
                gotoAddress(parseInt(textField.text, hexCheckbox.checked ? 16 : 10));
                close();
            }
        }

        Button {
            anchors.right: okButton.left
            anchors.rightMargin: gotoAddressDialog.margin_size / 2
            anchors.top: conversionField.bottom
            anchors.topMargin: gotoAddressDialog.margin_size
            text: Translations.id_cancel_button_text
            onClicked: {
                close();
            }
        }
    }
}
