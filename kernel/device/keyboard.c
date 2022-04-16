#include "kernel/debug.h"
#include "kernel/iopic.h"
#include "kernel/keyboard.h"
#include "kernel/ringbuffer.h"
#include "kernel/spinlock.h"
#include "kernel/task.h"
#include "kernel/trap.h"
#include "kernel/x86.h"

#define KEYBOARD_BUF_PORT 0x60

#define CHAR_ESC       '\033'
#define CHAR_BACKSPACE '\b'
#define CHAR_TAB       '\t'
#define CHAR_ENTER     '\r'
#define CHAR_DELETE    '\177'

#define CHAR_INVISIBLE     0
#define CHAR_LEFT_CTRL     CHAR_INVISIBLE
#define CHAR_LEFT_SHIFT    CHAR_INVISIBLE
#define CHAR_RIFHT_SHIFT   CHAR_INVISIBLE
#define CHAR_LEFT_ALT      CHAR_INVISIBLE
#define CHAR_CAPS_LOCK     CHAR_INVISIBLE


/**
 * Make and break code
 */
#define MAKECODE_LEFT_SHIFT      0x2a
#define MAKECODE_RIGHT_SHIFT     0x36
#define MAKECODE_LEFT_ALT        0x38 
#define MAKECODE_RIGHT_ALT       0xe038 
#define BREAKCODE_RIGHT_ALT      0xe0b8 
#define MAKECODE_LEFT_CTRL       0x1d 
#define MAKECODE_RIGHT_CTRL      0xe01d 
#define BREAKCODE_RIGHT_CTRL     0xe09d 
#define MAKECODE_CAPS_LOCK       0x3a


#define INPUT_BUFFER_SIZE 256
char buf[INPUT_BUFFER_SIZE];
struct ringbuffer input_ringbuffer;

static bool ctrl_pressed;
static bool shift_pressed;
static bool alt_pressed;
static bool caps_lock_pressed;
static bool ext_scancode;

static char shift_keymap[][2] = {
	/* 0x00 */ {0, 0},
	/* 0x01 */ {CHAR_ESC, CHAR_ESC},
	/* 0x02 */ {'1', '!'},
	/* 0x03 */ {'2', '@'},
	/* 0x04 */ {'3', '#'},
	/* 0x05 */ {'4', '$'},
	/* 0x06 */ {'5', '%'},
	/* 0x07 */ {'6', '^'},
	/* 0x08 */ {'7', '&'},
	/* 0x09 */ {'8', '*'},
	/* 0x0A */ {'9', '('},
	/* 0x0B */ {'0', ')'},
	/* 0x0C */ {'-', '_'},
	/* 0x0D */ {'=', '+'},
	/* 0x0E */ {'\b', '\b'},
	/* 0x0F */ {'\t', '\t'},
	/* 0x10 */ {'q', 'Q'},
	/* 0x11 */ {'w', 'W'},
	/* 0x12 */ {'e', 'E'},
	/* 0x13 */ {'r', 'R'},
	/* 0x14 */ {'t', 'T'},
	/* 0x15 */ {'y', 'Y'},
	/* 0x16 */ {'u', 'u'},
	/* 0x17 */ {'i', 'I'},
	/* 0x18 */ {'o', 'O'},
	/* 0x19 */ {'p', 'P'},
	/* 0x1A */ {'[', '{'},
	/* 0x1B */ {']', '}'},
	/* 0x1C */ {CHAR_ENTER, CHAR_ENTER},
	/* 0x1D */ {CHAR_LEFT_CTRL, CHAR_LEFT_CTRL},
	/* 0x1E */ {'a', 'A'},
	/* 0x1F */ {'s', 'S'},
	/* 0x20 */ {'d', 'D'},
	/* 0x21 */ {'f', 'F'},
	/* 0x22 */ {'g', 'G'},
	/* 0x23 */ {'h', 'H'},
	/* 0x24 */ {'j', 'J'},
	/* 0x25 */ {'k', 'K'},
	/* 0x26 */ {'l', 'L'},
	/* 0x27 */ {';', ':'},
	/* 0x28 */ {'\'', '"'},
	/* 0x29 */ {'`', '~'},
	/* 0x2A */ {CHAR_LEFT_SHIFT, CHAR_LEFT_SHIFT},
	/* 0x2B */ {'\\', '|'},
	/* 0x2C */ {'z', 'Z'},
	/* 0x2D */ {'x', 'X'},
	/* 0x2E */ {'c', 'C'},
	/* 0x2F */ {'v', 'V'},
	/* 0x30 */ {'b', 'B'},
	/* 0x31 */ {'n', 'N'},
	/* 0x32 */ {'m', 'M'},
	/* 0x33 */ {',', '<'},
	/* 0x34 */ {'.', '>'},
	/* 0x35 */ {'/', '?'},
	/* 0x36 */ {CHAR_RIFHT_SHIFT, CHAR_RIFHT_SHIFT},
	/* 0x37 */ {'*', '*'},
	/* 0x38 */ {CHAR_LEFT_ALT, CHAR_LEFT_ALT},
	/* 0x39 */ {' ', ' '},
	/* 0x3A */ {CHAR_CAPS_LOCK, CHAR_CAPS_LOCK}
};

