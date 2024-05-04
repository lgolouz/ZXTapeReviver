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
    id: frequencyDialog

    property real frequency: 0

    visible: false
    title: Translations.id_measured_frequency_window_header
    //standardButtons: StandardButton.Ok
    //modality: Qt.WindowModal
    width: componentsContainer.childrenRect.width + 2 * margin_size
    height: componentsContainer.childrenRect.height + 2 * margin_size

    readonly property int margin_size: 10

    Item {
        id: componentsContainer

        anchors {
            fill: parent
            topMargin: aboutDialog.margin_size
            leftMargin: aboutDialog.margin_size
        }

        Text {
            id: textWithField
            text: Translations.id_measured_frequency
        }

        TextField {
            id: textField
            anchors.top: textWithField.bottom
            anchors.topMargin: 5
            width: 200
            text: frequency
            readOnly: true
        }

        Button {
            anchors.right: textField.right
            anchors.top: textField.bottom
            anchors.topMargin: frequencyDialog.margin_size
            text: Translations.id_close_button_text

            onClicked: {
                close();
            }
        }
    }
}
