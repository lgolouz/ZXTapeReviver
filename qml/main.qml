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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.qmlmodels
import Qt.labs.platform

import WaveformControl 1.0
import com.enums.zxtapereviver 1.0
import com.models.zxtapereviver 1.0
import com.core.zxtapereviver 1.0

import "."

ApplicationWindow {
    id: mainWindow

    readonly property int mainAreaWidth: width * 0.75
    property var suspiciousPoints: SuspiciousPointsModel
    property int waveformPlaybackInitialWavePos: 0
    property int waveformPlaybackInitialCursorSample: 0
    property int waveformPlaybackActiveChannel: -1
    property string parserDebugText: Translations.id_parser_debug_not_started
    property bool parserDebugActive: false
    property bool parserStartupPending: false

    visible: true
    width: 1600
    height: 800
    title: openedFileName().length > 0 ? "ZX Tape Reviver - " + openedFileName() : "ZX Tape Reviver"

    function getWaveShiftIndex(wfWidth, wfXScale) {
        return wfWidth * wfXScale / 2;
    }

    function openedFileName() {
        var fileName = FileWorkerModel.wavFileName;
        if (fileName === undefined || fileName === null || fileName.length === 0) {
            return "";
        }

        var pathParts = fileName.split(/[\\/]/);
        return pathParts.length > 0 ? pathParts[pathParts.length - 1] : fileName;
    }

    function getSelectedWaveform() {
        return channelsComboBox.currentIndex == 0 ? waveformControlCh0 : waveformControlCh1;
    }

    function getWaveformByChannel(chNum) {
        return chNum == 0 ? waveformControlCh0 : waveformControlCh1;
    }

    function setWaveformOperationMode(mode) {
        waveformControlCh0.operationMode = mode;
        waveformControlCh1.operationMode = mode;
    }

    function reparseWaveforms() {
        parserStartupPending = false;
        WaveformParser.clearParsingCancellation();
        var parsedDataViewIdx = parsedDataView.currentRow;
        waveformControlCh0.reparse();
        if (!WaveformParser.parsingCancellationRequested) {
            waveformControlCh1.reparse();
        }
        parsedDataView.selection.select(parsedDataViewIdx);
        parsedDataView.currentRow = parsedDataViewIdx;
    }

    Timer {
        id: parserStartupTimer

        interval: 80
        repeat: false

        onTriggered: reparseWaveforms()
    }

    function followParserDebugSample(sample) {
        if (sample === undefined || sample < 0) {
            return;
        }

        var visibleSamples = waveformControlCh0.width * waveformControlCh0.xScaleFactor;
        var idx = sample - visibleSamples * 0.35;
        if (idx < 0) {
            idx = 0;
        }

        waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
    }

    function refreshParserDebugState(chNum) {
        var state = WaveformParser.experimentalDebugState(chNum);
        parserDebugActive = state.active === true;
        parserDebugText = state.message !== undefined && state.message.length > 0
                ? state.message
                : Translations.id_parser_debug_not_started;
        if (state.sample !== undefined) {
            followParserDebugSample(state.sample);
        }

        waveformControlCh0.update();
        waveformControlCh1.update();
    }

    function followWaveformPlayback(sample) {
        if (!playbackModeToggleButton.checked || waveformPlaybackActiveChannel < 0 || sample < 0) {
            return;
        }

        var control = getWaveformByChannel(waveformPlaybackActiveChannel);
        control.cursorSample = sample;

        var visibleSamples = control.width * control.xScaleFactor;
        var rightEdge = waveformControlCh0.wavePos + visibleSamples;
        var targetWavePos = waveformControlCh0.wavePos;
        if (sample > rightEdge - visibleSamples * 0.3) {
            targetWavePos = sample - visibleSamples * 0.65;
        } else if (sample < waveformControlCh0.wavePos + visibleSamples * 0.1) {
            targetWavePos = sample - visibleSamples * 0.2;
        }

        if (targetWavePos < 0) {
            targetWavePos = 0;
        }

        if (targetWavePos !== waveformControlCh0.wavePos) {
            waveformControlCh0.wavePos = waveformControlCh1.wavePos = targetWavePos;
        }
    }

    function finishWaveformPlayback() {
        if (waveformPlaybackActiveChannel < 0) {
            return;
        }

        if (returnWaveformPlaybackPosition.checked) {
            waveformControlCh0.wavePos = waveformControlCh1.wavePos = waveformPlaybackInitialWavePos;
            getWaveformByChannel(waveformPlaybackActiveChannel).cursorSample = waveformPlaybackInitialCursorSample;
        }

        waveformPlaybackActiveChannel = -1;
    }

    function restoreWaveformView() {
        waveformControlCh0.xScaleFactor = 1;
        waveformControlCh0.yScaleFactor = 80000;
        waveformControlCh0.wavePos = 0;

        waveformControlCh1.xScaleFactor = 1;
        waveformControlCh1.yScaleFactor = 80000;
        waveformControlCh1.wavePos = 0;
    }

    Connections {
        target: WaveformPlayerModel

        function onCurrentSampleChanged() {
            followWaveformPlayback(WaveformPlayerModel.currentSample);
        }

        function onStoppedChanged() {
            if (WaveformPlayerModel.stopped && waveformPlaybackActiveChannel >= 0) {
                finishWaveformPlayback();
            }
        }
    }

    Connections {
        target: WaveformParser

        function onExperimentalDebugChanged(chNum) {
            if (chNum === channelsComboBox.currentIndex) {
                refreshParserDebugState(chNum);
            } else {
                waveformControlCh0.update();
                waveformControlCh1.update();
            }
        }
    }

    MenuBar {
        Menu {
            title: Translations.id_file_menu_item

            MenuItem {
                text: Translations.id_open_wav_file_menu_item
                onTriggered: {
                    console.log("Opening WAV file");
                    openFileDialog.openDialogType = openFileDialog.openWav;
                    openFileDialog.open();
                }
            }

            MenuItem {
                text: Translations.id_open_waveform_file_menu_item
                onTriggered:  {
                    console.log("Opening Waveform file");
                    openFileDialog.openDialogType = openFileDialog.openWfm;
                    openFileDialog.open();
                }
            }

            MenuItem {
                text: Translations.id_open_tap_file_menu_item
                onTriggered: {
                    console.log("Opening TAP file");
                    openFileDialog.openDialogType = openFileDialog.openTap;
                    openFileDialog.open();
                }
            }

            MenuSeparator { }

            Menu {
                title: Translations.id_save_menu_item

                Menu {
                    title: Translations.id_save_parsed_menu_item

                    MenuItem {
                        text: Translations.id_left_channel_menu_item

                        onTriggered: {
                            saveFileDialog.saveParsed = true;
                            saveFileDialog.channelNumber = 0;
                            saveFileDialog.open();
                        }
                    }

                    MenuItem {
                        text: Translations.id_right_channel_menu_item

                        onTriggered: {
                            saveFileDialog.saveParsed = true;
                            saveFileDialog.channelNumber = 1;
                            saveFileDialog.open();
                        }
                    }
                }

                MenuItem {
                    text: Translations.id_save_waveform_menu_item

                    onTriggered: {
                        saveFileDialog.saveParsed = false;
                        saveFileDialog.open();
                    }
                }
            }

            MenuSeparator { }

            MenuItem {
                text: Translations.id_exit_menu_item
                onTriggered: {
                    mainWindow.close();
                }
            }
        }

        Menu {
            title: Translations.id_waveform_menu_item

            MenuItem {
                text: Translations.id_restore_view_menu_item
                onTriggered: {
                    restoreWaveformView();
                }
            }

            MenuItem {
                text: Translations.id_reparse_menu_item
            }

            MenuSeparator { }

            MenuItem {
                text: Translations.id_parser_settings_menu_item
                onTriggered: {
                    parserSettingsDialog.show();
                }
            }
        }

        Menu {
            id: languageMenu
            title: Translations.id_language_menu_item

            Instantiator  {
                id: menuInstantiator

                model: TranslationManager.languages
                MenuItem {
                    readonly property int countryCode: modelData.countryCode

                    text: modelData.language
                    onTriggered: {
                        //Refreshing the mainWindow to update main menu translation
                        mainWindow.hide();
                        TranslationManager.setTranslation(countryCode);
                        mainWindow.show();
                        //Re-assign the menu items binding
                        //menuInstantiator.model = Qt.binding(function() { return TranslationManager.languages; });
                    }
                }

                onObjectAdded: (index, object) => { languageMenu.insertItem(index, object) }
                onObjectRemoved: (object) => { languageMenu.removeItem(object) }
            }
        }

        Menu {
            title: Translations.id_help_menu_item

            MenuItem {
                text: Translations.id_about_menu_item
                onTriggered: {
                    aboutDialog.show();
                }
            }
        }
    }

    FileDialog {
        id: openFileDialog

        readonly property int openWav: 0
        readonly property int openWfm: 1
        readonly property int openTap: 2

        property int openDialogType: openFileDialog.openWav

        title: openDialogType === openFileDialog.openWfm
                 ? Translations.id_please_choose_wfm_file
                 : openDialogType === openFileDialog.openTap
                   ? Translations.id_please_choose_tap_file
                   : Translations.id_please_choose_wav_file

        //selectMultiple: false
        //sidebarVisible: true
        fileMode: FileDialog.OpenFile

        defaultSuffix: openDialogType === openFileDialog.openWfm
                       ? Translations.wfm_file_suffix
                       : openDialogType === openFileDialog.openTap
                         ? Translations.tap_file_suffix
                         : Translations.wav_file_suffix

        nameFilters: openDialogType === openFileDialog.openWfm
                       ? [ Translations.id_wfm_files ]
                       : openDialogType === openFileDialog.openTap
                         ? [ Translations.id_tap_files ]
                         : [ Translations.id_wav_files ]

        onAccepted: {
            var filetype = openDialogType === openFileDialog.openWfm
                             ? "Waveform"
                             : openDialogType === openFileDialog.openTap
                               ? "TAP"
                               : "WAV";

            console.log("Selected %1 file: ".arg(filetype) + openFileDialog.currentFile);
            var res = (openDialogType === openFileDialog.openWfm
                        ? FileWorkerModel.openWaveformFileByUrl(openFileDialog.currentFile)
                        : openDialogType === openFileDialog.openTap
                           ? FileWorkerModel.openTapFileByUrl(openFileDialog.currentFile)
                           : FileWorkerModel.openWavFileByUrl(openFileDialog.currentFile));

            console.log("Open %1 file result: ".arg(filetype) + res);
            if (res === 0) {
                if (openDialogType !== openFileDialog.openWfm) {
                    SuspiciousPointsModel.clearSuspiciousPoints();
                }
                restoreWaveformView();
            }
        }

        onRejected: {
            console.log("No WAV file selected");
        }
    }

    FileDialog {
        id: saveFileDialog

        property bool saveParsed: true
        property int channelNumber: 0

        title: saveParsed ? Translations.id_save_tap_file : Translations.id_save_wfm_file
        //selectExisting: false
        //selectMultiple: false
        //sidebarVisible: true
        defaultSuffix: saveParsed ? Translations.tap_file_suffix : Translations.wfm_file_suffix
        nameFilters: saveParsed ? [ Translations.id_tap_files ] : [ Translations.id_wfm_files ]
        fileMode: FileDialog.SaveFile

        onAccepted: {
            if (saveParsed) {
                if (channelNumber == 0) {
                    waveformControlCh0.saveTap(saveFileDialog.currentFile);
                }
                else {
                    waveformControlCh1.saveTap(saveFileDialog.currentFile);
                }
                console.log("Tap saved: " + saveFileDialog.currentFile)
            }
            else {
                FileWorkerModel.saveWaveformFileByUrl(saveFileDialog.currentFile);
                console.log("Waveform saved: " + saveFileDialog.currentFile);
            }
        }
    }

    MessageDialog {
        id: saveTapErrorDialog

        title: Translations.id_error
        text: ""
        buttons: MessageDialog.Ok
    }

    function saveTapErrorText(errorCode, details) {
        switch (errorCode) {
        case WaveformParser.NoParsedData:
            return Translations.id_no_parsed_data_for_selected_channel;
        case WaveformParser.CannotRemoveExistingFile:
            return Translations.id_cannot_replace_tap_file.arg(details);
        case WaveformParser.CannotOpenFile:
            return Translations.id_cannot_open_tap_file_for_writing.arg(details);
        default:
            return details;
        }
    }

    function showSaveTapError(fileName, errorCode, details) {
        saveTapErrorDialog.text = Translations.id_cannot_save_tap_file.arg(fileName).arg(saveTapErrorText(errorCode, details));
        saveTapErrorDialog.open();
    }

    Connections {
        target: FileWorkerModel
        function onWavFileNameChanged() {
            reparseWaveforms();
        }
    }

    Rectangle {
        id: mainArea

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: mainAreaWidth
        color: "black"

        readonly property int spacerHeight: ~~(parent.height * 0.0075);
        readonly property string hotkeyHint: " (%1)"

        WaveformControl {
            id: waveformControlCh0

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: vZoomInButton.left
            anchors.rightMargin: 5

            channelNumber: 0
            width: parent.width - (parent.width * 0.11)
            height: parent.height - parent.height / 2 - parent.spacerHeight / 2

            Behavior on wavePos {
                enabled: !WaveformPlayerModel.stopped
                NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
            }

            onDoubleClick: (idx) => {
                SuspiciousPointsModel.addSuspiciousPoint(idx);
            }

            onSaveTapFailed: (fileName, error, details) => {
                mainWindow.showSaveTapError(fileName, error, details);
            }
        }

        WaveformControl {
            id: waveformControlCh1

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: vZoomInButton.left
            anchors.rightMargin: 5

            channelNumber: 1
            width: parent.width - (parent.width * 0.11)
            height: waveformControlCh0.height

            Behavior on wavePos {
                enabled: !WaveformPlayerModel.stopped
                NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
            }

            onDoubleClick: (idx) => {
                SuspiciousPointsModel.addSuspiciousPoint(idx);
            }

            onSaveTapFailed: (fileName, error, details) => {
                mainWindow.showSaveTapError(fileName, error, details);
            }
        }

        Button {
            id: vZoomInButton

            Shortcut {
                id: shortcut_vZoomIn

                sequence: "w"
                autoRepeat: false
                onActivated: vZoomInButton.clicked()
            }

            text: Translations.id_vertical_zoom_in + mainArea.hotkeyHint.arg(shortcut_vZoomIn.sequence)
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5
            width: hZoomOutButton.width

            onClicked: {
                var yfactor = waveformControlCh0.yScaleFactor;
                if (yfactor > 1000) {
                    waveformControlCh0.yScaleFactor = yfactor / 2;
                }

                yfactor = waveformControlCh1.yScaleFactor;
                if (yfactor > 1000) {
                    waveformControlCh1.yScaleFactor = yfactor / 2;
                }
            }
        }

        Button {
            id: vZoomOutButton

            Shortcut {
                id: shortcut_vZoomOut

                sequence: "s"
                autoRepeat: false
                onActivated: vZoomOutButton.clicked()
            }

            text: Translations.id_vertical_zoom_out + mainArea.hotkeyHint.arg(shortcut_vZoomOut.sequence)
            anchors.top: vZoomInButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5
            width: hZoomOutButton.width

            onClicked: {
                var yfactor = waveformControlCh0.yScaleFactor;
                if (yfactor < 320000) {
                    waveformControlCh0.yScaleFactor = yfactor * 2;
                }

                yfactor = waveformControlCh1.yScaleFactor;
                if (yfactor < 320000) {
                    waveformControlCh1.yScaleFactor = yfactor * 2;
                }
            }
        }

        Button {
            id: hZoomInButton

            Shortcut {
                id: shortcut_hZoomIn

                sequence: "e"
                autoRepeat: false
                onActivated: hZoomInButton.clicked()
            }

            text: Translations.id_horizontal_zoom_in + mainArea.hotkeyHint.arg(shortcut_hZoomIn.sequence)
            anchors.top: vZoomOutButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 10
            width: hZoomOutButton.width

            onClicked: {
                waveformControlCh0.xScaleFactor = waveformControlCh0.xScaleFactor / 2;
                waveformControlCh1.xScaleFactor = waveformControlCh1.xScaleFactor / 2;
            }
        }

        Button {
            id: hZoomOutButton

            Shortcut {
                id: shortcut_hZoomOut

                sequence: "q"
                autoRepeat: false
                onActivated: hZoomOutButton.clicked()
            }

            text: Translations.id_horizontal_zoom_out + mainArea.hotkeyHint.arg(shortcut_hZoomOut.sequence)
            anchors.top: hZoomInButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5

            onClicked: {
                waveformControlCh0.xScaleFactor = waveformControlCh0.xScaleFactor * 2;
                waveformControlCh1.xScaleFactor = waveformControlCh1.xScaleFactor * 2;
            }
        }

        Button {
            id: playParsedData

            text: DataPlayerModel.stopped ? Translations.id_play_parsed_data : Translations.id_stop_playing_parsed_data
            enabled: DataPlayerModel.stopped && WaveformPlayerModel.stopped
            anchors.top: hZoomOutButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: hZoomOutButton.anchors.topMargin * 10
            width: hZoomOutButton.width

            onClicked: {
                if (DataPlayerModel.stopped) {
                    DataPlayerModel.playParsedData(channelsComboBox.currentIndex, parsedDataView.currentRow === -1 ? 0 : parsedDataView.currentRow);
                    dataPlayerDialog.show();
                } else {
                    DataPlayerModel.stop();
                }
            }
        }

        Button {
            id: playWaveformFromCursor

            text: WaveformPlayerModel.stopped ? Translations.id_play_waveform_from_cursor : Translations.id_stop_playing_parsed_data
            enabled: playbackModeToggleButton.checked && DataPlayerModel.stopped
            anchors.top: playParsedData.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5
            width: hZoomOutButton.width

            onClicked: {
                if (!WaveformPlayerModel.stopped) {
                    WaveformPlayerModel.stop();
                    return;
                }

                var control = getSelectedWaveform();
                waveformPlaybackInitialWavePos = waveformControlCh0.wavePos;
                waveformPlaybackInitialCursorSample = control.cursorSample;
                var channel = channelsComboBox.currentIndex;
                if (WaveformPlayerModel.playChannelFromSample(channel, control.cursorSample)) {
                    waveformPlaybackActiveChannel = channel;
                } else {
                    waveformPlaybackActiveChannel = -1;
                }
            }
        }

        Button {
            id: pauseWaveformPlayback

            text: WaveformPlayerModel.paused ? Translations.id_resume_playing_parsed_data : Translations.id_pause_playing_parsed_data
            enabled: playbackModeToggleButton.checked && !WaveformPlayerModel.stopped
            anchors.top: playWaveformFromCursor.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5
            width: hZoomOutButton.width

            onClicked: {
                if (WaveformPlayerModel.paused) {
                    WaveformPlayerModel.resume();
                } else {
                    WaveformPlayerModel.pause();
                }
            }
        }

        CheckBox {
            id: returnWaveformPlaybackPosition

            text: Translations.id_return_waveform_playback_position
            checked: true
            anchors.top: pauseWaveformPlayback.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 2
            width: hZoomOutButton.width
            visible: playbackModeToggleButton.checked

            contentItem: Text {
                text: Translations.id_return_waveform_playback_position
                color: "white"
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
                leftPadding: returnWaveformPlaybackPosition.indicator.width + returnWaveformPlaybackPosition.spacing
            }
        }

        Button {
            id: shiftWaveRight

            Shortcut {
                id: shortcut_shiftWaveRight

                sequence: "a"
                autoRepeat: true
                onActivated: shiftWaveRight.clicked()
            }

            text: Translations.id_waveform_shift_right + mainArea.hotkeyHint.arg(shortcut_shiftWaveRight.sequence)
            anchors.bottom: waveformControlCh0.bottom
            anchors.left: hZoomOutButton.left
            width: 40

            onClicked: {
                waveformControlCh0.wavePos -= getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                waveformControlCh1.wavePos -= getWaveShiftIndex(waveformControlCh1.width, waveformControlCh1.xScaleFactor);
            }
        }

        Button {
            id: shiftWaveLeft

            Shortcut {
                id: shortcut_shiftWaveLeft

                sequence: "d"
                autoRepeat: true
                onActivated: shiftWaveLeft.clicked()
            }

            text: Translations.id_waveform_shift_left + mainArea.hotkeyHint.arg(shortcut_shiftWaveLeft.sequence)
            anchors.bottom: shiftWaveRight.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            width: 40

            onClicked: {
                waveformControlCh0.wavePos += getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                waveformControlCh1.wavePos += getWaveShiftIndex(waveformControlCh1.width, waveformControlCh1.xScaleFactor);
            }
        }

        Button {
            id: reparseButton

            function reparse() {
            }

            Shortcut {
                id: shortcut_reparseButton

                sequence: "p"
                autoRepeat: true
                onActivated: reparseButton.clicked()
            }

            Shortcut {
                id: shortcut_reparseButtonShift

                sequence: "Shift+s"
                autoRepeat: true
                onActivated: reparseButton.clicked()
            }

            text: Translations.id_reparse + mainArea.hotkeyHint.arg(shortcut_reparseButton.sequence + " / " + shortcut_reparseButtonShift.sequence)
            anchors.top: shiftWaveLeft.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width
            enabled: !WaveformParser.parsingActive && !parserStartupPending

            onClicked: {
                parserStartupPending = true;
                parserStartupTimer.restart();
            }
        }

        Button {
            id: saveParsedDataButton

            text: Translations.id_save_parsed
            anchors.top: parsingProgressPanel.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                if (channelsComboBox.currentIndex == 0) {
                    waveformControlCh0.saveTap();
                } else {
                    waveformControlCh1.saveTap();
                }
            }
        }

        Rectangle {
            id: parsingProgressPanel

            anchors {
                top: reparseButton.bottom
                right: parent.right
                rightMargin: 5
                topMargin: 5
            }
            width: hZoomOutButton.width
            height: WaveformParser.parsingActive || parserStartupPending ? 118 : 0
            visible: WaveformParser.parsingActive || parserStartupPending
            color: "#202020"
            border.color: "#666666"
            clip: true

            Text {
                id: parsingProgressText

                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    leftMargin: 4
                    rightMargin: 4
                    topMargin: 4
                }
                height: 34
                text: parserStartupPending ? Translations.id_parser_starting : WaveformParser.parsingStatus
                color: "white"
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                font.pixelSize: 11
            }

            ProgressBar {
                id: parsingProgressBar

                anchors {
                    top: parsingProgressText.bottom
                    left: parent.left
                    right: parent.right
                    leftMargin: 4
                    rightMargin: 4
                    topMargin: 4
                }
                from: 0
                to: 100
                indeterminate: parserStartupPending || (WaveformParser.parsingActive && WaveformParser.parsingProgress <= 0)
                value: parserStartupPending ? 0 : WaveformParser.parsingProgress
            }

            Button {
                anchors {
                    top: parsingProgressBar.bottom
                    right: parent.right
                    topMargin: 8
                    rightMargin: 4
                }
                width: 70
                height: 24
                text: Translations.id_stop_parser
                enabled: WaveformParser.parsingActive && !WaveformParser.parsingCancellationRequested

                onClicked: WaveformParser.cancelParsing()
            }
        }

        Button {
            id: saveWaveformButton

            text: Translations.id_save_waveform
            anchors.top: saveParsedDataButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                waveformControlCh0.saveWaveform();
                //waveformControlCh1.saveWaveform();
            }
        }

        Button {
            id: repairRestoreButton

            text: getSelectedWaveform().isWaveformRepaired ? Translations.id_restore_waveform : Translations.id_repair_waveform
            anchors.top: saveParsedDataButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                if (getSelectedWaveform().isWaveformRepaired) {
                    getSelectedWaveform().restoreWaveform();
                }
                else {
                    getSelectedWaveform().repairWaveform();
                }
            }
        }

        Button {
            id: shiftWaveform

            text: Translations.id_shift_waveform
            anchors {
                top: repairRestoreButton.bottom
                topMargin: 5
                right: parent.right
                rightMargin: 5
            }
            width: hZoomOutButton.width

            onClicked: {
                ActionsModel.shiftWaveform(-300);
                waveformControlCh0.update();
                //getSelectedWaveform().shiftWaveform();
            }
        }

        Button {
            id: gotoAddressButton

            text: Translations.id_goto_address
            anchors {
                top: shiftWaveform.bottom
                topMargin: 5
                right: parent.right
                rightMargin: 5
            }
            width: hZoomOutButton.width

            onClicked: {
                gotoAddressDialog.show();
            }
        }

        Button {
            id: selectionModeToggleButton

            text: Translations.id_selection_mode
            anchors.right: parent.right
            anchors.bottom: waveformControlCh1.bottom
            anchors.bottomMargin: 5
            anchors.rightMargin: 5
            width: hZoomOutButton.width
            checkable: true
            visible: !measurementModeToggleButton.checked && !playbackModeToggleButton.checked

            onCheckedChanged: {
                setWaveformOperationMode(checked ? WaveformControlOperationModes.WaveformSelectionMode : WaveformControlOperationModes.WaveformRepairMode);
            }
        }

        Button {
            id: measurementModeToggleButton

            text: Translations.id_measurement_mode
            anchors {
                right: parent.right
                rightMargin: 5
                bottom: selectionModeToggleButton.top
                bottomMargin: 5
            }
            width: hZoomOutButton.width
            checkable: true
            visible: !selectionModeToggleButton.checked && !playbackModeToggleButton.checked

            onCheckedChanged: {
                setWaveformOperationMode(checked ? WaveformControlOperationModes.WaveformMeasurementMode : WaveformControlOperationModes.WaveformRepairMode);
            }
        }

        Button {
            id: playbackModeToggleButton

            text: Translations.id_waveform_playback_mode
            anchors {
                right: parent.right
                rightMargin: 5
                bottom: measurementModeToggleButton.top
                bottomMargin: 5
            }
            width: hZoomOutButton.width
            checkable: true
            visible: !selectionModeToggleButton.checked && !measurementModeToggleButton.checked

            onCheckedChanged: {
                if (checked) {
                    setWaveformOperationMode(WaveformControlOperationModes.WaveformPlaybackMode);
                } else {
                    if (!WaveformPlayerModel.stopped) {
                        WaveformPlayerModel.stop();
                    }
                    finishWaveformPlayback();
                    setWaveformOperationMode(WaveformControlOperationModes.WaveformRepairMode);
                }
            }
        }

        states: [
            State {
                when: measurementModeToggleButton.checked
                AnchorChanges {
                    target: measurementModeToggleButton
                    anchors.bottom: waveformControlCh1.bottom
                }
            },
            State {
                when: playbackModeToggleButton.checked
                AnchorChanges {
                    target: playbackModeToggleButton
                    anchors.bottom: waveformControlCh1.bottom
                }
            }
        ]

        Button {
            id: copyFromRigthToLeftChannel

            anchors.right: parent.right
            anchors.bottom: selectionModeToggleButton.top
            anchors.bottomMargin: 15
            anchors.rightMargin: 5
            width: hZoomOutButton.width

            text: Translations.id_copy_from_r_to_l
            visible: selectionModeToggleButton.checked
            onClicked: {
                waveformControlCh1.copySelectedToAnotherChannel();
                waveformControlCh0.update();
            }
        }

        Button {
            id: copyFromLeftToRightChannel

            anchors.right: parent.right
            anchors.bottom: copyFromRigthToLeftChannel.top
            anchors.bottomMargin: 5
            anchors.rightMargin: 5
            width: hZoomOutButton.width

            text: Translations.id_copy_from_l_to_r
            visible: selectionModeToggleButton.checked
            onClicked: {
                waveformControlCh0.copySelectedToAnotherChannel();
                waveformControlCh1.update();
            }
        }
    }

    Rectangle {
        id: rightArea

        anchors {
            left: mainArea.right
            top: parent.top
            bottom: parent.bottom
        }

        width: parent.width - mainAreaWidth

        color: "transparent"

        ComboBox {
            id: channelsComboBox

            model: [Translations.id_left_channel, Translations.id_right_channel]
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            onCurrentIndexChanged: {
                parsedDataView.invalidate();
                refreshParserDebugState(currentIndex);
            }
        }

        Button {
            id: toBlockBeginningButton

            Shortcut {
                id: shortcut_toBlockBeginning

                sequence: "Shift+a"
                autoRepeat: true
                onActivated: toBlockBeginningButton.clicked()
            }

            ToolTip {
                delay: 1000
                timeout: 5000
                visible: toBlockBeginningButton.hovered
                text: Translations.id_hotkey_tooltip.arg(shortcut_toBlockBeginning.sequence)
            }

            text: Translations.id_to_the_beginning_of_the_block
            anchors {
                top: channelsComboBox.bottom
                left: parent.left
                rightMargin: 2
                topMargin: 2
                bottomMargin: 2
            }
            width: parent.width / 2

            onClicked: {
                if (parsedDataView.currentRow !== -1) {
                    var idx = WaveformParser.getBlockDataStart(channelsComboBox.currentIndex, parsedDataView.currentRow) - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                    if (idx < 0) {
                        idx = 0;
                    }

                    waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
                }
            }
        }

        Button {
            id: toBlockEndButton

            Shortcut {
                id: shortcut_toBlockEnd

                sequence: "Shift+d"
                autoRepeat: true
                onActivated: toBlockEndButton.clicked()
            }

            ToolTip {
                delay: 1000
                timeout: 5000
                visible: toBlockEndButton.hovered
                text: Translations.id_hotkey_tooltip.arg(shortcut_toBlockEnd.sequence)
            }

            text: Translations.id_to_the_end_of_the_block
            anchors {
                top: channelsComboBox.bottom
                right: parent.right
                left: toBlockBeginningButton.right
                leftMargin: 2
                topMargin: 2
                bottomMargin: 2
            }

            onClicked: {
                if (parsedDataView.currentRow !== -1) {
                    var idx = WaveformParser.getBlockDataEnd(channelsComboBox.currentIndex, parsedDataView.currentRow) - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                    if (idx < 0) {
                        idx = 0;
                    }

                    waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
                }
            }
        }

        Rectangle {
            id: parserDebugPanel

            anchors {
                top: toBlockEndButton.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }
            height: Math.min(250, parent.height * 0.32)
            color: "#222222"
            border.color: parserDebugActive ? "#70c8ff" : "#555555"
            border.width: 1

            Text {
                id: parserDebugHeader

                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    margins: 4
                }
                height: 18
                text: Translations.id_parser_debug
                color: "white"
                font.bold: true
                elide: Text.ElideRight
            }

            Button {
                id: startParserDebugButton

                anchors {
                    top: parserDebugHeader.bottom
                    left: parent.left
                    right: parent.horizontalCenter
                    margins: 4
                    rightMargin: 2
                }
                text: Translations.id_start_parser_debug
                enabled: !parserDebugActive

                onClicked: {
                    WaveformParser.startExperimentalDebug(channelsComboBox.currentIndex);
                }
            }

            Button {
                id: nextParserDebugButton

                anchors {
                    top: parserDebugHeader.bottom
                    left: parent.horizontalCenter
                    right: parent.right
                    margins: 4
                    leftMargin: 2
                }
                text: Translations.id_next_parser_debug_step
                enabled: parserDebugActive

                onClicked: {
                    WaveformParser.nextExperimentalDebugStep();
                }
            }

            Button {
                id: stopParserDebugButton

                anchors {
                    top: startParserDebugButton.bottom
                    left: parent.left
                    right: parent.right
                    margins: 4
                }
                text: Translations.id_stop_parser_debug
                enabled: parserDebugActive

                onClicked: {
                    WaveformParser.stopExperimentalDebug();
                }
            }

            ScrollView {
                id: parserDebugDetailsScroll

                anchors {
                    top: stopParserDebugButton.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    margins: 4
                }
                clip: true

                TextArea {
                    id: parserDebugDetails

                    width: parserDebugDetailsScroll.availableWidth
                    readOnly: true
                    wrapMode: Text.WordWrap
                    text: parserDebugText
                    color: "white"
                    selectedTextColor: "black"
                    selectionColor: "#9fdcff"
                    font.pixelSize: 11
                    background: Rectangle {
                        color: "#111111"
                        border.color: "#444444"
                    }
                }

                background: Rectangle {
                    color: "#111111"
                    border.color: "#444444"
                }
            }
        }

        ZXTableControl {
            id: parsedDataView

            height: parent.height * 0.25
            anchors {
                top: parserDebugPanel.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }
            model: channelsComboBox.currentIndex === 0 ? WaveformParser.parsedChannel0 : WaveformParser.parsedChannel1
            checkableRows: true
            checkedRowProvider: function(row) { return WaveformParser.isBlockSelected(channelsComboBox.currentIndex, row); }
            checkedRowSetter: function(row, checked) { WaveformParser.setBlockSelected(channelsComboBox.currentIndex, row, checked); }
            rowErrorProvider: function(row) { return WaveformParser.isBlockParseError(channelsComboBox.currentIndex, row); }
            fallbackHeaders: [
                Translations.id_block_number,
                Translations.id_block_type,
                Translations.id_block_name,
                Translations.id_block_size,
                Translations.id_block_status
            ]
            fallbackColumnWidths: [40, 80, 120, 60, 60]
        }


        Button {
            id: gotoPointButton

            anchors {
                top: parsedDataView.bottom
                left: parent.left
                rightMargin: 2
                topMargin: 2
            }
            width: parent.width / 2
            text: Translations.id_goto_suspicious_point

            onClicked: {
                if (suspiciousPointsView.currentRow < 0 || suspiciousPointsView.currentRow >= SuspiciousPointsModel.size) {
                    return;
                }

                var idx = SuspiciousPointsModel.getSuspiciousPoint(suspiciousPointsView.currentRow) - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                if (idx < 0) {
                    idx = 0;
                }
                console.log("Go to point: " + idx);

                waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
            }
        }

        Button {
            id: removePointButton

            anchors {
                top: parsedDataView.bottom
                left: gotoPointButton.right
                right: parent.right
                leftMargin: 2
                topMargin: 2
            }

            text: Translations.id_remove_suspicious_point

            onClicked: {
                if (suspiciousPointsView.currentRow >= 0 && suspiciousPointsView.currentRow < SuspiciousPointsModel.size) {
                    console.log("Removing suspicious point: " + SuspiciousPointsModel.getSuspiciousPoint(suspiciousPointsView.currentRow));
                    SuspiciousPointsModel.removeSuspiciousPoint(suspiciousPointsView.currentRow);
                }
            }
        }

        ZXTableControl {
            id: suspiciousPointsView

            anchors {
                top: gotoPointButton.bottom
                //bottom: parent.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }
            height: parent.height * 0.25
            implicitHeight: parent.height * 0.25

             model: suspiciousPoints
             fallbackHeaders: [
                 Translations.id_suspicious_point_number,
                 Translations.id_suspicious_point_position
             ]
             fallbackColumnWidths: [70, 180]
        }

        Button {
            id: removeActionButton

            anchors {
                top: suspiciousPointsView.bottom
                left: parent.left
                right: parent.horizontalCenter
                leftMargin: 2
                rightMargin: 1
                topMargin: 2
            }

            text: Translations.id_remove_action
            enabled: ActionsModel.canUndo

            onClicked: {
                ActionsModel.removeAction();
                waveformControlCh0.update();
                waveformControlCh1.update();
            }
        }

        Button {
            id: redoActionButton

            anchors {
                top: removeActionButton.top
                left: parent.horizontalCenter
                right: parent.right
                leftMargin: 1
                rightMargin: 2
            }

            text: Translations.id_redo_action
            enabled: ActionsModel.canRedo

            onClicked: {
                ActionsModel.redoAction();
                waveformControlCh0.update();
                waveformControlCh1.update();
            }
        }

        ZXTableControl {
            id: actionsView

            anchors {
                top: removeActionButton.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }

            model: ActionsModel
            fallbackHeaders: [
                Translations.id_suspicious_point_number,
                Translations.id_action_name
            ]
            fallbackColumnWidths: [70, 180]
        }
    }

    GoToAddress {
        id: gotoAddressDialog
        onGotoAddress: {
            console.log("Goto address: " + adr);
            var pos = WaveformParser.getPositionByAddress(channelsComboBox.currentIndex, parsedDataView.currentRow, adr);
            if (pos !== 0) {
                var idx = pos - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                if (idx < 0) {
                    idx = 0;
                }

                waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
            }
        }
    }

    About {
        id: aboutDialog
        transientParent: mainWindow
    }

    ParserSettings {
        id: parserSettingsDialog
        //transientParent: parent
    }

    Frequency {
        id: frequencyDialog
        //parent: parent

        Component.onCompleted: {
            var func = function(fr) { frequency = fr; frequencyDialog.show(); };
            waveformControlCh0.frequency.connect(func);
            waveformControlCh1.frequency.connect(func);
        }
    }

    DataPlayer {
        id: dataPlayerDialog
        //parent: parent

        selectedChannel: channelsComboBox.currentIndex
        parsedChannel: channelsComboBox.currentIndex === 0 ? WaveformParser.parsedChannel0 : WaveformParser.parsedChannel1
    }
}