static void reset_status(uint16_t make_code, bool is_break_code) {
	switch (make_code) {
		case MAKECODE_LEFT_CTRL:
		case MAKECODE_RIGHT_CTRL: 
			ctrl_pressed = !is_break_code;
			break;

		case MAKECODE_LEFT_SHIFT:
		case MAKECODE_RIGHT_SHIFT: 
			shift_pressed = !is_break_code; 
			break;

		case MAKECODE_LEFT_ALT:
		case MAKECODE_RIGHT_ALT: 
			alt_pressed = !is_break_code; 
			break;
		
		case MAKECODE_CAPS_LOCK:
			if (!is_break_code) {
				caps_lock_pressed = !caps_lock_pressed;
			}
			break;
	}
}

static void keyboard_intr_handler() {
	uint16_t scancode = inb(KEYBOARD_BUF_PORT);
	
	if (scancode == 0xe0) {
		ext_scancode = true;
		return;
	}

	if (ext_scancode) {
		scancode = 0xe000 | scancode;
		ext_scancode = false;
	}

	bool is_break_code = (scancode & 0x0080) != 0;
	if (is_break_code) {
		uint16_t make_code = scancode & 0xff7f;
		reset_status(make_code, true);
		return;
	}

	if ((scancode == 0x00 || scancode > 0x3A) &&
		(scancode != MAKECODE_RIGHT_ALT) && 
		(scancode != MAKECODE_RIGHT_CTRL)) {
		return;
	}
	// convert right alt(e0xx) and right ctrl(e0xx) into alt(xx) and ctrl(xx)
	scancode &= 0x00FF;
	char ch = shift_keymap[scancode][0];
	bool shift = 0;
	if (ch >= 'a' && ch <= 'z') {
		// If the shift key and the caps_lock key are both pressed,
		// the letter char should be lower case.
		shift = shift_pressed ^ caps_lock_pressed;
	} else {
		shift = shift_pressed | caps_lock_pressed;
	}
	
	ch = shift_keymap[scancode][(int)shift];
	if (ch != 0) {
		ch = (ch == '\r') ? '\n' : ch;
		if (ch == 'd' && ctrl_pressed) {
			ch = 0;
		}
		if (!ringbuffer_isfull(&input_ringbuffer)) {
			ringbuffer_put_char(&input_ringbuffer, ch);
		}
		return;
	}

	reset_status(scancode, false);
}

int keyboard_read(char *ch) {
	return ringbuffer_read_char(&input_ringbuffer, ch);
}

void keyboard_init() {
	ringbuffer_init(&input_ringbuffer, buf, INPUT_BUFFER_SIZE);
	setup_irq_handler(1, keyboard_intr_handler);
	enable_irq(1);
}