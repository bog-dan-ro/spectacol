/* keysyms.cpp: Qt keysyms map

    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Qt>

extern "C"  {
# include <input.h>
# include <keyboard.h>
}

keysyms_map_t keysyms_map[] = {

  { Qt::Key_Ampersand,   INPUT_KEY_ampersand   },
  { Qt::Key_Apostrophe,  INPUT_KEY_apostrophe  },
  { Qt::Key_AsciiCircum, INPUT_KEY_asciicircum },
  { Qt::Key_AsciiTilde,  INPUT_KEY_asciitilde  },
  { Qt::Key_Asterisk,    INPUT_KEY_asterisk    },
  { Qt::Key_At,          INPUT_KEY_at          },
  { Qt::Key_Backslash,   INPUT_KEY_backslash   },
  { Qt::Key_Bar,         INPUT_KEY_bar         },
  { Qt::Key_BraceLeft,   INPUT_KEY_braceleft   },
  { Qt::Key_BraceRight,  INPUT_KEY_braceright  },
  { Qt::Key_BracketLeft, INPUT_KEY_bracketleft },
  { Qt::Key_BracketRight,INPUT_KEY_bracketright},
  { Qt::Key_Colon,       INPUT_KEY_colon       },
  { Qt::Key_Comma,       INPUT_KEY_comma       },
  { Qt::Key_Dead_Circumflex, INPUT_KEY_dead_circumflex },
  { Qt::Key_Dollar,      INPUT_KEY_dollar      },
  { Qt::Key_Equal,       INPUT_KEY_equal       },
  { Qt::Key_Escape,      INPUT_KEY_Escape      },
  { Qt::Key_Exclam,      INPUT_KEY_exclam      },
  { Qt::Key_Greater,     INPUT_KEY_greater     },
  { Qt::Key_Less,        INPUT_KEY_less        },
  { Qt::Key_Minus,       INPUT_KEY_minus       },
  { Qt::Key_NumberSign,  INPUT_KEY_numbersign  },
  { Qt::Key_ParenLeft,   INPUT_KEY_parenleft   },
  { Qt::Key_ParenRight,  INPUT_KEY_parenright  },
  { Qt::Key_Percent,     INPUT_KEY_percent     },
  { Qt::Key_Period,      INPUT_KEY_period      },
  { Qt::Key_Plus,        INPUT_KEY_plus        },
  { Qt::Key_Question,    INPUT_KEY_question    },
  { Qt::Key_QuoteDbl,    INPUT_KEY_quotedbl    },
  { Qt::Key_Return,      INPUT_KEY_Return      },
  { Qt::Key_Semicolon,   INPUT_KEY_semicolon   },
  { Qt::Key_Slash,       INPUT_KEY_slash       },
  { Qt::Key_Space,       INPUT_KEY_space       },
  { Qt::Key_Tab,         INPUT_KEY_Tab         },
  { Qt::Key_Underscore,  INPUT_KEY_underscore  },

  { Qt::Key_0,           INPUT_KEY_0           },
  { Qt::Key_1,           INPUT_KEY_1           },
  { Qt::Key_2,           INPUT_KEY_2           },
  { Qt::Key_3,           INPUT_KEY_3           },
  { Qt::Key_4,           INPUT_KEY_4           },
  { Qt::Key_5,           INPUT_KEY_5           },
  { Qt::Key_6,           INPUT_KEY_6           },
  { Qt::Key_7,           INPUT_KEY_7           },
  { Qt::Key_8,           INPUT_KEY_8           },
  { Qt::Key_9,           INPUT_KEY_9           },

  { Qt::Key_A + Qt::ShiftModifier, INPUT_KEY_A           },
  { Qt::Key_B + Qt::ShiftModifier, INPUT_KEY_B           },
  { Qt::Key_C + Qt::ShiftModifier, INPUT_KEY_C           },
  { Qt::Key_D + Qt::ShiftModifier, INPUT_KEY_D           },
  { Qt::Key_E + Qt::ShiftModifier, INPUT_KEY_E           },
  { Qt::Key_F + Qt::ShiftModifier, INPUT_KEY_F           },
  { Qt::Key_G + Qt::ShiftModifier, INPUT_KEY_G           },
  { Qt::Key_H + Qt::ShiftModifier, INPUT_KEY_H           },
  { Qt::Key_I + Qt::ShiftModifier, INPUT_KEY_I           },
  { Qt::Key_J + Qt::ShiftModifier, INPUT_KEY_J           },
  { Qt::Key_K + Qt::ShiftModifier, INPUT_KEY_K           },
  { Qt::Key_L + Qt::ShiftModifier, INPUT_KEY_L           },
  { Qt::Key_M + Qt::ShiftModifier, INPUT_KEY_M           },
  { Qt::Key_N + Qt::ShiftModifier, INPUT_KEY_N           },
  { Qt::Key_O + Qt::ShiftModifier, INPUT_KEY_O           },
  { Qt::Key_P + Qt::ShiftModifier, INPUT_KEY_P           },
  { Qt::Key_Q + Qt::ShiftModifier, INPUT_KEY_Q           },
  { Qt::Key_R + Qt::ShiftModifier, INPUT_KEY_R           },
  { Qt::Key_S + Qt::ShiftModifier, INPUT_KEY_S           },
  { Qt::Key_T + Qt::ShiftModifier, INPUT_KEY_T           },
  { Qt::Key_U + Qt::ShiftModifier, INPUT_KEY_U           },
  { Qt::Key_V + Qt::ShiftModifier, INPUT_KEY_V           },
  { Qt::Key_W + Qt::ShiftModifier, INPUT_KEY_W           },
  { Qt::Key_X + Qt::ShiftModifier, INPUT_KEY_X           },
  { Qt::Key_Y + Qt::ShiftModifier, INPUT_KEY_Y           },
  { Qt::Key_Z + Qt::ShiftModifier, INPUT_KEY_Z           },

  { Qt::Key_A,           INPUT_KEY_a           },
  { Qt::Key_B,           INPUT_KEY_b           },
  { Qt::Key_C,           INPUT_KEY_c           },
  { Qt::Key_D,           INPUT_KEY_d           },
  { Qt::Key_E,           INPUT_KEY_e           },
  { Qt::Key_F,           INPUT_KEY_f           },
  { Qt::Key_G,           INPUT_KEY_g           },
  { Qt::Key_H,           INPUT_KEY_h           },
  { Qt::Key_I,           INPUT_KEY_i           },
  { Qt::Key_J,           INPUT_KEY_j           },
  { Qt::Key_K,           INPUT_KEY_k           },
  { Qt::Key_L,           INPUT_KEY_l           },
  { Qt::Key_M,           INPUT_KEY_m           },
  { Qt::Key_N,           INPUT_KEY_n           },
  { Qt::Key_O,           INPUT_KEY_o           },
  { Qt::Key_P,           INPUT_KEY_p           },
  { Qt::Key_Q,           INPUT_KEY_q           },
  { Qt::Key_R,           INPUT_KEY_r           },
  { Qt::Key_S,           INPUT_KEY_s           },
  { Qt::Key_T,           INPUT_KEY_t           },
  { Qt::Key_U,           INPUT_KEY_u           },
  { Qt::Key_V,           INPUT_KEY_v           },
  { Qt::Key_W,           INPUT_KEY_w           },
  { Qt::Key_X,           INPUT_KEY_x           },
  { Qt::Key_Y,           INPUT_KEY_y           },
  { Qt::Key_Z,           INPUT_KEY_z           },

  { Qt::Key_Backspace,   INPUT_KEY_BackSpace   },
  { Qt::Key_Enter,       INPUT_KEY_KP_Enter    },
  { Qt::Key_Up,          INPUT_KEY_Up          },
  { Qt::Key_Down,        INPUT_KEY_Down        },
  { Qt::Key_Left,        INPUT_KEY_Left        },
  { Qt::Key_Right,       INPUT_KEY_Right       },
  { Qt::Key_Insert,      INPUT_KEY_Insert      },
  { Qt::Key_Delete,      INPUT_KEY_Delete      },
  { Qt::Key_Home,        INPUT_KEY_Home        },
  { Qt::Key_End,         INPUT_KEY_End         },
  { Qt::Key_PageUp,      INPUT_KEY_Page_Up     },
  { Qt::Key_PageDown,    INPUT_KEY_Page_Down   },
  { Qt::Key_CapsLock,    INPUT_KEY_Caps_Lock   },
  { Qt::Key_Shift,       INPUT_KEY_Shift_L     },
  { Qt::Key_Control,     INPUT_KEY_Control_L   },
  { Qt::Key_Alt,         INPUT_KEY_Alt_L       },
  { Qt::Key_Meta,        INPUT_KEY_Meta_L      },
  { Qt::Key_Super_L,     INPUT_KEY_Super_L     },
  { Qt::Key_Super_R,     INPUT_KEY_Super_R     },
  { Qt::Key_Hyper_L,     INPUT_KEY_Hyper_L     },
  { Qt::Key_Hyper_R,     INPUT_KEY_Hyper_R     },
  { Qt::Key_Mode_switch, INPUT_KEY_Mode_switch },

  { Qt::Key_F1,          INPUT_KEY_F1          },
  { Qt::Key_F2,          INPUT_KEY_F2          },
  { Qt::Key_F3,          INPUT_KEY_F3          },
  { Qt::Key_F4,          INPUT_KEY_F4          },
  { Qt::Key_F5,          INPUT_KEY_F5          },
  { Qt::Key_F6,          INPUT_KEY_F6          },
  { Qt::Key_F7,          INPUT_KEY_F7          },
  { Qt::Key_F8,          INPUT_KEY_F8          },
  { Qt::Key_F9,          INPUT_KEY_F9          },
  { Qt::Key_F10,         INPUT_KEY_F10         },
  { Qt::Key_F11,         INPUT_KEY_F11         },
  { Qt::Key_F12,         INPUT_KEY_F12         },


  { INPUT_KEY_NONE, INPUT_KEY_NONE }			/* End marker: DO NOT MOVE! */

};
