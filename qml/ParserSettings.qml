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
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import com.models.zxtapereviver 1.0

Dialog {
    id: parserSettingsDialog

    visible: false
    title: "Parser settings"
    standardButtons: StandardButton.Ok | StandardButton.RestoreDefaults
    //modality: Qt.WindowModal

    Grid {
        id: grid
        columns: 4

        GroupBox {
            title: "Pilot-tone settings:"
            ColumnLayout {
                Layout.fillHeight: true
                Text {
                    text: "Pilot half frequency:"
                }

                TextField {
                    text: ParserSettingsModel.pilotHalfFreq;
                    onTextChanged: {
                        ParserSettingsModel.pilotHalfFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "Pilot frequency:"
                }

                TextField {
                    text: ParserSettingsModel.pilotFreq;
                    onTextChanged: {
                        ParserSettingsModel.pilotFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "Pilot delta:"
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
            title: "Synchro signal settigns:"
            ColumnLayout {
                Text {
                    text: "Synchro first half frequency:"
                }

                TextField {
                    text: ParserSettingsModel.synchroFirstHalfFreq;
                    onTextChanged: {
                        ParserSettingsModel.synchroFirstHalfFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "Synchro second half frequency:"
                }

                TextField {
                    text: ParserSettingsModel.synchroSecondHalfFreq;
                    onTextChanged: {
                        ParserSettingsModel.synchroSecondHalfFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "Synchro frequency:"
                }

                TextField {
                    text: ParserSettingsModel.synchroFreq;
                    onTextChanged: {
                        ParserSettingsModel.synchroFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "Synchro delta:"
                }

                TextField {
                    text: ParserSettingsModel.synchroDelta
                    onTextChanged: {
                        ParserSettingsModel.synchroDelta = text;
                    }
                }
            }
        }

        GroupBox {
            title: "Zero digit settings:"
            ColumnLayout {
                Text {
                    text: "Zero half frequency:"
                }

                TextField {
                    text: ParserSettingsModel.zeroHalfFreq;
                    onTextChanged: {
                        ParserSettingsModel.zeroHalfFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "Zero frequency:"
                }

                TextField {
                    text: ParserSettingsModel.zeroFreq;
                    onTextChanged: {
                        ParserSettingsModel.zeroFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "Zero delta:"
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
            title: "One digit settings:"
            ColumnLayout {
                Text {
                    text: "One half frequency:"
                }

                TextField {
                    text: ParserSettingsModel.oneHalfFreq;
                    onTextChanged: {
                        ParserSettingsModel.oneHalfFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "One frequency:"
                }

                TextField {
                    text: ParserSettingsModel.oneFreq;
                    onTextChanged: {
                        ParserSettingsModel.oneFreq = parseInt(text, 10);
                    }
                }

                Text {
                    text: "One delta:"
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
        anchors.top: grid.bottom
        anchors.topMargin: 5

        text: "Check for abnormal sine when parsing"
        checked: ParserSettingsModel.checkForAbnormalSine
        onCheckedChanged: {
            ParserSettingsModel.checkForAbnormalSine = checked;
        }
    }

    onReset: {
        ParserSettingsModel.restoreDefaultSettings();
    }
}
