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

Dialog {
    id: frequencyDialog

    property var frequency: 0

    visible: false
    title: Translations.id_measured_frequency_window_header
    standardButtons: StandardButton.Ok
    //modality: Qt.WindowModal
    width: 200
    height: 120

    Text {
        id: textWithField
        text: Translations.id_measured_frequency
    }

    TextField {
        id: textField
        anchors.top: textWithField.bottom
        anchors.topMargin: 5
        width: parent.width
        text: frequency
    }
}
