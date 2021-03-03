#ifndef _PIXIRETRO_INPUT_H_
#define _PIXIRETRO_INPUT_H_

#include <vector>

union SDL_Event;

namespace pxr
{
namespace input
{

enum KeyCode { 
  KEY_a, KEY_b, KEY_c, KEY_d, KEY_e, KEY_f, KEY_g, KEY_h, KEY_i, KEY_j, KEY_k, KEY_l, KEY_m, 
  KEY_n, KEY_o, KEY_p, KEY_q, KEY_r, KEY_s, KEY_t, KEY_u, KEY_v, KEY_w, KEY_x, KEY_y, KEY_z,
  KEY_SPACE, KEY_BACKSPACE, KEY_ENTER, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_COUNT 
};

struct KeyLog
{
  bool _isDown;         // is key currently down?
  bool _isPressed;      // was key pressed between last update tick and the current?
  bool _isReleased;     // was key released between last update tick and the current?
};

//
// Must be called prior to any other function in this module. Results of calls prior to
// initialization are undefined.
//
void initialize();

//
// Records a key event. Called by the engine in response to key events.
//
void onKeyEvent(const SDL_Event& event);

//
// Updates the key logs and clears the key history. Called by the engine during the update tick.
//
void onUpdate();

//
// Accessors for key state for each key. For use by applications to get user input.
//
bool isKeyDown(KeyCode key);
bool isKeyPressed(KeyCode key);
bool isKeyReleased(KeyCode key);

//
// Accessor to get key press history. The history is simply an ordered list of the keys
// pressed by the user in ascending order of the time pressed. The history is wiped at the
// end of every update tick thus the history is includes only those keys pressed since the
// last update.
//
const std::vector<KeyCode>& getHistory();

//
// Simple helper to convert a key code to the ascii value of the character associated with
// the key. Only valid for key codes which have an associated ascii value, i.e. for alpha
// numeric keys and the space key. All other key codes return -1.
//
int keyToAsciiCode(KeyCode key);

//
// Simple helper to translate a string of a key enum value to the enum value, e.g. translate
// "KEY_a" to Key_a enum value.
//
KeyCode keyStringToKeyCode(const std::string& keyString);

} // namespace input
} // namespace pxr

#endif
