#pragma once
#include <stdint.h>
#include "../ThreadSafeContainers/Queue.h"
#include <windows.h>

/*  

TODO:

mouse state struct and member
mouse input message processing function
get state of mouse function
create structure/table that converts from a configurable set of "gameactions" to chars on the keyboard
*/
class InputManager
{
public:
	/*  statemachine for an input
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
	
	explicit InputManager(HWND hwnd) :m_hwnd(hwnd), m_keystate() {
		for (int i = 0; i != 0xFF; i++) {
			m_keystate[i].m_curr_state = KeyState::UNHELD;
			m_keystate[i].m_prev_state = KeyState::UNHELD;
			m_keystate[i].m_timestamp = 0;
		}

	}
	~InputManager() {}
	void get_input();
	// indexes into the keystate array with the given byte. create function that converts from "gameactions" to uint8_ts
	const KeyState get_state_of_key(uint8_t) const; // both return a constant and don't modify the underlying structures so they are threadsafe
	const KeyState get_prev_state_of_key(uint8_t) const; // both return a constant and don't modify the underlying structures so they are threadsafe
	//void process_input();
	//KeyState get_state_of_key(uint8_t); // eventually make it take input from an enum of valid keys
private:
	HWND m_hwnd;

	// is prev_state even necessary?...
	struct KeyEntry {
		uint64_t m_timestamp; // time the message was posted
		KeyState m_curr_state;
		KeyState m_prev_state;
	};
	KeyEntry m_keystate[256]; // array of entries, one for each key, manages the curr, prev, and timestamps for each key
	// have different struct for mouse input (as only one mouse), eventually extend some abstract input class to allow for easy integration of controllers and flightsticks later...
	struct MouseState {

	};
	// have a enum of valid keys (named uint32_t variables) that can be used to index into the m_keystate array, and use the table to convert from what the programmer wants to this index
	// have pool of empty keystates we can grab from (could eventually use from memorymanager...)

	// updates the state of the key for the message received
	void process_keyup(const MSG&);
	void process_keydown(const MSG&);
	// have an array of keystates that is indexed by the index member of the keyentry function
	
	//Queue<KeyEntry>* m_input_queue; // needs to be threadsafe if we ever want to use this as a job
	// use a table to convert from game actions to an index into this array, table should be configurable
	uint32_t m_state_delay = 100; // milliseconds of delay that should be present before switching from PRESSED to HELD
};

