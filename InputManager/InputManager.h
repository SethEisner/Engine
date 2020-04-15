#pragma once
#include <stdint.h>
#include "../ThreadSafeContainers/Queue.h"
#include <windows.h>

/*  
TODO:
handle scroll wheel
create structure/table that converts from a configurable set of "gameactions" to chars on the keyboard (make map to a union so we can support remapping to a key or a mouse button
handle a controller
handle multiple input devices (of varying kinds means each needs to extend a base class we can have an array of)
*/

class InputManager
{
public:
	/*
	+---------------------+
	|					  |
	V					  |
PRESSED  --->  HELD		UNHELD  <--- start
	|			|		  ^
	|			V		  |
	+----->  RELEASED  ---+
	*/
	enum struct KeyState { // use struct for a scoped enum
		PRESSED, // 0 -> 1 (should be immediate)
		HELD, // 1 -> 1 (AFTER ENOUGH TIME HAS PASSED)
		RELEASED, // 1 -> 0 (should be immediate, if a key is released it should remain released for atleast one frame)
		UNHELD // 0 -> 0 (should be in the next frame) 
	};
	enum struct MouseButton {
		LEFT = 0,
		MIDDLE = 1,
		RIGHT = 2
	};
	InputManager() : m_keystate(), m_character_pressed(), m_mouse() {}
	~InputManager() {}
	void get_input();
	// indexes into the keystate array with the given byte. create function that converts from "gameactions" to uint8_ts
	// overloaded for keys and mouse buttons so they can eveually be interchangeable
	const KeyState get_state(uint8_t) const; // both return a constant and don't modify the underlying structures so they are threadsafe
	const KeyState get_prev_state(uint8_t) const; // both return a constant and don't modify the underlying structures so they are threadsafe
	const KeyState get_state(MouseButton) const;
	const KeyState get_prev_state(MouseButton) const;
	float get_mouse_x() const; // dont need const because we return a copy so they couldnt change out value even if they wanted to
	float get_mouse_y() const; // dont need const because we return a copy so they couldnt change out value even if they wanted to

	/* query state directly instead of get_state function. does this allow for abstracting away different input types?
	bool is_pressed();
	bool is_released();
	bool is_held();
	bool is_unheld();
	*/

private:
	struct KeyEntry {
		uint64_t m_timestamp; // time the message was posted
		KeyState m_curr_state;
		KeyState m_prev_state; // is prev state necessary? only really used if we want the single frame the key is initially pressed for
		KeyEntry() : m_timestamp(0), m_curr_state(KeyState::UNHELD), m_prev_state(KeyState::UNHELD) {}
	};
	struct Mouse {
		float x; // number of pixels from left side of window
		float y; // number of pixels from top of window
		uint32_t m_button_count; // size of the m_buttons array
		KeyEntry m_buttons[3]; // left, middle, right states
		Mouse() : x(0.0), y(0.0), m_button_count(3), m_buttons() {}
	};

	const uint32_t m_held_delay = 100;	// milliseconds of delay that should be present before switching from PRESSED to HELD
	KeyEntry m_keystate[256];			// array of keystates, used for input to the actual game, array of entries, one for each key, manages the curr, prev, and timestamps for each key.
	uint8_t m_character_pressed[256];	// used for generic typing into a textbox or the eventual console menu. not used for controlling a character
	Mouse m_mouse;

	void update_key_states(void);		// used to update the state changes that aren't dependent on getting a new message/ can't happend due to the implementation
	void update_mouse_pos(const MSG&);	// changes the position of the mouse for any mouse message that contains the position
	void process_mousemove(const MSG&);
	void process_mousedown(const MSG&, uint32_t);
	void process_mouseup(const MSG&, uint32_t);
	//void process_mousewheel(const MSG&);
	void process_char(const MSG&);
	void process_keyup(const MSG&);
	void process_keydown(const MSG&); // still need these keys because we need to be able to process non character keys (shift, control,...)
};

