#pragma once
#include <stdint.h>
#include "../ThreadSafeContainers/Queue.h"
#include <unordered_map>
#include <windows.h>
//#include "../Globals.h"
#include "../Memory/MemoryManager.h"

/*  
TODO:

handle scroll wheel
handle a controller
handle multiple input devices (of varying kinds means each needs to extend a base class we can have an array of)
make input manager create the gameactions on it's own by reading in from a file
make inputmanager update gameactions if that file is changed while the engine is running
*/

class InputManager
{
public:
	/*
    +---------------------+
    |                     |
    V                     |
PRESSED  --->  HELD    UNHELD  <--- start
    |           |         ^
    |           V         |
    +----->  RELEASED  ---+
	*/
	typedef char Key;
	class GameAction;
	enum struct State { // use struct for a scoped enum
		PRESSED, // 0 -> 1 (should be immediate)
		HELD, // 1 -> 1 (AFTER ENOUGH TIME HAS PASSED) need held state because it can invalidate tapping (along with staying unheld for long enough)
		RELEASED, // 1 -> 0 (should be immediate, if a key is released it should remain released for atleast one frame)
		UNHELD // 0 -> 0 (should be in the next frame) 
	};
	enum struct MouseButton {
		LEFT = 0,
		MIDDLE = 1,
		RIGHT = 2
	};
	// InputManager() : m_key_state(), m_character_pressed(), m_mouse_state(), m_name_to_action(new std::unordered_map<uint32_t, GameAction*>) {}
	InputManager() : m_key_state(), m_character_pressed(), m_mouse_state() {
		// allocate an array of game_objects
		//m_name_to_action = static_cast<GameAction*>(memory_manager.get_linear_allocator().allocate_aligned(sizeof(GameAction) * m_action_count, alignof(GameAction)));
		// allocate an array of game_objects using the default constructor
		m_name_to_action = NEW_ARRAY(GameAction, m_action_count, memory_manager.get_linear_allocator())();
	}
	~InputManager() {
		//delete m_name_to_action;
		FREE(m_name_to_action, memory_manager.get_linear_allocator());
	}
	void get_input();
	// get state of a game action using a hashed string literal (e.g. HASH("shoot"))
	bool is_pressed(uint32_t) const;
	bool is_released(uint32_t) const;
	bool is_held(uint32_t) const;
	bool is_unheld(uint32_t) const;
	void add_action(uint32_t, MouseButton); // calls new which is fine because it's probably only called during inputmanager construction (by reading from a file)
	void add_action(uint32_t, Key); // eventually should use memory grabbed by the memory allocator so all the engine 
	// remap functions throw if we are trying to remap an action that doesn't exists (should throw because programmer could have misspelled something)
	void remap_action(uint32_t, MouseButton); // need to make sure remap functions are threadsafe (updating when no other thread is reading, can use a job for that)
	void remap_action(uint32_t, Key);
	int get_mouse_x() const;
	int get_mouse_y() const;

private:
	struct GameAction { // represents a single game action that is managed by the input manager
		enum class GameAction_t { // the type of a game action is the input type that action is set to use. extend this if we want to add controllers
			MOUSEBUTTON,
			KEY
		};
		union GameAction_v { // the value of a game action is the actual key/button that is used as input for that action
			MouseButton m_button;
			Key m_key;
		};
		GameAction_t m_type; // type of game action input (mouse button or keyboard button)
		GameAction_v m_value; // the actual mouse button or keyboard key that we use to look up the state
		GameAction() : m_type(GameAction_t::KEY) { // default game action has input of null character so it's impossible to input
			m_value.m_key = Key('\0');
		}
		explicit GameAction(MouseButton index) : m_type(), m_value() {
			m_type = GameAction_t::MOUSEBUTTON;
			m_value.m_button = index;
		}
		explicit GameAction(Key key) : m_type(), m_value() {
			m_type = GameAction_t::KEY;
			m_value.m_key = key;
		}
		~GameAction() = default;
	};
	struct KeyEntry {
		uint64_t m_timestamp; // time the message was posted
		State m_curr_state;
		State m_prev_state; // is prev state necessary? only really used if we want the single frame the key is initially pressed for
		KeyEntry() : m_timestamp(0), m_curr_state(State::UNHELD), m_prev_state(State::UNHELD) {}
	};
	struct Mouse {
		int x; // number of pixels from left side of window
		int y; // number of pixels from top of window
		uint32_t m_button_count; // size of the m_buttons array
		KeyEntry m_buttons[3]; // left, middle, right states
		Mouse() : x(0), y(0), m_button_count(3), m_buttons() {}
	};
	// should use pointer so the input manager can manage the lifetime of the GameAction objects
	// std::unordered_map<uint32_t, GameAction*>* m_name_to_action; // stores the mapping from the hashed string literal used by the programmer to identify a game action to the actual game action object itself
	const size_t m_action_count = 64;
	GameAction* m_name_to_action;
	const uint32_t m_held_delay = 100;	// milliseconds of delay that should be present before switching from PRESSED to HELD (anything shorter seems to convert to held instead of pressed)
	KeyEntry m_key_state[256];			// array of keystates, used for input to the actual game, array of entries, one for each key, manages the curr, prev, and timestamps for each key.
	uint8_t m_character_pressed[256];	// used for generic typing into a textbox or the eventual console menu. not used for controlling a character
	Mouse m_mouse_state;

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