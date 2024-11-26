#pragma once

#include "displayapp/apps/Apps.h"
#include "components/settings/Settings.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/widgets/Counter.h"
#include "displayapp/Controllers.h"
#include "systemtask/SystemTask.h"
#include "systemtask/WakeLock.h"
#include "Symbols.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class Sleep : public Screen {
      public:
        explicit Sleep(Controllers::InfiniSleepController& infiniSleepController,
                       Controllers::Settings::ClockType clockType,
                       System::SystemTask& systemTask,
                       Controllers::MotorController& motorController);
        ~Sleep() override;
        void Refresh() override;
        void SetAlerting();
        void RedrawSetAlerting();
        void OnButtonEvent(lv_obj_t* obj, lv_event_t event);
        bool OnButtonPushed() override;
        bool OnTouchEvent(TouchEvents event) override;
        void OnValueChanged();
        void StopAlerting(bool setSwitch = true);
        void SnoozeWakeAlarm();
        void UpdateDisplay();
        enum class SleepDisplayState { Alarm, Info, Settings };
        SleepDisplayState displayState = SleepDisplayState::Info;
        SleepDisplayState lastDisplayState = SleepDisplayState::Info;

        Controllers::InfiniSleepController& infiniSleepController;

      private:
        System::WakeLock wakeLock;
        Controllers::MotorController& motorController;
        Controllers::Settings::ClockType clockType;

        lv_obj_t *btnStop, *txtStop, /**btnRecur, *txtRecur,*/ *btnInfo, *enableSwitch;
        lv_obj_t *trackerToggleBtn, *trackerToggleLabel;
        lv_obj_t* lblampm = nullptr;
        lv_obj_t* txtMessage = nullptr;
        lv_obj_t* btnMessage = nullptr;
        lv_task_t* taskSnoozeWakeAlarm = nullptr;

        lv_task_t* taskRefresh = nullptr;

        lv_task_t* taskPressesToStopAlarmTimeout = nullptr;

        // enum class EnableButtonState { On, Off, Alerting };
        void DisableWakeAlarm();
        void SetSwitchState(lv_anim_enable_t anim);
        void SetWakeAlarm();
        void ShowAlarmInfo();
        void HideAlarmInfo();
        void UpdateWakeAlarmTime();
        Widgets::Counter hourCounter = Widgets::Counter(0, 23, jetbrains_mono_76);
        Widgets::Counter minuteCounter = Widgets::Counter(0, 59, jetbrains_mono_76);

        void DrawAlarmScreen();
        void DrawInfoScreen();
        void DrawSettingsScreen();

        bool alreadyAlerting = false;

        lv_obj_t* label_hr;
        lv_obj_t* label_start_time;
        lv_obj_t* label_alarm_time;
        lv_obj_t* label_gradual_wake;
        lv_obj_t* label_total_sleep;
        lv_obj_t* label_sleep_cycles;
        lv_obj_t *btnSuggestedAlarm, *txtSuggestedAlarm;
      };
    }

    template <>
    struct AppTraits<Apps::Sleep> {
      static constexpr Apps app = Apps::Sleep;
      static constexpr const char* icon = Screens::Symbols::bed;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::Sleep(controllers.infiniSleepController,
                                  controllers.settingsController.GetClockType(),
                                  *controllers.systemTask,
                                  controllers.motorController);
      }
    };
  }
}