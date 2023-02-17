#include <peripheral/keyboard.h>
#include <proc.h>


void _console_main(){

  struct kdb_keyinfo_pkt keyevent;
  while (1) {
      if (!kbd_recv_key(&keyevent)) {
        yield();
        continue;
      }
      if ((keyevent.state & KBD_KEY_FPRESSED)) {
          if ((keyevent.keycode & 0xff00) <= KEYPAD) {
              console_write_char((char)(keyevent.keycode & 0x00ff));
          } else if (keyevent.keycode == KEY_UP) {
              console_view_up();
          } else if (keyevent.keycode == KEY_DOWN) {
              console_view_down();
          }
      }
  }

}
