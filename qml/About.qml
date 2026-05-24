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

    readonly property int contentMargin: 14
    readonly property string versionText: '<b>ZX Tape Reviver</b> <i>%1</i> (c) 2020-2024 Leonid Golouz'.arg(ConfigurationManager.zxTapeReviverVersion)
    readonly property int textWidth: Math.min(720, Math.max(420, Math.ceil(Math.max(versionTextMetrics.implicitWidth,
                                                                                      emailTextMetrics.implicitWidth,
                                                                                      youtubeTextMetrics.implicitWidth,
                                                                                      donationTextMetrics.implicitWidth,
                                                                                      hintTextMetrics.implicitWidth))))

    visible: false
    title: Translations.id_about_window_header
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowCloseButtonHint
    modality: Qt.WindowModal

    width: textWidth + contentMargin * 2
    height: aboutContent.implicitHeight + contentMargin * 2
    minimumWidth: 420
    minimumHeight: aboutContent.implicitHeight + contentMargin * 2
    maximumWidth: 760
    maximumHeight: aboutContent.implicitHeight + contentMargin * 2

    function show() {
        visible = true;
        raise();
        requestActivate();
    }

    Text {
        id: versionTextMetrics
        visible: false
        text: aboutDialog.versionText
        textFormat: Text.RichText
    }

    Text {
        id: emailTextMetrics
        visible: false
        text: Translations.id_email_link
        textFormat: Text.RichText
    }

    Text {
        id: youtubeTextMetrics
        visible: false
        text: Translations.id_youtube_channel_link
        textFormat: Text.RichText
    }

    Text {
        id: donationTextMetrics
        visible: false
        text: Translations.id_donations_link
        textFormat: Text.RichText
    }

    Text {
        id: hintTextMetrics
        visible: false
        text: Translations.id_please_click_to_open_link
    }

    Rectangle {
        anchors.fill: parent
        color: "#f7f7f7"

        Column {
            id: aboutContent

            spacing: 8
            anchors {
                fill: parent
                margins: aboutDialog.contentMargin
            }

            Text {
                width: aboutDialog.textWidth
                text: aboutDialog.versionText
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
            }

            Text {
                width: aboutDialog.textWidth
                text: Translations.id_email_link
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }

            Text {
                width: aboutDialog.textWidth
                text: Translations.id_youtube_channel_link
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }

            Text {
                width: aboutDialog.textWidth
                text: Translations.id_donations_link
                textFormat: Text.RichText
                color: "black"
                wrapMode: Text.Wrap
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }

            Text {
                width: aboutDialog.textWidth
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
