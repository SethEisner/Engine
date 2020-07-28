#include "InputManager.h"
#include <windows.h>
#include <windowsx.h>  // for GET_X/Y_PARAM
#include <string>
#include "../RenderManager/Window.h"
#include "../Engine.h"
#include "../Utilities/Utilities.h"


InputManager::InputManager() : m_name_to_action(NEW(HashTable<uint32_t COMMA GameAction>, memory_manager->get_linear_allocator())(63)), m_key_state(), m_character_pressed(), 
							   m_mouse_state(static_cast<int>(engine->window->m_width / 2), static_cast<int>(engine->window->m_height / 2), static_cast<int>(engine->window->m_width / 2), static_cast<int>(engine->window->m_height / 2)) {

}

void InputManager::init() {
	add_action(HASH("right"), InputManager::Key('D'));
	add_action(HASH("left"), InputManager::Key('A'));
	add_action(HASH("forward"), InputManager::Key('W'));
	add_action(HASH("backward"), InputManager::Key('S'));
}
void InputManager::update() {
	update_key_states(); // update the states of the relative buttons before we read in more input
	update_mouse_state(); // reset the mouse input once per frame
}
void InputManager::update_mouse_state() {
	// set the prev state to be the current state
	if (m_mouse_state.position_changed) {
		// m_mouse_state.prev_x = m_mouse_state.x;
		// m_mouse_state.prev_y = m_mouse_state.y;
		//m_mouse_state.delta_y = 0.0f;
		//SetCursorPos(static_cast<int>(engine->window->m_width / 2), static_cast<int>(engine->window->m_height / 2));
		m_mouse_state.position_changed = false;
	}
}
void InputManager::get_input(const MSG& message) {
	// update_key_states(); // update the states of the relative buttons before we read in more input
	// update_mouse_state();
	//MSG message = { 0 };
	//while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) > 0) { // there's a message in the queue
		// TranslateMessage(&message); // translate the message into 
		// DispatchMessageA(&message); // dispatch the messages we dont handle to the window procedure so we dont need to worry about the nitty gritty
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

bool InputManager::is_pressed(uint32_t hashed_action_name) const {
	//const GameAction* action = &(this->m_name_to_action[hashed_action_name % m_action_count]); // return a const gameaction pointer so we dont modify the object here
	const GameAction action = m_name_to_action->at(hashed_action_name);
	if (action.m_type == GameAction::GameAction_t::MOUSEBUTTON) { // find which array we need to index into (super low overhead because this type rarely changes)
		return m_mouse_state.m_buttons[static_cast<uint32_t>(action.m_value.m_button)].m_curr_state == State::PRESSED;
	}
	return m_key_state[action.m_value.m_key].m_curr_state == State::PRESSED;
}
bool InputManager::is_released(uint32_t hashed_action_name) const {
	//const GameAction* action = &(this->m_name_to_action[hashed_action_name % m_action_count]);
	const GameAction action = m_name_to_action->at(hashed_action_name);
	if (action.m_type == GameAction::GameAction_t::MOUSEBUTTON) {
		return m_mouse_state.m_buttons[static_cast<uint32_t>(action.m_value.m_button)].m_curr_state == State::RELEASED;
	}
	return m_key_state[action.m_value.m_key].m_curr_state == State::RELEASED;
}
bool InputManager::is_held(uint32_t hashed_action_name) const {
	//const GameAction* action = &(m_name_to_action[hashed_action_name % m_action_count]);
	//OutputDebugStringA("is held\n");
	const GameAction action = m_name_to_action->at(hashed_action_name);
	if (action.m_type == GameAction::GameAction_t::MOUSEBUTTON) {
		return m_mouse_state.m_buttons[static_cast<uint32_t>(action.m_value.m_button)].m_curr_state == State::HELD;
	}
	return m_key_state[action.m_value.m_key].m_curr_state == State::HELD;
}
bool InputManager::is_unheld(uint32_t hashed_action_name) const {
	//const GameAction* action = &(this->m_name_to_action[hashed_action_name % m_action_count]);
	const GameAction action = m_name_to_action->at(hashed_action_name);
	if (action.m_type == GameAction::GameAction_t::MOUSEBUTTON) {
		return m_mouse_state.m_buttons[static_cast<uint32_t>(action.m_value.m_button)].m_curr_state == State::UNHELD;
	}
	return m_key_state[action.m_value.m_key].m_curr_state == State::UNHELD;
}
void InputManager::add_action(uint32_t hashed_action_name, MouseButton button) {
	//m_name_to_action[hashed_action_name % m_action_count] = GameAction(button);
	m_name_to_action->insert(hashed_action_name, GameAction(button));

}
void InputManager::add_action(uint32_t hashed_action_name, Key key) {
	//m_name_to_action[hashed_action_name % m_action_count] = GameAction(key);
	m_name_to_action->insert(hashed_action_name, GameAction(key));

}
void InputManager::remap_action(uint32_t hashed_action_name, MouseButton button) {
	// GameAction* action = &(this->m_name_to_action[hashed_action_name % m_action_count]); // at function will currently throw an exception if we remap an action that doesn't exist. (probably okay because that's a bug in the user's code and not the intended interface)
	// action.m_type = GameAction::GameAction_t::MOUSEBUTTON;
	// action.m_value.m_button = button;
	m_name_to_action->set(hashed_action_name, GameAction(button));
}
void InputManager::remap_action(uint32_t hashed_action_name, Key key) {
	// GameAction* action = &(this->m_name_to_action[hashed_action_name % m_action_count]);
	// action.m_type = GameAction::GameAction_t::KEY;
	// action.m_value.m_key = key;
	m_name_to_action->set(hashed_action_name, GameAction(key));
}
int InputManager::get_mouse_x() const {
	return m_mouse_state.x;
}
int InputManager::get_mouse_y() const {
	return m_mouse_state.y;
}
int InputManager::get_mouse_prev_x() const {
	return m_mouse_state.prev_x;
}
int InputManager::get_mouse_prev_y() const {
	return m_mouse_state.prev_y;
}
float InputManager::get_mouse_delta_x() const {
	return m_mouse_state.delta_x;
	//return m_mouse_state.x - m_mouse_state.prev_x;
}
float InputManager::get_mouse_delta_y() const {
	return m_mouse_state.delta_y;
	// return m_mouse_state.y - m_mouse_state.prev_y;
}
bool  InputManager::mouse_pos_changed() const {
	return m_mouse_state.position_changed;
}
// private functions:
void InputManager::update_key_states() { // used to make sure released only exists for one frame, and to make sure that we properly transition to held for the mouse buttons
	memset(m_character_pressed, 0, 256 * sizeof(uint8_t)); // compiler should make this super fast

	uint64_t curr_time = GetTickCount64();
	for (int i = 0; i != 0xFF; i++) { // if the current state of a key is released, change it to unheld. released is only supposed to exist for one frame
		switch (m_key_state[i].m_curr_state) {
		case State::RELEASED:
			m_key_state[i].m_curr_state = State::UNHELD;
			m_key_state[i].m_prev_state = State::RELEASED;
			break;
		case State::PRESSED: // update the state to pressed here too because we have to do it for the mouse anyway and it should be consistent
			if (curr_time - m_key_state[i].m_timestamp >= m_held_delay) { // message clock uses GetTickCount which is milliseconds since startup
				m_key_state[i].m_curr_state = State::HELD;
				m_key_state[i].m_timestamp = curr_time;
			}
			m_key_state[i].m_prev_state = State::PRESSED;
			break;
		}
	}

	for (int i = 0; i != m_mouse_state.m_button_count; i++) { // if the current state of a mouse button is released, change it to unheld
		switch (m_mouse_state.m_buttons[i].m_curr_state) {
		case State::RELEASED:
			m_mouse_state.m_buttons[i].m_curr_state = State::UNHELD;
			m_mouse_state.m_buttons[i].m_prev_state = State::RELEASED;
			break;
		case State::PRESSED: // if the current state of a mouse button is released, change it to unheld
			if (curr_time - m_mouse_state.m_buttons[i].m_timestamp >= m_held_delay) { // message clock uses GetTickCount which is milliseconds since startup
				m_mouse_state.m_buttons[i].m_curr_state = State::HELD;
				m_mouse_state.m_buttons[i].m_timestamp = curr_time;
			}
			m_mouse_state.m_buttons[i].m_prev_state = State::PRESSED;
			break;
		}
	}
}

void InputManager::update_mouse_pos(const MSG& message) {
	// convert window coordinate to client area coordinate with origin at the center of the client area
	int delta_x = GET_X_LPARAM(message.lParam) - static_cast<int>(engine->window->m_width / 2) - engine->window->m_left;
	int delta_y = -(GET_Y_LPARAM(message.lParam) - static_cast<int>(engine->window->m_height / 2)) + engine->window->m_top;
	//std::string debug = std::to_string(delta_x) + " " + std::to_string(delta_y) + "\n";
	//OutputDebugStringA(debug.c_str());
	// if the mouse is not centered at the screen
	if (delta_x != 0 || delta_y != 0) { // there is a change in either the x or y direction
		// distance of mouse from the center of the window
		m_mouse_state.delta_x += delta_x;
		m_mouse_state.delta_y += -delta_y;
		// m_mouse_state.delta_y = m_mouse_state.y - m_mouse_state.prev_y;
		// m_mouse_state.delta_x = (static_cast<float>(GET_X_LPARAM(message.lParam)) - static_cast<float>(m_mouse_state.prev_x)) - static_cast<float>(engine->window->m_width / 2);
		// m_mouse_state.delta_y = -(static_cast<float>(GET_Y_LPARAM(message.lParam)) - static_cast<float>(engine->window->m_height / 2));
		m_mouse_state.position_changed = true;
		if (engine->window->m_mouse_captured) {
			SetCursorPos(static_cast<int>(engine->window->m_width / 2) + engine->window->m_screen_x, static_cast<int>(engine->window->m_height / 2) + engine->window->m_screen_y);
		}
		// std::string temp = "";
		// temp += std::to_string(m_mouse_state.x) + " " + std::to_string(m_mouse_state.y) + "\n";
		// OutputDebugStringA(temp.c_str());
		// std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
void InputManager::process_mousemove(const MSG& message) {
	update_mouse_pos(message);
}
void InputManager::process_mousedown(const MSG& message, uint32_t index) {
	update_mouse_pos(message);
	State state = m_mouse_state.m_buttons[index].m_curr_state;
	uint64_t time = m_mouse_state.m_buttons[index].m_timestamp;
	switch (state) { // dont need to process PRESSED here because only a single mousedown message is sent when we click the mouse
	case State::HELD: // can only stay held, if the current state is held the previous state can be pressed so that needs to be updated
		m_mouse_state.m_buttons[index].m_prev_state = State::HELD;
		return;
	case State::RELEASED: // can get here if the frame rate is low and we are spamming a key
		m_mouse_state.m_buttons[index].m_curr_state = State::UNHELD;
		m_mouse_state.m_buttons[index].m_prev_state = State::RELEASED;
		m_mouse_state.m_buttons[index].m_timestamp = message.time;
		return;
	case State::UNHELD: // key pressed
		m_mouse_state.m_buttons[index].m_curr_state = State::PRESSED;
		m_mouse_state.m_buttons[index].m_prev_state = State::UNHELD;
		m_mouse_state.m_buttons[index].m_timestamp = message.time;
		return;
	}
}
void InputManager::process_mouseup(const MSG& message, uint32_t index) { // mouse key released
	update_mouse_pos(message);
	State state = m_mouse_state.m_buttons[index].m_curr_state;
	uint64_t time = m_mouse_state.m_buttons[index].m_timestamp;
	switch (state) {
	case State::PRESSED: // change to released
		m_mouse_state.m_buttons[index].m_curr_state = State::RELEASED;
		m_mouse_state.m_buttons[index].m_prev_state = State::PRESSED;
		m_mouse_state.m_buttons[index].m_timestamp = message.time;
		return;
	case State::HELD: // change to released
		m_mouse_state.m_buttons[index].m_curr_state = State::RELEASED;
		m_mouse_state.m_buttons[index].m_prev_state = State::HELD;
		m_mouse_state.m_buttons[index].m_timestamp = message.time;
		return;
	case State::RELEASED: // can get here if the frame rate is low and we are spamming a key, change to unheld
		m_mouse_state.m_buttons[index].m_curr_state = State::HELD;
		m_mouse_state.m_buttons[index].m_prev_state = State::RELEASED;
		m_mouse_state.m_buttons[index].m_timestamp = message.time;
		return;
	case State::UNHELD: // keep as unheld
		m_mouse_state.m_buttons[index].m_curr_state = State::UNHELD;
		return;
	}
}
void InputManager::process_keydown(const MSG& message) {
	uint8_t char_code = message.wParam;
	State state = m_key_state[char_code].m_curr_state;
	uint64_t time = m_key_state[char_code].m_timestamp;
	switch (state) {
	case State::HELD: // can only stay held, if the current state is held the previous state can be pressed so that needs to be updated
		m_key_state[char_code].m_prev_state = State::HELD;
		return;
	case State::RELEASED: // can get here if the frame rate is low and we are spamming a key
		m_key_state[char_code].m_curr_state = State::UNHELD;
		m_key_state[char_code].m_prev_state = State::RELEASED;
		m_key_state[char_code].m_timestamp = message.time;
		return;
	case State::UNHELD:
		m_key_state[char_code].m_curr_state = State::PRESSED;
		m_key_state[char_code].m_prev_state = State::UNHELD;
		m_key_state[char_code].m_timestamp = message.time;
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
	State state = m_key_state[char_code].m_curr_state; // store the current state of the key
	uint64_t time = m_key_state[char_code].m_timestamp;

	switch (state) {
	case State::PRESSED: // can become held or released (cannot become pressed because that is a single frame action
		m_key_state[char_code].m_curr_state = State::RELEASED; // set the current state
		m_key_state[char_code].m_prev_state = State::PRESSED; // set the previos state
		m_key_state[char_code].m_timestamp = message.time; // set the time this action took place
		return;
	case State::HELD: // can become released or stay held
		m_key_state[char_code].m_curr_state = State::RELEASED;
		m_key_state[char_code].m_prev_state = State::HELD;
		m_key_state[char_code].m_timestamp = message.time;
		return;
	case State::RELEASED: // can get here if the frame rate is low and we are spamming a key
		m_key_state[char_code].m_curr_state = State::UNHELD;
		m_key_state[char_code].m_prev_state = State::RELEASED;
		m_key_state[char_code].m_timestamp = message.time;
		return;
	case State::UNHELD: // can  only stay unheld 
		m_key_state[char_code].m_prev_state = State::UNHELD;
		return;
	}
	//}

}
void InputManager::process_char(const MSG& message) { // there isn't a separate WM_CHAR message for a WM_KEYUP, this will hold input for straight typing, not for controlling a character
	// set the character to be the repeat count if we receive the character message for that character
	m_character_pressed[message.wParam] = static_cast<uint8_t>(message.lParam & 0xFF); // get the lower byte of the lparam
}