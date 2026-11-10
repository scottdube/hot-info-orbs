#ifdef BOOT_EYES
    #include "BootEyes.h"
    #include "EyeRenderer.h"
    #include "WakeScript.h"
    #include <Arduino.h>
    #include <new>

namespace {
constexpr int kLeftScreen = 1;  // orb 2
constexpr int kRightScreen = 3; // orb 4
// Longest legitimate wait: connected at t ~ 0, then 2.6 s to the first crack
// open plus the 2 s finale
constexpr uint32_t kFinishTimeoutMs = 6000;

ScreenManager *s_sm = nullptr;
EyeRenderer *s_renderer = nullptr;
WakeScript s_script;
SemaphoreHandle_t s_lock = nullptr; // guards s_script: the task reads, finish() writes
TaskHandle_t s_task = nullptr;
volatile bool s_stop = false;
volatile bool s_exited = false;

EyePose currentPose() {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    const EyePose p = s_script.at(millis());
    xSemaphoreGive(s_lock);
    return p;
}

void eyeTask(void *) {
    while (!s_stop) {
        const EyePose pose = currentPose();
        s_sm->selectScreen(kLeftScreen);
        if (!s_renderer->draw(0, pose, &s_stop)) {
            break;
        }
        s_sm->selectScreen(kRightScreen);
        if (!s_renderer->draw(1, pose, &s_stop)) {
            break;
        }
        if (pose.done) {
            break;
        }
        vTaskDelay(1); // let the idle task feed the watchdog
    }
    s_exited = true; // last touch of shared state
    vTaskDelete(nullptr);
}

void release() {
    delete s_renderer;
    s_renderer = nullptr;
    if (s_lock) {
        vSemaphoreDelete(s_lock);
        s_lock = nullptr;
    }
    s_task = nullptr;
}
} // namespace

bool BootEyes::start(ScreenManager &sm, TFT_eSPI &tft) {
    s_sm = &sm;
    s_lock = xSemaphoreCreateMutex();
    s_renderer = new (std::nothrow) EyeRenderer(tft);
    s_stop = false;
    s_exited = false;
    if (s_lock && s_renderer) {
        s_script.begin(millis());
        // Core 1 beside loopTask (blocked in autoConnect); WiFi runs on core 0
        if (xTaskCreatePinnedToCore(eyeTask, "booteyes", 6144, nullptr, 1, &s_task, 1) == pdPASS) {
            return true;
        }
    }
    Serial.println("Boot eyes: could not start (out of memory)");
    release();
    return false;
}

void BootEyes::finish(bool connected) {
    if (!s_task) {
        return;
    }
    if (connected) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_script.connected(millis());
        xSemaphoreGive(s_lock);
        const uint32_t t0 = millis();
        while (!s_exited && millis() - t0 < kFinishTimeoutMs) {
            delay(10);
        }
        if (!s_exited) {
            Serial.println("Boot eyes: finale overran, stopping");
        }
    }
    s_stop = true; // checked between lines, never mid-transaction
    while (!s_exited) {
        delay(5);
    }
    if (!connected) {
        s_sm->clearScreen(kLeftScreen);
        s_sm->clearScreen(kRightScreen);
    }
    release();
}
#endif // BOOT_EYES
