#ifdef BOOT_EYES
    #include "BootEyes.h"
    #include "EyeRenderer.h"
    #include "WakeScript.h"
    #include <Arduino.h>
    #include <new>

namespace {
constexpr int kLeftScreen = 1;  // orb 2
constexpr int kRightScreen = 3; // orb 4
// Longest wait for the finale in end(): connected at t ~ 0, then 2.6 s to the
// first crack open plus the 2 s finale
constexpr uint32_t kFinaleTimeoutMs = 6000;

ScreenManager *s_sm = nullptr;
EyeRenderer *s_renderer = nullptr;
WakeScript s_script;
SemaphoreHandle_t s_lock = nullptr;    // guards s_script: the task reads, finish() writes
SemaphoreHandle_t s_screens = nullptr; // the SPI bus: eye task vs. start-up text
TaskHandle_t s_task = nullptr;
volatile bool s_stop = false;
volatile bool s_exited = false;

EyePose currentPose() {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    const EyePose p = s_script.at(millis());
    xSemaphoreGive(s_lock);
    return p;
}

// One eye, holding the bus for the whole frame. false = aborted mid-frame.
bool drawEye(int screen, uint8_t side, const EyePose &pose) {
    xSemaphoreTake(s_screens, portMAX_DELAY);
    s_sm->selectScreen(screen);
    const bool ok = s_renderer->draw(side, pose, &s_stop);
    xSemaphoreGive(s_screens);
    return ok;
}

void eyeTask(void *) {
    while (!s_stop) {
        const EyePose pose = currentPose();
        if (!drawEye(kLeftScreen, 0, pose) || !drawEye(kRightScreen, 1, pose)) {
            break;
        }
        vTaskDelay(1); // let the idle task feed the watchdog, and orb 5 a turn
    }
    s_exited = true; // last touch of shared state
    vTaskDelete(nullptr);
}

void stopTask() {
    s_stop = true; // checked between lines, never mid-transaction
    while (!s_exited) {
        delay(5);
    }
}

void release() {
    delete s_renderer;
    s_renderer = nullptr;
    if (s_lock) {
        vSemaphoreDelete(s_lock);
        s_lock = nullptr;
    }
    if (s_screens) {
        vSemaphoreDelete(s_screens);
        s_screens = nullptr;
    }
    s_task = nullptr;
}
} // namespace

bool BootEyes::start(ScreenManager &sm, TFT_eSPI &tft) {
    s_sm = &sm;
    s_lock = xSemaphoreCreateMutex();
    s_screens = xSemaphoreCreateMutex();
    s_renderer = new (std::nothrow) EyeRenderer(tft);
    s_stop = false;
    s_exited = false;
    if (s_lock && s_screens && s_renderer) {
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
        return; // the task plays the finale, then the awake loop, until end()
    }
    stopTask();
    s_sm->clearScreen(kLeftScreen);
    s_sm->clearScreen(kRightScreen);
    release();
}

void BootEyes::end() {
    if (!s_task) {
        return;
    }
    const uint32_t t0 = millis();
    while (!s_exited && !currentPose().done && millis() - t0 < kFinaleTimeoutMs) {
        delay(10); // a quick data load still gets the whole finale
    }
    stopTask();
    release();
}

void BootEyes::lockScreens() {
    if (s_screens) {
        xSemaphoreTake(s_screens, portMAX_DELAY);
    }
}

void BootEyes::unlockScreens() {
    if (s_screens) {
        xSemaphoreGive(s_screens);
    }
}
#endif // BOOT_EYES
