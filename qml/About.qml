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

import com.models.zxtapereviver 1.0
import "."

Dialog {
    id: aboutDialog

    visible: false
    title: Translations.id_about_window_header
    standardButtons: StandardButton.Ok
    modality: Qt.WindowModal
    width: 300
    height: 140

    Text {
        id: zxTapeReviverText
        text: '<b>ZX Tape Reviver</b> <i>%1</i> (c) 2020-2021 <a href="mailto:lgolouz@list.ru">Leonid Golouz</a>'.arg(ConfigurationManager.zxTapeReviverVersion)
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
        text: Translations.id_email_link
        onLinkActivated: Qt.openUrlExternally(link)
    }
    Text {
        id: youtubeText
        anchors.top: emailText.bottom
        text: Translations.id_youtube_channel_link
        onLinkActivated: Qt.openUrlExternally(link)
    }
    Text {
        id: donationText
        anchors.top: youtubeText.bottom
        text: Translations.id_donations_link
        onLinkActivated: Qt.openUrlExternally(link)
    }

    Text {
        id: skip2Text
        anchors.top: donationText.bottom
        text: ""
    }
    Text {
        anchors.top: skip2Text.bottom
        text: Translations.id_please_click_to_open_link
    }
}
