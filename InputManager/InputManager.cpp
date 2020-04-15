#include "InputManager.h"
#include <windows.h>
#include <Windows.h>
#include <windowsx.h>
#include <iostream>
#include <chrono>

// public functions:
void InputManager::get_input() {
	update_key_states(); // update the states of the relative buttons before we read in more input

	MSG message = { 0 };
	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) != 0) { // there's a message in the queue
		TranslateMessage(&message); // translate the message into 
		DispatchMessageA(&message); // dispatch the messages we dont handle to the window procedure so we dont need to worry about the nitty gritty
		uint32_t index = -1;
		switch (message.message) {
		case WM_MOUSEMOVE:
			process_mousemove(message);
			break;
		case WM_LBUTTONDOWN:
			process_mousedown(message, static_cast<uint32_t>(MouseButton::LEFT));
			break;
		case WM_MBUTTONDOWN:
			process_mousedown(message, static_cast<uint32_t>(MouseButton::MIDDLE));
			break;
		case WM_RBUTTONDOWN:
			process_mousedown(message, static_cast<uint32_t>(MouseButton::RIGHT));
			break;
		case WM_LBUTTONUP:
			process_mouseup(message, static_cast<uint32_t>(MouseButton::LEFT));
			break;
		case WM_MBUTTONUP:
			process_mouseup(message, static_cast<uint32_t>(MouseButton::MIDDLE));
			break;
		case WM_RBUTTONUP:
			process_mouseup(message, static_cast<uint32_t>(MouseButton::RIGHT));
			break;
		//case WM_MOUSEWHEEL: 
		//	process_mousewheel(message);
		//	break;
		case WM_KEYDOWN:
			process_keydown(message);
			break;
		case WM_KEYUP:
			process_keyup(message);
			break;
		case WM_CHAR: // used for generic typing, not for game input that we check the state of
			process_char(message);
			break;
		case WM_DEADCHAR: // eventually use for i18n
			break;
		}
	}
}
const InputManager::KeyState InputManager::get_state(uint8_t index) const {
	return static_cast<const KeyState>(m_keystate[index].m_curr_state);
}
const InputManager::KeyState InputManager::get_state(MouseButton index) const {
	return static_cast<const KeyState>(m_mouse.m_buttons[static_cast<uint32_t>(index)].m_curr_state);
}
const InputManager::KeyState InputManager::get_prev_state(uint8_t index) const {
	return static_cast<const KeyState>(m_keystate[index].m_prev_state);
}
const InputManager::KeyState InputManager::get_prev_state(MouseButton index) const {
	return static_cast<const KeyState>(m_mouse.m_buttons[static_cast<uint32_t>(index)].m_prev_state);
}
float InputManager::get_mouse_x() const {
	return m_mouse.x;
}
float InputManager::get_mouse_y() const {
	return m_mouse.y;
}
// private functions:
void InputManager::update_key_states() { // used to make sure released only exists for one frame, and to make sure that we properly transition to held for the mouse buttons
	memset(m_character_pressed, 0, 256 * sizeof(uint8_t)); // compiler should make this super fast

	uint64_t curr_time = GetTickCount64();
	for (int i = 0; i != 0xFF; i++) { // if the current state of a key is released, change it to unheld. released is only supposed to exist for one frame
		switch (m_keystate[i].m_curr_state) {
		case KeyState::RELEASED:
			m_keystate[i].m_curr_state = KeyState::UNHELD;
			m_keystate[i].m_prev_state = KeyState::RELEASED;
			break;
		case KeyState::PRESSED: // update the state to pressed here too because we have to do it for the mouse anyway and it should be consistent
			if (curr_time - m_keystate[i].m_timestamp >= m_held_delay) { // message clock uses GetTickCount which is milliseconds since startup
				m_keystate[i].m_curr_state = KeyState::HELD;
				m_keystate[i].m_timestamp = curr_time;
			}
			m_keystate[i].m_prev_state = KeyState::PRESSED;
			break;
		}
	}

	for (int i = 0; i != m_mouse.m_button_count; i++) { // if the current state of a mouse button is released, change it to unheld
		switch (m_mouse.m_buttons[i].m_curr_state) {
		case KeyState::RELEASED:
			m_mouse.m_buttons[i].m_curr_state = KeyState::UNHELD;
			m_mouse.m_buttons[i].m_prev_state = KeyState::RELEASED;
			break;
		case KeyState::PRESSED: // if the current state of a mouse button is released, change it to unheld
			if (curr_time - m_mouse.m_buttons[i].m_timestamp >= m_held_delay) { // message clock uses GetTickCount which is milliseconds since startup
				m_mouse.m_buttons[i].m_curr_state = KeyState::HELD;
				m_mouse.m_buttons[i].m_timestamp = curr_time;
			}
			m_mouse.m_buttons[i].m_prev_state = KeyState::PRESSED;
			break;
		}
	}
}
void InputManager::update_mouse_pos(const MSG& message) {
	m_mouse.x = GET_X_LPARAM(message.lParam);
	m_mouse.y = GET_Y_LPARAM(message.lParam);
}
void InputManager::process_mousemove(const MSG& message) {
	update_mouse_pos(message);
}
void InputManager::process_mousedown(const MSG& message, uint32_t index) {
	update_mouse_pos(message);
	KeyState state = m_mouse.m_buttons[index].m_curr_state;
	uint64_t time = m_mouse.m_buttons[index].m_timestamp;
	switch (state) { // dont need to process PRESSED here because only a single mousedown message is sent when we click the mouse
	case KeyState::HELD: // can only stay held, if the current state is held the previous state can be pressed so that needs to be updated
		m_mouse.m_buttons[index].m_prev_state = KeyState::HELD;
		return;
	case KeyState::RELEASED: // can get here if the frame rate is low and we are spamming a key
		m_mouse.m_buttons[index].m_curr_state = KeyState::UNHELD;
		m_mouse.m_buttons[index].m_prev_state = KeyState::RELEASED;
		m_mouse.m_buttons[index].m_timestamp = message.time;
		return;
	case KeyState::UNHELD: // key pressed
		m_mouse.m_buttons[index].m_curr_state = KeyState::PRESSED;
		m_mouse.m_buttons[index].m_prev_state = KeyState::UNHELD;
		m_mouse.m_buttons[index].m_timestamp = message.time;
		return;
	}
}
void InputManager::process_mouseup(const MSG& message, uint32_t index) { // mouse key released
	update_mouse_pos(message);
	KeyState state = m_mouse.m_buttons[index].m_curr_state;
	uint64_t time = m_mouse.m_buttons[index].m_timestamp;
	switch (state) {
	case KeyState::PRESSED: // change to released
		m_mouse.m_buttons[index].m_curr_state = KeyState::RELEASED;
		m_mouse.m_buttons[index].m_prev_state = KeyState::PRESSED;
		m_mouse.m_buttons[index].m_timestamp = message.time;
		return;
	case KeyState::HELD: // change to released
		m_mouse.m_buttons[index].m_curr_state = KeyState::RELEASED;
		m_mouse.m_buttons[index].m_prev_state = KeyState::HELD;
		m_mouse.m_buttons[index].m_timestamp = message.time;
		return;
	case KeyState::RELEASED: // can get here if the frame rate is low and we are spamming a key, change to unheld
		m_mouse.m_buttons[index].m_curr_state = KeyState::HELD;
		m_mouse.m_buttons[index].m_prev_state = KeyState::RELEASED;
		m_mouse.m_buttons[index].m_timestamp = message.time;
		return;
	case KeyState::UNHELD: // keep as unheld
		m_mouse.m_buttons[index].m_curr_state = KeyState::UNHELD;
		return;
	}
}
void InputManager::process_keydown(const MSG& message) {
	uint8_t char_code = message.wParam;
	KeyState state = m_keystate[char_code].m_curr_state;
	uint64_t time = m_keystate[char_code].m_timestamp;
	switch (state) {
	case KeyState::HELD: // can only stay held, if the current state is held the previous state can be pressed so that needs to be updated
		m_keystate[char_code].m_prev_state = KeyState::HELD;
		return;
	case KeyState::RELEASED: // can get here if the frame rate is low and we are spamming a key
		m_keystate[char_code].m_curr_state = KeyState::UNHELD;
		m_keystate[char_code].m_prev_state = KeyState::RELEASED;
		m_keystate[char_code].m_timestamp = message.time;
		return;
	case KeyState::UNHELD:
		m_keystate[char_code].m_curr_state = KeyState::PRESSED;
		m_keystate[char_code].m_prev_state = KeyState::UNHELD;
		m_keystate[char_code].m_timestamp = message.time;
		return;
	}
	//}
}
void InputManager::process_keyup(const MSG& message) { // dont process characters
	//if (iscntrl(message.wParam)) { // makse sure we only process control keys, not printable keys
	uint8_t char_code = message.wParam;
	// if (message.wParam & ~VK_SHIFT) { // shift is not being pressed
	//     char_code += 'a' - 'A';
	// }
	KeyState state = m_keystate[char_code].m_curr_state; // store the current state of the key
	uint64_t time = m_keystate[char_code].m_timestamp;

	switch (state) {
	case KeyState::PRESSED: // can become held or released (cannot become pressed because that is a single frame action
		m_keystate[char_code].m_curr_state = KeyState::RELEASED; // set the current state
		m_keystate[char_code].m_prev_state = KeyState::PRESSED; // set the previos state
		m_keystate[char_code].m_timestamp = message.time; // set the time this action took place
		return;
	case KeyState::HELD: // can become released or stay held
		m_keystate[char_code].m_curr_state = KeyState::RELEASED;
		m_keystate[char_code].m_prev_state = KeyState::HELD;
		m_keystate[char_code].m_timestamp = message.time;
		return;
	case KeyState::RELEASED: // can get here if the frame rate is low and we are spamming a key
		m_keystate[char_code].m_curr_state = KeyState::UNHELD;
		m_keystate[char_code].m_prev_state = KeyState::RELEASED;
		m_keystate[char_code].m_timestamp = message.time;
		return;
	case KeyState::UNHELD: // can  only stay unheld 
		m_keystate[char_code].m_prev_state = KeyState::UNHELD;
		return;
	}
	//}

}
void InputManager::process_char(const MSG& message) { // there isn't a separate WM_CHAR message for a WM_KEYUP, this will hold input for straight typing, not for controlling a character
	// set the character to be the repeat count if we receive the character message for that character
	m_character_pressed[message.wParam] = static_cast<uint8_t>(message.lParam & 0xFF); // get the lower byte of the lparam
}