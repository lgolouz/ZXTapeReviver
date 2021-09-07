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
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.15

Dialog {
    id: aboutDialog

    visible: false
    title: "About..."
    standardButtons: StandardButton.Ok
    modality: Qt.WindowModal
    width: 300
    height: 140

    Text {
        id: zxTapeReviverText
        text: 'ZX Tape Reviver (c) 2020-2021 <a href="mailto:lgolouz@list.ru">Leonid Golouz</a>'
        onLinkActivated: Qt.openUrlExternally(link)
    }
    Text {
        id: skipText
        anchors.top: zxTapeReviverText.bottom
        text: ""
    }
    Text {
        id: emailText
        anchors.top: skipText.bottom
        text: 'E-mail: <a href="mailto:computerenthusiasttips@mail.ru">computerenthusiasttips@mail.ru</a>'
        onLinkActivated: Qt.openUrlExternally(link)
    }
    Text {
        id: youtubeText
        anchors.top: emailText.bottom
        text: 'YouTube channel: <a href="https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg">Советы компьютерного энтузиаста</a> - <a href="https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg">Computer Enthusiast Tips</a>'
        onLinkActivated: Qt.openUrlExternally(link)
    }
    Text {
        id: skip2Text
        anchors.top: youtubeText.bottom
        text: ""
    }
    Text {
        anchors.top: skip2Text.bottom
        text: "Please click the highlighted links to open"
    }
}
