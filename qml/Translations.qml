pragma Singleton

import QtQuick 2.0
import com.models.zxtapereviver 1.0

QtObject {
    id: tranlations

    readonly property string wav_file_suffix: "wav"
    readonly property string wfm_file_suffix: "wfm"
    readonly property string tap_file_suffix: "tap"
    readonly property string filename_wildcard: "*."

    property string id_about_window_header:                  qsTrId("id_about_window_header") + TranslationManager.translationChanged
    property string id_please_click_to_open_link:            qsTrId("id_please_click_to_open_link") + TranslationManager.translationChanged
    property string id_email_link:                           qsTrId("id_email_link").arg('<a href="mailto:computerenthusiasttips@mail.ru">computerenthusiasttips@mail.ru</a>') + TranslationManager.translationChanged
    property string id_youtube_channel_name:                 qsTrId("id_youtube_channel_name") + TranslationManager.translationChanged
    property string id_youtube_channel_link:                 qsTrId("id_youtube_channel_link").arg('<a href="https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg">%1</a>'.arg(id_youtube_channel_name))
    property string id_donations_link:                       qsTrId("id_donations_link").arg('<a href="https://destream.net/live/lgolouz/donate">https://destream.net/live/lgolouz/donate</a>')
    property string id_measured_frequency_window_header:     qsTrId("id_measured_frequency_window_header") + TranslationManager.translationChanged
    property string id_measured_frequency:                   qsTrId("id_measured_frequency") + TranslationManager.translationChanged
    property string id_goto_address_window_header:           qsTrId("id_goto_address_window_header") + TranslationManager.translationChanged
    property string id_please_enter_address:                 qsTrId("id_please_enter_address") + TranslationManager.translationChanged
    property string id_hexadecimal:                          qsTrId("id_hexadecimal") + TranslationManager.translationChanged
    property string id_file_menu_item:                       qsTrId("id_file_menu_item") + TranslationManager.translationChanged
    property string id_open_wav_file_menu_item:              qsTrId("id_open_wav_file_menu_item") + TranslationManager.translationChanged
    property string id_open_waveform_file_menu_item:         qsTrId("id_open_waveform_file_menu_item") + TranslationManager.translationChanged
    property string id_open_tap_file_menu_item:              qsTrId("id_open_tap_file_menu_item") + TranslationManager.translationChanged
    property string id_save_menu_item:                       qsTrId("id_save_menu_item") + TranslationManager.translationChanged
    property string id_save_parsed_menu_item:                qsTrId("id_save_parsed_menu_item") + TranslationManager.translationChanged
    property string id_left_channel_menu_item:               qsTrId("id_left_channel_menu_item") + TranslationManager.translationChanged
    property string id_right_channel_menu_item:              qsTrId("id_right_channel_menu_item") + TranslationManager.translationChanged
    property string id_save_waveform_menu_item:              qsTrId("id_save_waveform_menu_item") + TranslationManager.translationChanged
    property string id_exit_menu_item:                       qsTrId("id_exit_menu_item") + TranslationManager.translationChanged
    property string id_waveform_menu_item:                   qsTrId("id_waveform_menu_item") + TranslationManager.translationChanged
    property string id_restore_view_menu_item:               qsTrId("id_restore_view_menu_item") + TranslationManager.translationChanged
    property string id_reparse_menu_item:                    qsTrId("id_reparse_menu_item") + TranslationManager.translationChanged
    property string id_parser_settings_menu_item:            qsTrId("id_parser_settings_menu_item") + TranslationManager.translationChanged
    property string id_help_menu_item:                       qsTrId("id_help_menu_item") + TranslationManager.translationChanged
    property string id_about_menu_item:                      qsTrId("id_about_menu_item") + TranslationManager.translationChanged
    property string id_parser_settings_window_header:        qsTrId("id_parser_settings_window_header") + TranslationManager.translationChanged
    property string id_pilot_tone_settings:                  qsTrId("id_pilot_tone_settings") + TranslationManager.translationChanged
    property string id_pilot_half_frequency:                 qsTrId("id_pilot_half_frequency") + TranslationManager.translationChanged
    property string id_pilot_frequency:                      qsTrId("id_pilot_frequency") + TranslationManager.translationChanged
    property string id_pilot_delta:                          qsTrId("id_pilot_delta") + TranslationManager.translationChanged
    property string id_synchro_signal_settings:              qsTrId("id_synchro_signal_settings") + TranslationManager.translationChanged
    property string id_synchro_first_half_frequency:         qsTrId("id_synchro_first_half_frequency") + TranslationManager.translationChanged
    property string id_synchro_second_half_frequency:        qsTrId("id_synchro_second_half_frequency") + TranslationManager.translationChanged
    property string id_synchro_frequency:                    qsTrId("id_synchro_frequency") + TranslationManager.translationChanged
    property string id_synchro_delta:                        qsTrId("id_synchro_delta") + TranslationManager.translationChanged
    property string id_zero_digit_settings:                  qsTrId("id_zero_digit_settings") + TranslationManager.translationChanged
    property string id_zero_half_frequency:                  qsTrId("id_zero_half_frequency") + TranslationManager.translationChanged
    property string id_zero_frequency:                       qsTrId("id_zero_frequency") + TranslationManager.translationChanged
    property string id_zero_delta:                           qsTrId("id_zero_delta") + TranslationManager.translationChanged
    property string id_one_digit_settings:                   qsTrId("id_one_digit_settings") + TranslationManager.translationChanged
    property string id_one_half_frequency:                   qsTrId("id_one_half_frequency") + TranslationManager.translationChanged
    property string id_one_frequency:                        qsTrId("id_one_frequency") + TranslationManager.translationChanged
    property string id_one_delta:                            qsTrId("id_one_delta") + TranslationManager.translationChanged
    property string id_check_for_abnormal_sine_when_parsing: qsTrId("id_check_for_abnormal_sine_when_parsing") + TranslationManager.translationChanged
    property string id_please_choose_wav_file:               qsTrId("id_please_choose_wav_file") + TranslationManager.translationChanged
    property string id_please_choose_wfm_file:               qsTrId("id_please_choose_wfm_file") + TranslationManager.translationChanged
    property string id_please_choose_tap_file:               qsTrId("id_please_choose_tap_file") + TranslationManager.translationChanged
    property string id_wav_files:                            qsTrId("id_wav_files").arg(filename_wildcard + wav_file_suffix) + TranslationManager.translationChanged
    property string id_wfm_files:                            qsTrId("id_wfm_files").arg(filename_wildcard + wfm_file_suffix) + TranslationManager.translationChanged
    property string id_tap_files:                            qsTrId("id_tap_files").arg(filename_wildcard + tap_file_suffix) + TranslationManager.translationChanged
    property string id_save_tap_file:                        qsTrId("id_save_tap_file") + TranslationManager.translationChanged
    property string id_save_wfm_file:                        qsTrId("id_save_wfm_file") + TranslationManager.translationChanged
    property string id_vertical_zoom_in:                     qsTrId("id_vertical_zoom_in") + TranslationManager.translationChanged
    property string id_vertical_zoom_out:                    qsTrId("id_vertical_zoom_out") + TranslationManager.translationChanged
    property string id_horizontal_zoom_in:                   qsTrId("id_horizontal_zoom_in") + TranslationManager.translationChanged
    property string id_horizontal_zoom_out:                  qsTrId("id_horizontal_zoom_out") + TranslationManager.translationChanged
    property string id_waveform_shift_left:                  qsTrId("id_waveform_shift_left") + TranslationManager.translationChanged
    property string id_waveform_shift_right:                 qsTrId("id_waveform_shift_right") + TranslationManager.translationChanged
    property string id_reparse:                              qsTrId("id_reparse") + TranslationManager.translationChanged //Button caption
    property string id_save_parsed:                          qsTrId("id_save_parsed") + TranslationManager.translationChanged //Button caption
    property string id_save_waveform:                        qsTrId("id_save_waveform") + TranslationManager.translationChanged //Button caption
    property string id_restore_waveform:                     qsTrId("id_restore_waveform") + TranslationManager.translationChanged //Button caption
    property string id_repair_waveform:                      qsTrId("id_repair_waveform") + TranslationManager.translationChanged //Button caption
    property string id_shift_waveform:                       qsTrId("id_shift_waveform") + TranslationManager.translationChanged //Button caption
    property string id_goto_address:                         qsTrId("id_goto_address") + TranslationManager.translationChanged //Button caption
    property string id_selection_mode:                       qsTrId("id_selection_mode") + TranslationManager.translationChanged //Button caption
    property string id_measurement_mode:                     qsTrId("id_measurement_mode") + TranslationManager.translationChanged //Button caption
    property string id_copy_from_r_to_l:                     qsTrId("id_copy_from_r_to_l") + TranslationManager.translationChanged //Button caption
    property string id_copy_from_l_to_r:                     qsTrId("id_copy_from_l_to_r") + TranslationManager.translationChanged //Button caption
    property string id_left_channel:                         qsTrId("id_left_channel") + TranslationManager.translationChanged
    property string id_right_channel:                        qsTrId("id_right_channel") + TranslationManager.translationChanged
    property string id_to_the_beginning_of_the_block:        qsTrId("id_to_the_beginning_of_the_block") + TranslationManager.translationChanged //Button caption
    property string id_to_the_end_of_the_block:              qsTrId("id_to_the_end_of_the_block") + TranslationManager.translationChanged //Button caption
    property string id_block_number:                         qsTrId("id_block_number") + TranslationManager.translationChanged
    property string id_block_type:                           qsTrId("id_block_type") + TranslationManager.translationChanged
    property string id_block_name:                           qsTrId("id_block_name") + TranslationManager.translationChanged
    property string id_block_size:                           qsTrId("id_block_size") + TranslationManager.translationChanged
    property string id_block_status:                         qsTrId("id_block_status") + TranslationManager.translationChanged
    property string id_goto_suspicious_point:                qsTrId("id_goto_suspicious_point") + TranslationManager.translationChanged //Button caption
    property string id_remove_suspicious_point:              qsTrId("id_remove_suspicious_point") + TranslationManager.translationChanged //Button caption
    property string id_suspicious_point_number:              qsTrId("id_suspicious_point_number") + TranslationManager.translationChanged
    property string id_suspicious_point_position:            qsTrId("id_suspicious_point_position") + TranslationManager.translationChanged
    property string id_language_menu_item:                   qsTrId("id_language_menu_item") + TranslationManager.translationChanged
    property string id_hotkey_tooltip:                       qsTrId("id_hotkey_tooltip") + TranslationManager.translationChanged
    property string id_remove_action:                        qsTrId("id_remove_action") + TranslationManager.translationChanged //Button caption
    property string id_action_name:                          qsTrId("id_action_name") + TranslationManager.translationChanged
    property string id_sine_check_tolerance:                 qsTrId("id_sine_check_tolerance") + TranslationManager.translationChanged
    property string id_play_parsed_data:                     qsTrId("id_play_parsed_data") + TranslationManager.translationChanged
    property string id_stop_playing_parsed_data:             qsTrId("id_stop_playing_parsed_data") + TranslationManager.translationChanged
    property string id_playing_parsed_data_window_header:    qsTrId("id_playing_parsed_data_window_header") + TranslationManager.translationChanged
}
