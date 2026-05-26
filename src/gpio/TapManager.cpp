
#include "gpio/TapManager.h"

bool TapManager::begin() {
    if (m_pin < 0) {
        return false;
    }
    pinMode((uint8_t)m_pin, INPUT_PULLUP);
    return true;
}

void TapManager::onTapChange(std::function<void(bool, unsigned long)> callback) {
    _tapChangeCallbacks.push_back(callback);
}
void TapManager::onTapChange(std::function<void(bool)> callback) {
    _tapChangeCallbacks.push_back([callback](bool isTapped, unsigned long) { callback(isTapped); });
}

void TapManager::onSingleTap(std::function<void(unsigned long)> callback) { _singleTapCallbacks.push_back(callback); }
void TapManager::onSingleTap(std::function<void()> callback) {
    _singleTapCallbacks.push_back([callback](unsigned long) { callback(); });
}

void TapManager::onDoubleTap(std::function<void(unsigned long)> callback) { _doubleTapCallbacks.push_back(callback); }
void TapManager::onDoubleTap(std::function<void()> callback) {
    _doubleTapCallbacks.push_back([callback](unsigned long) { callback(); });
}

void TapManager::onLongPress(std::function<void(unsigned long)> callback) { _longPressCallbacks.push_back(callback); }
void TapManager::onLongPress(std::function<void()> callback) {
    _longPressCallbacks.push_back([callback](unsigned long) { callback(); });
}

void TapManager::nextTick() {
    if (m_pin < 0) {
        return;
    }
    const bool currentlyTapped = digitalRead((uint8_t)m_pin) == HIGH;
    const unsigned long now = millis();

    if (currentlyTapped != m_isTapped) {
        m_isTapped = currentlyTapped;
        m_lastTapChangeTimestamp = now;

        for (const auto& callback : _tapChangeCallbacks) {
            callback(m_isTapped, now);
        }

        if (m_isTapped) {
            if (m_waitingForSecondTap && (now - m_lastTapTimestamp <= DOUBLE_TAP_MAX_DELAY_MS)) {
                for (const auto& callback : _doubleTapCallbacks) {
                    callback(now);
                }
                m_waitingForSecondTap = false;
            } else {
                m_waitingForSecondTap = true;
            }
            m_lastTapTimestamp = now;
            m_longPressActive = false;
        } else {
            if (m_waitingForSecondTap && (now - m_lastTapTimestamp > DOUBLE_TAP_MAX_DELAY_MS)) {
                for (const auto& callback : _singleTapCallbacks) {
                    callback(m_lastTapTimestamp);
                }
                m_waitingForSecondTap = false;
            }
        }
    } else if (m_isTapped && !m_longPressActive && (now - m_lastTapChangeTimestamp >= LONG_PRESS_DELAY_MS)) {
        m_longPressActive = true;
        for (const auto& callback : _longPressCallbacks) {
            callback(now);
        }
    }
}
