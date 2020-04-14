#include "InputManager.h"
#include <windows.h>
#include <iostream>
void InputManager::get_input() {
    MSG message = { 0 };
    for (int i = 0; i != 0xFF; i++) { // if the current state of a key is released, change it to unheld. released is only supposed to exist for one frame
        if (m_keystate[i].m_curr_state == KeyState::RELEASED) {
            m_keystate[i].m_curr_state = KeyState::UNHELD;
            m_keystate[i].m_prev_state = KeyState::RELEASED;
        }
    }
    while (PeekMessage(&message, m_hwnd, 0, 0, PM_REMOVE | PM_QS_INPUT) != 0) { // there's a message in the queue
        switch (message.message) {
        case WM_KEYDOWN:
            process_keydown(message);
            break;
        case WM_KEYUP:
           process_keyup(message);
            break;

        }
    }
}
inline const InputManager::KeyState InputManager::get_state_of_key(uint8_t index) const{
    return m_keystate[index].m_curr_state;
}
inline const InputManager::KeyState InputManager::get_prev_state_of_key(uint8_t index) const {
    return m_keystate[index].m_prev_state;
}
void InputManager::process_keydown(const MSG& message) {
    if (message.wParam <= 0xFF) { // make sure we are inbound of the array
        KeyState state = m_keystate[message.wParam].m_curr_state; // store the current state of the key
        uint64_t time = m_keystate[message.wParam].m_timestamp;
        switch (state) {
        case KeyState::PRESSED: // can become held if enough time has passed // only need to change the timestamp when the current time changes
            if (message.time - time >= m_state_delay) {
                m_keystate[message.wParam].m_curr_state = KeyState::HELD;
                m_keystate[message.wParam].m_prev_state = KeyState::PRESSED;
                m_keystate[message.wParam].m_timestamp = message.time;
            }
            return;
        case KeyState::HELD: // can only stay held, if the current state is held the previous state can be pressed so that needs to be updated
            m_keystate[message.wParam].m_prev_state = KeyState::HELD;
            return;
        case KeyState::RELEASED: // can get here if the frame rate is low and we are spamming a key
            m_keystate[message.wParam].m_prev_state = KeyState::HELD;
            m_keystate[message.wParam].m_prev_state = KeyState::RELEASED;
            m_keystate[message.wParam].m_timestamp = message.time;
            return;
        case KeyState::UNHELD: // can become pressed or stay unheld
            m_keystate[message.wParam].m_curr_state = KeyState::PRESSED;
            m_keystate[message.wParam].m_prev_state = KeyState::UNHELD;
            m_keystate[message.wParam].m_timestamp = message.time;
            return;
        }
    }
}
void InputManager::process_keyup(const MSG& message) {
    if (message.wParam <= 0xFF) {
        KeyState state = m_keystate[message.wParam].m_curr_state; // store the current state of the key
        uint64_t time = m_keystate[message.wParam].m_timestamp;
        switch (state) {
        case KeyState::PRESSED: // can become held or released (cannot become pressed because that is a single frame action
            m_keystate[message.wParam].m_curr_state = KeyState::RELEASED; // set the current state
            m_keystate[message.wParam].m_prev_state = KeyState::PRESSED; // set the previos state
            m_keystate[message.wParam].m_timestamp = message.time; // set the time this action took place
            return;
        case KeyState::HELD: // can become released or stay held
            m_keystate[message.wParam].m_curr_state = KeyState::RELEASED;
            m_keystate[message.wParam].m_prev_state = KeyState::HELD;
            m_keystate[message.wParam].m_timestamp = message.time;
            return;
        case KeyState::RELEASED: // can get here if the frame rate is low and we are spamming a key
            m_keystate[message.wParam].m_curr_state = KeyState::UNHELD;
            m_keystate[message.wParam].m_prev_state = KeyState::RELEASED;
            m_keystate[message.wParam].m_timestamp = message.time;
            return;
        case KeyState::UNHELD: // can  only stay unheld 
            m_keystate[message.wParam].m_prev_state = KeyState::UNHELD; 
            return;
        }
    }

}