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
import QtQuick.Layouts

import com.models.zxtapereviver 1.0
import "."

Window {
    id: parserSettingsDialog

    visible: false
    title: Translations.id_parser_settings_window_header
    //flags: Qt.Dialog
    modality: Qt.WindowModal
    width: controlsContainer.childrenRect.width + 2 * margin_size
    height: controlsContainer.childrenRect.height + 2 * margin_size

    readonly property int margin_size: 10

    Item {
        id: controlsContainer
        anchors {
            fill: parent
            topMargin: parserSettingsDialog.margin_size
            //leftMargin: parserSettingsDialog.margin_size
        }

        Grid {
            id: grid
            columns: 4
            anchors.horizontalCenter: parent.horizontalCenter

            GroupBox {
                title: Translations.id_pilot_tone_settings
                ColumnLayout {
                    Layout.fillHeight: true
                    Text {
                        text: Translations.id_pilot_half_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.pilotHalfFreq;
                        onTextChanged: {
                            ParserSettingsModel.pilotHalfFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_pilot_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.pilotFreq;
                        onTextChanged: {
                            ParserSettingsModel.pilotFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_pilot_delta
                    }

                    TextField {
                        text: ParserSettingsModel.pilotDelta
                        onTextChanged: {
                            ParserSettingsModel.pilotDelta = text;
                        }
                    }
                }
            }

            GroupBox {
                title: Translations.id_synchro_signal_settings
                ColumnLayout {
                    Text {
                        text: Translations.id_synchro_first_half_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.synchroFirstHalfFreq;
                        onTextChanged: {
                            ParserSettingsModel.synchroFirstHalfFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_synchro_second_half_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.synchroSecondHalfFreq;
                        onTextChanged: {
                            ParserSettingsModel.synchroSecondHalfFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_synchro_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.synchroFreq;
                        onTextChanged: {
                            ParserSettingsModel.synchroFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_synchro_delta
                    }

                    TextField {
                        text: ParserSettingsModel.synchroDelta
                        onTextChanged: {
                            ParserSettingsModel.synchroDelta = text;
                        }
                    }

                    CheckBox {
                        id: preciseSynchroCheck
                        text: Translations.id_precise_synchro_check

                        checked: ParserSettingsModel.preciseSynchroCheck
                        onCheckedChanged: {
                            ParserSettingsModel.preciseSynchroCheck = checked;
                        }
                    }
                }
            }

            GroupBox {
                title: Translations.id_zero_digit_settings
                ColumnLayout {
                    Text {
                        text: Translations.id_zero_half_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.zeroHalfFreq;
                        onTextChanged: {
                            ParserSettingsModel.zeroHalfFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_zero_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.zeroFreq;
                        onTextChanged: {
                            ParserSettingsModel.zeroFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_zero_delta
                    }

                    TextField {
                        text: ParserSettingsModel.zeroDelta
                        onTextChanged: {
                            ParserSettingsModel.zeroDelta = text;
                        }
                    }
                }
            }

            GroupBox {
                title: Translations.id_one_digit_settings
                ColumnLayout {
                    Text {
                        text: Translations.id_one_half_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.oneHalfFreq;
                        onTextChanged: {
                            ParserSettingsModel.oneHalfFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_one_frequency
                    }

                    TextField {
                        text: ParserSettingsModel.oneFreq;
                        onTextChanged: {
                            ParserSettingsModel.oneFreq = parseInt(text, 10);
                        }
                    }

                    Text {
                        text: Translations.id_one_delta
                    }

                    TextField {
                        text: ParserSettingsModel.oneDelta
                        onTextChanged: {
                            ParserSettingsModel.oneDelta = text;
                        }
                    }
                }
            }
        }
        CheckBox {
            id: checkForAbnormalSineCheckbox
            anchors.top: grid.bottom
            anchors.left: grid.left
            anchors.topMargin: 5

            text: Translations.id_check_for_abnormal_sine_when_parsing
            checked: ParserSettingsModel.checkForAbnormalSine
            onCheckedChanged: {
                ParserSettingsModel.checkForAbnormalSine = checked;
            }
        }
        Text {
            id: sineCheckToleranceText
            anchors.top: checkForAbnormalSineCheckbox.bottom
            anchors.left: grid.left
            text: Translations.id_sine_check_tolerance
            visible: checkForAbnormalSineCheckbox.checked
        }
        TextField {
            id: sineCheckToleranceTextField
            anchors.top: sineCheckToleranceText.bottom
            anchors.left: grid.left
            text: ParserSettingsModel.sineCheckTolerance;
            onTextChanged: {
                ParserSettingsModel.sineCheckTolerance = text;
            }
            visible: checkForAbnormalSineCheckbox.checked
        }

        Text {
            id: parserModeText
            anchors.top: sineCheckToleranceTextField.visible ? sineCheckToleranceTextField.bottom : checkForAbnormalSineCheckbox.bottom
            anchors.left: grid.left
            anchors.topMargin: 5
            text: Translations.id_parser_mode
        }

        ComboBox {
            id: parserModeComboBox
            anchors.top: parserModeText.bottom
            anchors.left: grid.left
            model: [
                Translations.id_parser_mode_standard,
                Translations.id_parser_mode_experimental_adaptive,
                Translations.id_parser_mode_experimental_virtual_axis
            ]
            currentIndex: ParserSettingsModel.parserMode
            onActivated: ParserSettingsModel.parserMode = currentIndex
            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: currentText

            Connections {
                target: ParserSettingsModel
                function onParserModeChanged() {
                    parserModeComboBox.currentIndex = ParserSettingsModel.parserMode;
                }
            }
        }

        GroupBox {
            id: adaptiveParserSettingsGroup
            title: Translations.id_adaptive_parser_settings
            anchors.top: parserModeComboBox.bottom
            anchors.left: grid.left
            anchors.topMargin: 5
            visible: ParserSettingsModel.parserMode === ParserSettingsModel.ExperimentalAdaptiveParser ||
                     ParserSettingsModel.parserMode === ParserSettingsModel.ExperimentalAdaptiveVirtualAxisParser

            GridLayout {
                columns: 2

                Text {
                    text: Translations.id_adaptive_parser_preset
                }

                RowLayout {
                    Button {
                        text: Translations.id_adaptive_parser_preset_basic
                        onClicked: ParserSettingsModel.applyAdaptiveParserPreset(ParserSettingsModel.AdaptiveBasicPreset)
                    }

                    Button {
                        text: Translations.id_adaptive_parser_preset_fast
                        onClicked: ParserSettingsModel.applyAdaptiveParserPreset(ParserSettingsModel.AdaptiveFastPreset)
                    }

                    Button {
                        text: Translations.id_adaptive_parser_preset_accurate
                        onClicked: ParserSettingsModel.applyAdaptiveParserPreset(ParserSettingsModel.AdaptiveAccuratePreset)
                    }

                    Button {
                        text: Translations.id_adaptive_parser_preset_maximum
                        onClicked: ParserSettingsModel.applyAdaptiveParserPreset(ParserSettingsModel.AdaptiveMaximumPreset)
                    }
                }

                Text {
                    text: Translations.id_adaptive_alternative_mode
                }

                ComboBox {
                    id: adaptiveAlternativeModeComboBox
                    Layout.preferredWidth: 150
                    model: [
                        Translations.id_adaptive_alternative_mode_smart,
                        Translations.id_adaptive_alternative_mode_full
                    ]
                    currentIndex: ParserSettingsModel.adaptiveAlternativeMode
                    onActivated: ParserSettingsModel.adaptiveAlternativeMode = currentIndex

                    Connections {
                        target: ParserSettingsModel
                        function onAdaptiveAlternativeModeChanged() {
                            adaptiveAlternativeModeComboBox.currentIndex = ParserSettingsModel.adaptiveAlternativeMode
                        }
                    }
                }

                Text {
                    text: Translations.id_adaptive_virtual_axis_mode
                    visible: ParserSettingsModel.parserMode === ParserSettingsModel.ExperimentalAdaptiveVirtualAxisParser
                }

                ComboBox {
                    id: adaptiveVirtualAxisModeComboBox
                    Layout.preferredWidth: 220
                    visible: ParserSettingsModel.parserMode === ParserSettingsModel.ExperimentalAdaptiveVirtualAxisParser
                    model: [
                        Translations.id_adaptive_virtual_axis_median,
                        Translations.id_adaptive_virtual_axis_percentile,
                        Translations.id_adaptive_virtual_axis_low_pass
                    ]
                    currentIndex: ParserSettingsModel.adaptiveVirtualAxisMode
                    onActivated: ParserSettingsModel.adaptiveVirtualAxisMode = currentIndex

                    Connections {
                        target: ParserSettingsModel
                        function onAdaptiveVirtualAxisModeChanged() {
                            adaptiveVirtualAxisModeComboBox.currentIndex = ParserSettingsModel.adaptiveVirtualAxisMode
                        }
                    }
                }

                Text {
                    text: Translations.id_adaptive_base_depth
                }

                TextField {
                    text: ParserSettingsModel.adaptiveBaseDepth
                    Layout.preferredWidth: 90
                    validator: IntValidator { bottom: 2; top: 128 }
                    onEditingFinished: ParserSettingsModel.adaptiveBaseDepth = parseInt(text, 10)
                }

                Text {
                    text: Translations.id_adaptive_uncertain_depth
                }

                TextField {
                    text: ParserSettingsModel.adaptiveUncertainDepth
                    Layout.preferredWidth: 90
                    validator: IntValidator { bottom: 2; top: 128 }
                    onEditingFinished: ParserSettingsModel.adaptiveUncertainDepth = parseInt(text, 10)
                }

                Text {
                    text: Translations.id_adaptive_max_depth
                }

                TextField {
                    text: ParserSettingsModel.adaptiveMaxDepth
                    Layout.preferredWidth: 90
                    validator: IntValidator { bottom: 2; top: 128 }
                    onEditingFinished: ParserSettingsModel.adaptiveMaxDepth = parseInt(text, 10)
                }

                Text {
                    text: Translations.id_adaptive_beam_width
                }

                TextField {
                    text: ParserSettingsModel.adaptiveBeamWidth
                    Layout.preferredWidth: 90
                    validator: IntValidator { bottom: 2; top: 64 }
                    onEditingFinished: ParserSettingsModel.adaptiveBeamWidth = parseInt(text, 10)
                }

                Text {
                    text: Translations.id_adaptive_timing_stability_penalty
                }

                TextField {
                    text: ParserSettingsModel.adaptiveTimingStabilityPenalty
                    Layout.preferredWidth: 90
                    validator: DoubleValidator { bottom: 0.0; top: 2.0; decimals: 2 }
                    onEditingFinished: ParserSettingsModel.adaptiveTimingStabilityPenalty = parseFloat(text)
                }
            }
        }

        Button {
            anchors.top: adaptiveParserSettingsGroup.visible ? adaptiveParserSettingsGroup.bottom : parserModeComboBox.bottom
            anchors.left: grid.left
            anchors.topMargin: parserSettingsDialog.margin_size * 2
            text: Translations.id_restore_defaults_button_text
            onClicked: {
                ParserSettingsModel.restoreDefaultSettings();
            }
        }
        Button {
            anchors.top: adaptiveParserSettingsGroup.visible ? adaptiveParserSettingsGroup.bottom : parserModeComboBox.bottom
            anchors.right: grid.right
            anchors.topMargin: parserSettingsDialog.margin_size * 2
            text: Translations.id_ok_button_text
            onClicked: {
                close();
            }
        }
    }
}
