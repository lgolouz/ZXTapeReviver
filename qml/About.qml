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
import QtQuick.Window

import com.models.zxtapereviver 1.0
import "."

Window {
    id: aboutDialog

    visible: false
    title: Translations.id_about_window_header
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowCloseButtonHint
    modality: Qt.WindowModal

    width: 560
    height: 230
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height

    function show() {
        visible = true;
        raise();
        requestActivate();
    }

    Rectangle {
        anchors.fill: parent
        color: "#f7f7f7"

        Column {
            spacing: 8
            anchors {
                fill: parent
                margins: 14
            }

            Text {
                width: parent.width
                text: '<b>ZX Tape Reviver</b> <i>%1</i> (c) 2020-2024 Leonid Golouz'.arg(ConfigurationManager.zxTapeReviverVersion)
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
            }

            Text {
                width: parent.width
                text: Translations.id_email_link
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }

            Text {
                width: parent.width
                text: Translations.id_youtube_channel_link
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }

            Text {
                width: parent.width
                text: Translations.id_donations_link
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }

            Text {
                width: parent.width
                text: Translations.id_please_click_to_open_link
                color: "black"
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: 4
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Translations.id_close_button_text
                onClicked: aboutDialog.close()
            }
        }
    }
}
