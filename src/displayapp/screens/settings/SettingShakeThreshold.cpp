#include "displayapp/screens/settings/SettingShakeThreshold.h"
#include <lvgl/lvgl.h>
#include "displayapp/DisplayApp.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/InfiniTimeTheme.h"
#include <components/motion/MotionController.h>
#include <components/settings/Settings.h>
#include "systemtask/SystemTask.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void event_handler(lv_obj_t* obj, lv_event_t event) {
    SettingShakeThreshold* screen = static_cast<SettingShakeThreshold*>(obj->user_data);
    screen->UpdateSelected(obj, event);
  }
}

SettingShakeThreshold::SettingShakeThreshold(Controllers::Settings& settingsController,
                                             Controllers::MotionController& motionController,
                                             System::SystemTask& systemTask)
  : settingsController {settingsController}, motionController {motionController}, systemTask {systemTask} {
  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, "Wake Sensitivity");
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);

  if (settingsController.GetNotificationStatus() == Controllers::Settings::Notification::Sleep) {
    lv_obj_t* explanation = lv_label_create(lv_scr_act(), nullptr);
    lv_label_set_long_mode(explanation, LV_LABEL_LONG_BREAK);
    lv_label_set_align(explanation, LV_LABEL_ALIGN_AUTO);
    lv_obj_set_width(explanation, LV_HOR_RES_MAX);
    lv_label_set_text_static(
      explanation,
      "\nShake detector is disabled in sleep mode, and will neither wake up the watch nor calibrate.\nDisable sleep mode to calibrate.");
    currentCalibrationStep = CalibrationStep::Disabled;
    lv_obj_align(explanation, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    return;
  }

  positionArc = lv_arc_create(lv_scr_act(), nullptr);
  positionArc->user_data = this;

  lv_obj_set_event_cb(positionArc, event_handler);
  lv_arc_set_bg_angles(positionArc, 180, 360);
  lv_arc_set_range(positionArc, 0, 4095);
  lv_arc_set_adjustable(positionArc, true);
  lv_obj_set_width(positionArc, lv_obj_get_width(lv_scr_act()) - 10);
  lv_obj_set_height(positionArc, 240);
  lv_obj_align(positionArc, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

  animArc = lv_arc_create(positionArc, positionArc);
  lv_arc_set_adjustable(animArc, false);
  lv_obj_set_width(animArc, lv_obj_get_width(positionArc));
  lv_obj_set_height(animArc, lv_obj_get_height(positionArc));
  lv_obj_align_mid(animArc, positionArc, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_line_opa(animArc, LV_ARC_PART_BG, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_line_opa(animArc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, LV_OPA_70);
  lv_obj_set_style_local_line_opa(animArc, LV_ARC_PART_KNOB, LV_STATE_DEFAULT, LV_OPA_0);
  lv_obj_set_style_local_line_color(animArc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_obj_set_style_local_bg_color(animArc, LV_ARC_PART_BG, LV_STATE_CHECKED, LV_COLOR_TRANSP);

  animArc->user_data = this;
  lv_obj_set_click(animArc, false);

  calButton = lv_btn_create(lv_scr_act(), nullptr);
  calButton->user_data = this;
  lv_obj_set_event_cb(calButton, event_handler);
  lv_obj_set_height(calButton, 80);
  lv_obj_set_width(calButton, 200);
  lv_obj_align(calButton, lv_scr_act(), LV_ALIGN_IN_BOTTOM_MID, 0, 0);
  lv_btn_set_checkable(calButton, true);
  calLabel = lv_label_create(calButton, nullptr);
  lv_label_set_text_static(calLabel, "Calibrate");

  lv_arc_set_value(positionArc, settingsController.GetShakeThreshold());

  vDecay = xTaskGetTickCount();
  oldWakeupModeShake = settingsController.isWakeUpModeOn(Pinetime::Controllers::Settings::WakeUpMode::Shake);
  // Enable shake to wake for the calibration
  settingsController.setWakeUpMode(Pinetime::Controllers::Settings::WakeUpMode::Shake, true);
  refreshTask = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

SettingShakeThreshold::~SettingShakeThreshold() {
  if (currentCalibrationStep != CalibrationStep::Disabled) {
    // Don't set new threshold if calibration is disabled due to sleep mode
    settingsController.SetShakeThreshold(lv_arc_get_value(positionArc));
    // Restore previous wakeup mode which is only changed if calibration is not disabled due to sleep mode
    settingsController.setWakeUpMode(Pinetime::Controllers::Settings::WakeUpMode::Shake, oldWakeupModeShake);
    settingsController.SaveSettings();
    lv_task_del(refreshTask);
  }

  lv_obj_clean(lv_scr_act());
}

void SettingShakeThreshold::Refresh() {
  if (currentCalibrationStep == CalibrationStep::GettingReady) {
    if (xTaskGetTickCount() - vCalTime > pdMS_TO_TICKS(2000)) {
      vCalTime = xTaskGetTickCount();
      currentCalibrationStep = CalibrationStep::Calibrating;
      lv_obj_set_style_local_bg_color(calButton, LV_BTN_PART_MAIN, LV_STATE_CHECKED, LV_COLOR_RED);
      lv_obj_set_style_local_bg_color(calButton, LV_BTN_PART_MAIN, LV_STATE_CHECKED, LV_COLOR_RED);
      lv_label_set_text_static(calLabel, "Shake!");
    }
  }
  if (currentCalibrationStep == CalibrationStep::Calibrating) {

    if ((motionController.CurrentShakeSpeed() - 300) > lv_arc_get_value(positionArc)) {
      lv_arc_set_value(positionArc, (int16_t) motionController.CurrentShakeSpeed() - 300);
    }
    if (xTaskGetTickCount() - vCalTime > pdMS_TO_TICKS(7500)) {
      lv_btn_set_state(calButton, LV_STATE_DEFAULT);
      lv_event_send(calButton, LV_EVENT_VALUE_CHANGED, nullptr);
    }
  }
  if (motionController.CurrentShakeSpeed() - 300 > lv_arc_get_value(animArc)) {
    lv_arc_set_value(animArc, (uint16_t) motionController.CurrentShakeSpeed() - 300);
    vDecay = xTaskGetTickCount();
  } else if ((xTaskGetTickCount() - vDecay) > pdMS_TO_TICKS(1500)) {
    lv_arc_set_value(animArc, lv_arc_get_value(animArc) - 25);
  }
}

void SettingShakeThreshold::UpdateSelected(lv_obj_t* object, lv_event_t event) {
  switch (event) {
    case LV_EVENT_VALUE_CHANGED: {
      if (object == calButton) {
        if (lv_btn_get_state(calButton) == LV_BTN_STATE_CHECKED_RELEASED 
            && currentCalibrationStep == CalibrationStep::NotCalibrated) {
          lv_arc_set_value(positionArc, 0);
          currentCalibrationStep = CalibrationStep::GettingReady;
          vCalTime = xTaskGetTickCount();
          lv_label_set_text_static(calLabel, "Ready!");
          lv_obj_set_click(positionArc, false);
          lv_obj_set_style_local_bg_color(calButton, LV_BTN_PART_MAIN, LV_STATE_CHECKED, Colors::highlight);
        } else if (lv_btn_get_state(calButton) == LV_BTN_STATE_RELEASED) {
          currentCalibrationStep = CalibrationStep::NotCalibrated;
          lv_obj_set_click(positionArc, true);
          lv_label_set_text_static(calLabel, "Calibrate");
        }
        break;
      }
    }
  }
}
