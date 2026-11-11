#ifdef BOOT_EYES
    #include "BootEyes.h"
    #include "EyeRenderer.h"
    #include "LidPalette.h"
    #include "Utils.h" // ScreenCenterX/Y
    #include "WakeScript.h"
    #include <Arduino.h>
    #include <WiFi.h>
    #include <new>

namespace {
constexpr int kLeftScreen = 1;  // orb 2
constexpr int kRightScreen = 3; // orb 4
constexpr int kLabelScreen = 4; // orb 5: the style name in the lid demo
// Longest wait for the finale in end(): connected at t ~ 0, then 2.6 s to the
// first crack open plus the 2 s finale
constexpr uint32_t kFinaleTimeoutMs = 6000;
// WiFi didn't connect (issue #20): wide open, looking at the tip of the nose,
// so the irises sit in the lower inner corners (as cross-eyed cartoons and
// photos draw it). Side 0 (orb 2) looks to the viewer's right, side 1 (orb 4)
// to the left. Rig history 2026-11-11: {150, 873} at y 470 read as barely
// crossed; 48 px in at y 470 read as a sideways glance, not crossed.
// Scott's reference photo: the eye turns, so the iris is an upright oval
// tucked into the inner corner, only a little below level. Sliding the full
// circle in (5f7886d) had the wrong perspective.
constexpr int16_t kCrossedGazeX[2] = {0, 1023}; // 40 source px in (the window's edge)
constexpr int16_t kCrossedGazeY = 300;          // 15 source px down
constexpr uint8_t kCrossedTurn16 = 26;          // iris 16/26 = 62% as wide: turned ~52 degrees
// Lids wider than tracking: surprised. At 256 (tracking) they read as a squint
// on the rig, because the upper lid follows the pupil and the pupil is aside.
constexpr uint16_t kCrossedOpen = 256 + 160;
// Frames to draw: the renderer filters upper-lid tracking over a few frames
constexpr int kCrossedFrames = 6;

ScreenManager *s_sm = nullptr;
EyeRenderer *s_renderer = nullptr;
WakeScript s_script;
SemaphoreHandle_t s_lock = nullptr;    // guards s_script: the task reads, finish() writes
SemaphoreHandle_t s_screens = nullptr; // the SPI bus: eye task vs. start-up text
TaskHandle_t s_task = nullptr;
volatile bool s_stop = false;
volatile bool s_exited = false;
bool s_demo = false;
uint32_t s_demoStart = 0;
int s_demoShown = -1;      // style whose label is on orb 5
bool s_demoShownIp = false; // the label has the IP address on it

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

// The demo's caption on orb 5, redrawn with each new style
void drawDemoLabel(int style) {
    xSemaphoreTake(s_screens, portMAX_DELAY);
    s_sm->selectScreen(kLabelScreen);
    s_sm->fillScreen(TFT_BLACK);
    s_sm->setFontColor(TFT_LIGHTGREY);
    s_sm->drawCenterString("Eye lids", ScreenCenterX, ScreenCenterY - 55, 16);
    s_sm->setFontColor(TFT_WHITE);
    s_sm->drawCenterString(lids::kStyles[style].name, ScreenCenterX, ScreenCenterY, 30);
    s_sm->setFontColor(TFT_LIGHTGREY);
    char n[8];
    snprintf(n, sizeof(n), "%d of %d", style + 1, lids::kCount);
    s_sm->drawCenterString(n, ScreenCenterX, ScreenCenterY + 40, 14);
    if (WiFi.isConnected()) { // where the settings page is, to leave the demo
        s_sm->drawCenterString(WiFi.localIP().toString(), ScreenCenterX, ScreenCenterY + 70, 14);
    }
    s_sm->setFontColor(TFT_WHITE);
    xSemaphoreGive(s_screens);
}

// One whole wake (asleep, groggy, finale, awake) per style, from the top
EyePose demoPose() {
    const uint32_t t = millis() - s_demoStart;
    const int style = lids::demoStyle(t);
    const bool ip = WiFi.isConnected();
    if (style != s_demoShown || ip != s_demoShownIp) {
        s_demoShown = style;
        s_demoShownIp = ip;
        s_renderer->setStyle(style);
        drawDemoLabel(style);
    }
    WakeScript w;
    w.begin(0);
    w.connected(lids::kDemoConnectMs);
    return w.at(lids::demoCycleTime(t));
}

void eyeTask(void *) {
    while (!s_stop) {
        const EyePose pose = s_demo ? demoPose() : currentPose();
        if (!drawEye(kLeftScreen, 0, pose) || !drawEye(kRightScreen, 1, pose)) {
            break;
        }
        vTaskDelay(1); // let the idle task feed the watchdog, and orb 5 a turn
    }
    s_exited = true; // last touch of shared state
    vTaskDelete(nullptr);
}

// One still frame per eye, drawn after the task has stopped
void drawCrossed() {
    static const volatile bool kNever = false;
    s_renderer->setIrisTurn(kCrossedTurn16);
    for (int f = 0; f < kCrossedFrames; f++) {
        for (uint8_t side = 0; side < 2; side++) {
            s_sm->selectScreen(side ? kRightScreen : kLeftScreen);
            s_renderer->draw(side, {kCrossedOpen, kCrossedGazeX[side], kCrossedGazeY, false}, &kNever);
        }
    }
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

bool BootEyes::start(ScreenManager &sm, TFT_eSPI &tft, const Look &look) {
    s_sm = &sm;
    s_lock = xSemaphoreCreateMutex();
    s_screens = xSemaphoreCreateMutex();
    s_renderer = new (std::nothrow) EyeRenderer(tft);
    s_stop = false;
    s_exited = false;
    if (s_lock && s_screens && s_renderer) {
        s_renderer->setIris(look.iris);
        s_renderer->setLashes(look.lashes);
        s_demo = look.lids == lids::kDemo;
        if (s_demo) {
            s_demoStart = millis();
            s_demoShown = -1;
        } else {
            s_renderer->setStyle(look.lids);
        }
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
    drawCrossed();
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

bool BootEyes::demo() { return s_task != nullptr && s_demo; }

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
