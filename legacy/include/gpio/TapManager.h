#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

class TapManager {
   public:
    TapManager(int pin) : m_pin(pin) {}

    bool begin();

    void onTapChange(std::function<void(bool, unsigned long)> callback);
    void onTapChange(std::function<void(bool)> callback);

    void onSingleTap(std::function<void(unsigned long)> callback);
    void onSingleTap(std::function<void()> callback);

    void onDoubleTap(std::function<void(unsigned long)> callback);
    void onDoubleTap(std::function<void()> callback);

    void onLongPress(std::function<void(unsigned long)> callback);
    void onLongPress(std::function<void()> callback);

    void nextTick();

   private:
    int m_pin = -1;
    bool m_isTapped = false;
    unsigned long m_lastTapTimestamp = 0;
    unsigned long m_lastTapChangeTimestamp = 0;
    bool m_waitingForSecondTap = false;
    bool m_longPressActive = false;
    static constexpr unsigned long DOUBLE_TAP_MAX_DELAY_MS = 500;
    static constexpr unsigned long LONG_PRESS_DELAY_MS = 1000;

    std::vector<std::function<void(bool, unsigned long)>> _tapChangeCallbacks;
    std::vector<std::function<void(unsigned long)>> _singleTapCallbacks;
    std::vector<std::function<void(unsigned long)>> _doubleTapCallbacks;
    std::vector<std::function<void(unsigned long)>> _longPressCallbacks;
};