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

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.3

import WaveformControl 1.0
import com.enums.zxtapereviver 1.0
import com.models.zxtapereviver 1.0
import com.core.zxtapereviver 1.0

import "."

ApplicationWindow {
    id: mainWindow

    readonly property int mainAreaWidth: width * 0.75
    property var suspiciousPoints: SuspiciousPointsModel.suspiciousPoints

    visible: true
    width: 1600
    height: 800
    title: "ZX Tape Reviver"

    function getWaveShiftIndex(wfWidth, wfXScale) {
        return wfWidth * wfXScale / 2;
    }

    function getSelectedWaveform() {
        return channelsComboBox.currentIndex == 0 ? waveformControlCh0 : waveformControlCh1;
    }

    function restoreWaveformView() {
        waveformControlCh0.xScaleFactor = 1;
        waveformControlCh0.yScaleFactor = 80000;
        waveformControlCh0.wavePos = 0;

        waveformControlCh1.xScaleFactor = 1;
        waveformControlCh1.yScaleFactor = 80000;
        waveformControlCh1.wavePos = 0;
    }

    menuBar: MenuBar {
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
                    parserSettingsDialog.open();
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
                        menuInstantiator.model = Qt.binding(function() { return TranslationManager.languages; });
                    }
                }

                onObjectAdded: languageMenu.insertItem(index, object)
                onObjectRemoved: languageMenu.removeItem(object)
            }
        }

        Menu {
            title: Translations.id_help_menu_item

            MenuItem {
                text: Translations.id_about_menu_item
                onTriggered: {
                    aboutDialog.open();
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

        selectMultiple: false
        sidebarVisible: true

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

            console.log("Selected %1 file: ".arg(filetype) + openFileDialog.fileUrl);
            var res = (openDialogType === openFileDialog.openWfm
                        ? FileWorkerModel.openWaveformFileByUrl(openFileDialog.fileUrl)
                        : openDialogType === openFileDialog.openTap
                           ? FileWorkerModel.openTapFileByUrl(openFileDialog.fileUrl)
                           : FileWorkerModel.openWavFileByUrl(openFileDialog.fileUrl));

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
        selectExisting: false
        selectMultiple: false
        sidebarVisible: true
        defaultSuffix: saveParsed ? Translations.tap_file_suffix : Translations.wfm_file_suffix
        nameFilters: saveParsed ? [ Translations.id_tap_files ] : [ Translations.id_wfm_files ]

        onAccepted: {
            if (saveParsed) {
                if (channelNumber == 0) {
                    waveformControlCh0.saveTap(saveFileDialog.fileUrl);
                }
                else {
                    waveformControlCh1.saveTap(saveFileDialog.fileUrl);
                }
                console.log("Tap saved: " + saveFileDialog.fileUrl)
            }
            else {
                FileWorkerModel.saveWaveformFileByUrl(saveFileDialog.fileUrl);
                console.log("Waveform saved: " + saveFileDialog.fileUrl);
            }
        }
    }

    Connections {
        target: FileWorkerModel
        function onWavFileNameChanged() {
            waveformControlCh0.reparse();
            waveformControlCh1.reparse();
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

            onDoubleClick: {
                SuspiciousPointsModel.addSuspiciousPoint(idx);
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

            onDoubleClick: {
                SuspiciousPointsModel.addSuspiciousPoint(idx);
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
            anchors.top: hZoomOutButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: hZoomOutButton.anchors.topMargin * 10
            width: hZoomOutButton.width

            onClicked: {
                if (DataPlayerModel.stopped) {
                    DataPlayerModel.playParsedData(channelsComboBox.currentIndex, parsedDataView.currentRow === -1 ? 0 : parsedDataView.currentRow);
                    dataPlayerDialog.open();
                } else {
                    DataPlayerModel.stop();
                }
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

            onClicked: {
                var parsedDataViewIdx = parsedDataView.currentRow;
                waveformControlCh0.reparse();
                waveformControlCh1.reparse();
                parsedDataView.selection.select(parsedDataViewIdx);
                parsedDataView.currentRow = parsedDataViewIdx;
            }
        }

        Button {
            id: saveParsedDataButton

            text: Translations.id_save_parsed
            anchors.top: reparseButton.bottom
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
                ActionsModel.shiftWaveform(1300);
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
                gotoAddressDialog.open();
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
            visible: !measurementModeToggleButton.checked

            onCheckedChanged: {
                waveformControlCh0.operationMode = waveformControlCh1.operationMode = checked ? WaveformControlOperationModes.WaveformSelectionMode : WaveformControlOperationModes.WaveformRepairMode;
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
            visible: !selectionModeToggleButton.checked

            onCheckedChanged: {
                waveformControlCh0.operationMode = waveformControlCh1.operationMode = checked ? WaveformControlOperationModes.WaveformMeasurementMode : WaveformControlOperationModes.WaveformRepairMode;
            }
        }

        states: State {
            when: measurementModeToggleButton.checked
            AnchorChanges {
                target: measurementModeToggleButton
                anchors.bottom: waveformControlCh1.bottom
            }
        }

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

        TableView {
            id: parsedDataView

            height: parent.height * 0.4
            anchors {
                top: toBlockEndButton.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }

            TableViewColumn {
                title: Translations.id_block_number
                width: rightArea.width * 0.07
                role: "block"
                delegate: Item {
                    property bool blkSelected: styleData.value.blockSelected
                    property int blkNumber: styleData.value.blockNumber

                    Rectangle {
                        anchors.fill: parent
                        border.width: 0
                        color: parent.blkSelected ? "#A00000FF" : "transparent"
                        Text {
                            anchors.centerIn: parent
                            color: parent.parent.blkSelected ? "white" : "black"
                            text: blkNumber + 1
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            WaveformParser.toggleBlockSelection(blkNumber);
                        }
                    }
                }
            }

            TableViewColumn {
                title: Translations.id_block_type
                width: rightArea.width * 0.23
                role: "blockType"
            }

            TableViewColumn {
                title: Translations.id_block_name
                width: rightArea.width * 0.3
                role: "blockName"
            }

            TableViewColumn {
                title: Translations.id_block_size
                width: rightArea.width * 0.25
                role: "blockSize"
            }

            TableViewColumn {
                title: Translations.id_block_status
                width: rightArea.width * 0.45
                role: "blockStatus"
            }

            selectionMode: SelectionMode.SingleSelection
            model: channelsComboBox.currentIndex === 0 ? WaveformParser.parsedChannel0 : WaveformParser.parsedChannel1
            itemDelegate: Text {
                text: styleData.value
                color: modelData.state === 0 ? "black" : "red"
            }
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

        TableView {
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

            selectionMode: SelectionMode.SingleSelection
            model: suspiciousPoints
            itemDelegate: Text {
                text: styleData.column === 0 ? styleData.row + 1 : styleData.value
            }

            TableViewColumn {
                title: Translations.id_suspicious_point_number
                width: rightArea.width * 0.1
            }

            TableViewColumn {
                title: Translations.id_suspicious_point_position
                width: rightArea.width * 0.9
            }
        }

        Button {
            id: removeActionButton

            anchors {
                top: suspiciousPointsView.bottom
                left: parent.left
                right: parent.right
                leftMargin: 2
                topMargin: 2
            }

            text: Translations.id_remove_action

            onClicked: {
                ActionsModel.removeAction();
                waveformControlCh0.update();
                waveformControlCh1.update();
            }
        }

        TableView {
            id: actionsView

            anchors {
                top: removeActionButton.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }

            selectionMode: SelectionMode.SingleSelection
            model: ActionsModel.actions
            itemDelegate: Text {
                text: styleData.column === 0 ? styleData.row + 1 : modelData.name
            }

            TableViewColumn {
                title: Translations.id_suspicious_point_number
                width: rightArea.width * 0.1
            }

            TableViewColumn {
                title: Translations.id_action_name
                width: rightArea.width * 0.9
            }
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
    }

    ParserSettings {
        id: parserSettingsDialog
    }

    Frequency {
        id: frequencyDialog
        Component.onCompleted: {
            var func = function(fr) { frequency = fr; frequencyDialog.open(); };
            waveformControlCh0.frequency.connect(func);
            waveformControlCh1.frequency.connect(func);
        }
    }

    DataPlayer {
        id: dataPlayerDialog

        selectedChannel: channelsComboBox.currentIndex
        parsedChannel: channelsComboBox.currentIndex === 0 ? WaveformParser.parsedChannel0 : WaveformParser.parsedChannel1
    }
}
