#pragma once
/*#
    # vic20.h

    A Commodore VIC-20 emulator in a C header.

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C or C++ file to create the 
    implementation.

    Optionally provide the following macros with your own implementation
    
    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    You need to include the following headers before including c64.h:

    - chips/m6502.h
    - chips/m6522.h
    - chips/m6561.h
    - chips/kbd.h
    - chips/mem.h
    - chips/clk.h

    ## The Commodore VIC-20

    TODO!

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution. 
#*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIC20_FREQUENCY (1108404)
#define VIC20_MAX_AUDIO_SAMPLES (1024)        /* max number of audio samples in internal sample buffer */
#define VIC20_DEFAULT_AUDIO_SAMPLES (128)     /* default number of samples in internal sample buffer */ 

/* VIC-20 joystick types */
typedef enum {
    VIC20_JOYSTICKTYPE_NONE,
    VIC20_JOYSTICKTYPE_DIGITAL_1,
    VIC20_JOYSTICKTYPE_DIGITAL_2,
    VIC20_JOYSTICKTYPE_DIGITAL_12,    /* input routed to both joysticks */
} vic20_joystick_type_t;

/* joystick mask bits */
#define VIC20_JOYSTICK_UP    (1<<0)
#define VIC20_JOYSTICK_DOWN  (1<<1)
#define VIC20_JOYSTICK_LEFT  (1<<2)
#define VIC20_JOYSTICK_RIGHT (1<<3)
#define VIC20_JOYSTICK_BTN   (1<<4)

/* casette port bits, same as C1530_CASPORT_* */
#define VIC20_CASPORT_MOTOR   (1<<0)  /* 1: motor off, 0: motor on */
#define VIC20_CASPORT_READ    (1<<1)  /* 1: read signal from datasette, connected to CIA-1 FLAG */
#define VIC20_CASPORT_WRITE   (1<<2)  /* not implemented */
#define VIC20_CASPORT_SENSE   (1<<3)  /* 1: play button up, 0: play button down */

/* IEC port bits, same as C1541_IECPORT_* */
#define VIC20_IECPORT_RESET   (1<<0)  /* 1: RESET, 0: no reset */
#define VIC20_IECPORT_SRQIN   (1<<1)  /* connected to CIA-1 FLAG */
#define VIC20_IECPORT_DATA    (1<<2)
#define VIC20_IECPORT_CLK     (1<<3)
#define VIC20_IECPORT_ATN     (1<<4)

/* audio sample data callback */
typedef void (*vic20_audio_callback_t)(const float* samples, int num_samples, void* user_data);

/* config parameters for c64_init() */
typedef struct {
    vic20_joystick_type_t joystick_type;  /* default is VIC20_JOYSTICK_NONE */

    /* video output config (if you don't want video decoding, set these to 0) */
    void* pixel_buffer;         /* pointer to a linear RGBA8 pixel buffer,
                                   query required size via vic20_max_display_size() */
    int pixel_buffer_size;      /* size of the pixel buffer in bytes */

    /* optional user-data for callback functions */
    void* user_data;

    /* audio output config (if you don't want audio, set audio_cb to zero) */
    vic20_audio_callback_t audio_cb;  /* called when audio_num_samples are ready */
    int audio_num_samples;          /* default is VIC20_AUDIO_NUM_SAMPLES */
    int audio_sample_rate;          /* playback sample rate in Hz, default is 44100 */
    float audio_sid_volume;         /* audio volume of the VIC chip (0.0 .. 1.0), default is 1.0 */

    /* ROM images */
    const void* rom_char;           /* 4 KByte character ROM dump */
    const void* rom_basic;          /* 8 KByte BASIC dump */
    const void* rom_kernal;         /* 8 KByte KERNAL dump */
    int rom_char_size;
    int rom_basic_size;
    int rom_kernal_size;
} vic20_desc_t;

/* VIC-20 emulator state */
typedef struct {
    uint64_t pins;
    m6502_t cpu;
    m6522_t via_1;
    m6522_t via_2;
    m6561_t vic;
    
    bool valid;
    vic20_joystick_type_t joystick_type;
    uint8_t joystick_active;
    uint8_t cas_port;           /* cassette port, shared with c1530_t if datasette is connected */
    uint8_t iec_port;           /* IEC serial port, shared with c1541_t if connected */
    uint8_t kbd_joy1_mask;      /* current joystick-1 state from keyboard-joystick emulation */
    uint8_t kbd_joy2_mask;      /* current joystick-2 state from keyboard-joystick emulation */
    uint8_t joy_joy1_mask;      /* current joystick-1 state from c64_joystick() */
    uint8_t joy_joy2_mask;      /* current joystick-2 state from c64_joystick() */

    kbd_t kbd;                  /* keyboard matrix state */
    mem_t mem_cpu;              /* CPU-visible memory mapping */

    void* user_data;
    uint32_t* pixel_buffer;
    vic20_audio_callback_t audio_cb;
    int num_samples;
    int sample_pos;
    float sample_buffer[VIC20_MAX_AUDIO_SAMPLES];

    uint8_t ram0[0x0400];           /* 1 KB zero page, stack, system work area */
    uint8_t ram1[0x1000];           /* 4 KB main RAM */
    uint8_t ram_exp[3][0x2000];     /* optional expansion RAM areas */
    uint8_t rom_char[0x1000];       /* 4 KB character ROM image */
    uint8_t rom_basic[0x2000];      /* 8 KB BASIC ROM image */
    uint8_t rom_kernal[0x2000];     /* 8 KB KERNAL V3 ROM image */
} vic20_t;

/* initialize a new VIC-20 instance */
void vic20_init(vic20_t* sys, const vic20_desc_t* desc);
/* discard VIC-20 instance */
void vic20_discard(vic20_t* sys);
/* get the standard framebuffer width and height in pixels */
int vic20_std_display_width(void);
int vic20_std_display_height(void);
/* get the maximum framebuffer size in number of bytes */
int vic20_max_display_size(void);
/* get the current framebuffer width and height in pixels */
int vic20_display_width(vic20_t* sys);
int vic20_display_height(vic20_t* sys);
/* reset a VIC-20 instance */
void vic20_reset(vic20_t* sys);
/* tick VIC-20 instance for a given number of microseconds, also updates keyboard state */
void vic20_exec(vic20_t* sys, uint32_t micro_seconds);
/* ...or optionally: tick the VIC-20 instance once, does not update keyboard state! */
void vic20_tick(vic20_t* sys);
/* send a key-down event to the VIC-20 */
void vic20_key_down(vic20_t* sys, int key_code);
/* send a key-up event to the VIC-20 */
void vic20_key_up(vic20_t* sys, int key_code);
/* enable/disable joystick emulation */
void vic20_set_joystick_type(vic20_t* sys, vic20_joystick_type_t type);
/* get current joystick emulation type */
vic20_joystick_type_t vic20_joystick_type(vic20_t* sys);
/* set joystick mask (combination of VIC20_JOYSTICK_*) */
void vic20_joystick(vic20_t* sys, uint8_t joy1_mask, uint8_t joy2_mask);
/* quickload a .prg/.bin file */
bool vic20_quickload(vic20_t* sys, const uint8_t* ptr, int num_bytes);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h> /* memcpy, memset */
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

#define _VIC20_STD_DISPLAY_WIDTH (232)  /* actually 229, but rounded up to 8x */
#define _VIC20_STD_DISPLAY_HEIGHT (304) /* actually 303, but rounded up to 8x */
#define _VIC20_DBG_DISPLAY_WIDTH ((_M6561_HTOTAL+1)*8)
#define _VIC20_DBG_DISPLAY_HEIGHT (_M6561_VTOTAL+1)
#define _VIC20_DISPLAY_SIZE (_VIC20_DBG_DISPLAY_WIDTH*_VIC20_DBG_DISPLAY_HEIGHT*4)
#define _VIC20_DISPLAY_X (64)
#define _VIC20_DISPLAY_Y (24)

static uint64_t _vic20_tick(vic20_t* sys, uint64_t pins);
static void _vic20_via1_out(int port_id, uint8_t data, void* user_data);
static uint8_t _vic20_via1_in(int port_id, void* user_data);
static void _vic20_via2_out(int port_id, uint8_t data, void* user_data);
static uint8_t _vic20_via2_in(int port_id, void* user_data);
static uint16_t _vic20_vic_fetch(uint16_t addr, void* user_data);
static void _vic20_init_key_map(vic20_t* sys);

#define _VIC20_DEFAULT(val,def) (((val) != 0) ? (val) : (def));
#define _VIC20_CLEAR(val) memset(&val, 0, sizeof(val))

void vic20_init(vic20_t* sys, const vic20_desc_t* desc) {
    CHIPS_ASSERT(sys && desc);
    CHIPS_ASSERT(!desc->pixel_buffer || (desc->pixel_buffer_size >= _VIC20_DISPLAY_SIZE));

    memset(sys, 0, sizeof(vic20_t));
    sys->valid = true;
    sys->joystick_type = desc->joystick_type;
    CHIPS_ASSERT(desc->rom_char && (desc->rom_char_size == sizeof(sys->rom_char)));
    CHIPS_ASSERT(desc->rom_basic && (desc->rom_basic_size == sizeof(sys->rom_basic)));
    CHIPS_ASSERT(desc->rom_kernal && (desc->rom_kernal_size == sizeof(sys->rom_kernal)));
    memcpy(sys->rom_char, desc->rom_char, sizeof(sys->rom_char));
    memcpy(sys->rom_basic, desc->rom_basic, sizeof(sys->rom_basic));
    memcpy(sys->rom_kernal, desc->rom_kernal, sizeof(sys->rom_kernal));
    sys->user_data = desc->user_data;
    sys->audio_cb = desc->audio_cb;
    sys->num_samples = _VIC20_DEFAULT(desc->audio_num_samples, VIC20_DEFAULT_AUDIO_SAMPLES);
    CHIPS_ASSERT(sys->num_samples <= VIC20_MAX_AUDIO_SAMPLES);

    m6502_desc_t cpu_desc;
    _VIC20_CLEAR(cpu_desc);
    sys->pins = m6502_init(&sys->cpu, &cpu_desc);

    m6522_desc_t via_desc;
    _VIC20_CLEAR(via_desc);
    via_desc.user_data = sys;
    via_desc.in_cb = _vic20_via1_in;
    via_desc.out_cb = _vic20_via1_out;
    m6522_init(&sys->via_1, &via_desc);
    via_desc.in_cb = _vic20_via2_in;
    via_desc.out_cb = _vic20_via2_out;
    m6522_init(&sys->via_2, &via_desc);

    m6561_desc_t vic_desc;
    _VIC20_CLEAR(vic_desc);
    vic_desc.fetch_cb = _vic20_vic_fetch;
    vic_desc.rgba8_buffer = (uint32_t*) desc->pixel_buffer;
    vic_desc.rgba8_buffer_size = desc->pixel_buffer_size;
    vic_desc.vis_x = _VIC20_DISPLAY_X;
    vic_desc.vis_y = _VIC20_DISPLAY_Y;
    vic_desc.vis_w = _VIC20_STD_DISPLAY_WIDTH;
    vic_desc.vis_h = _VIC20_STD_DISPLAY_HEIGHT;
    vic_desc.user_data = sys;
    m6561_init(&sys->vic, &vic_desc);

    _vic20_init_key_map(sys);
    
    /*
        VIC-20 memory map:

        0000..0400      zero-page, stack, system area
        [unused]
        1000..1FFF      4 KB Main RAM (block 0)
        [2000..3FFF]    8 KB Expansion Block 1
        [4000..5FFF]    8 KB Expansion Block 2
        [6000..7FFF]    8 KB Expansion Block 3
        8000..8FFF      4 KB Character ROM
        9000..900F      VIC Registers
        9110..911F      VIA #1 Registers
        9120..912F      VIA #2 Regusters
        9400..97FF      1Kx4 bit color ram (either at 9600 or 9400)
        [9800..9BFF]    1 KB I/O Expansion 2
        [9C00..9FFF]    1 KB I/O Expansion 3
        [A000..BFFF]    8 KB Expansion Block 5 (usuall ROM cartrisges)
        C000..DFFF      8 KB BASIC ROM
        E000..FFFF      8 KB KERNAL ROM
    */
    mem_init(&sys->mem_cpu);
    mem_map_ram(&sys->mem_cpu, 0, 0x0000, 0x0400, sys->ram0);
    mem_map_ram(&sys->mem_cpu, 0, 0x1000, 0x1000, sys->ram1);
    mem_map_rom(&sys->mem_cpu, 0, 0x8000, 0x1000, sys->rom_char);
    mem_map_rom(&sys->mem_cpu, 0, 0xC000, 0xDFFF, sys->rom_basic);
    mem_map_rom(&sys->mem_cpu, 0, 0xE000, 0xFFFF, sys->rom_kernal);
}

void vic20_discard(vic20_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->valid = false;
}

int vic20_std_display_width(void) {
    return _VIC20_STD_DISPLAY_WIDTH;
}

int vic20_std_display_height(void) {
    return _VIC20_STD_DISPLAY_HEIGHT;
}

int vic20_max_display_size(void) {
    return _VIC20_DISPLAY_SIZE;
}

int vic20_display_width(vic20_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return m6561_display_width(&sys->vic);
}

int vic20_display_height(vic20_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return m6561_display_height(&sys->vic);
}

void vic20_reset(vic20_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->kbd_joy1_mask = sys->kbd_joy2_mask = 0;
    sys->joy_joy1_mask = sys->joy_joy2_mask = 0;
    sys->pins |= M6502_RES;
    m6522_reset(&sys->via_1);
    m6522_reset(&sys->via_2);
    m6561_reset(&sys->vic);
}

void vic20_tick(vic20_t* sys) {
    sys->pins = _vic20_tick(sys, sys->pins);
}

void vic20_exec(vic20_t* sys, uint32_t micro_seconds) {
    CHIPS_ASSERT(sys && sys->valid);
    uint32_t num_ticks = clk_us_to_ticks(VIC20_FREQUENCY, micro_seconds);
    uint64_t pins = sys->pins;
    for (uint32_t ticks = 0; ticks < num_ticks; ticks++) {
        pins = _vic20_tick(sys, pins);
    }
    sys->pins = pins;
    kbd_update(&sys->kbd);
}

void vic20_key_down(vic20_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    if (sys->joystick_type == VIC20_JOYSTICKTYPE_NONE) {
        kbd_key_down(&sys->kbd, key_code);
    }
    else {
        uint8_t m = 0;
        switch (key_code) {
            case 0x20: m = VIC20_JOYSTICK_BTN; break;
            case 0x08: m = VIC20_JOYSTICK_LEFT; break;
            case 0x09: m = VIC20_JOYSTICK_RIGHT; break;
            case 0x0A: m = VIC20_JOYSTICK_DOWN; break;
            case 0x0B: m = VIC20_JOYSTICK_UP; break;
            default: kbd_key_down(&sys->kbd, key_code); break;
        }
        if (m != 0) {
            switch (sys->joystick_type) {
                case VIC20_JOYSTICKTYPE_DIGITAL_1:
                    sys->kbd_joy1_mask |= m;
                    break;
                case VIC20_JOYSTICKTYPE_DIGITAL_2:
                    sys->kbd_joy2_mask |= m;
                    break;
                case VIC20_JOYSTICKTYPE_DIGITAL_12:
                    sys->kbd_joy1_mask |= m;
                    sys->kbd_joy2_mask |= m;
                    break;
                default: break;
            }
        }
    }
}

void vic20_key_up(vic20_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    if (sys->joystick_type == VIC20_JOYSTICKTYPE_NONE) {
        kbd_key_up(&sys->kbd, key_code);
    }
    else {
        uint8_t m = 0;
        switch (key_code) {
            case 0x20: m = VIC20_JOYSTICK_BTN; break;
            case 0x08: m = VIC20_JOYSTICK_LEFT; break;
            case 0x09: m = VIC20_JOYSTICK_RIGHT; break;
            case 0x0A: m = VIC20_JOYSTICK_DOWN; break;
            case 0x0B: m = VIC20_JOYSTICK_UP; break;
            default: kbd_key_up(&sys->kbd, key_code); break;
        }
        if (m != 0) {
            switch (sys->joystick_type) {
                case VIC20_JOYSTICKTYPE_DIGITAL_1:
                    sys->kbd_joy1_mask &= ~m;
                    break;
                case VIC20_JOYSTICKTYPE_DIGITAL_2:
                    sys->kbd_joy2_mask &= ~m;
                    break;
                case VIC20_JOYSTICKTYPE_DIGITAL_12:
                    sys->kbd_joy1_mask &= ~m;
                    sys->kbd_joy2_mask &= ~m;
                    break;
                default: break;
            }
        }
    }
}

void vic20_set_joystick_type(vic20_t* sys, vic20_joystick_type_t type) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->joystick_type = type;
}

vic20_joystick_type_t vic20_joystick_type(vic20_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return sys->joystick_type;
}

void vic20_joystick(vic20_t* sys, uint8_t joy1_mask, uint8_t joy2_mask) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->joy_joy1_mask = joy1_mask;
    sys->joy_joy2_mask = joy2_mask;
}

static uint64_t _vic20_tick(vic20_t* sys, uint64_t pins) {
    // FIXME
    return pins;
}

static void _vic20_via1_out(int port_id, uint8_t data, void* user_data) {
    // FIXME
}

static uint8_t _vic20_via1_in(int port_id, void* user_data) {
    // FIXME
    return ~0;
}

static void _vic20_via2_out(int port_id, uint8_t data, void* user_data) {
    // FIXME
}

static uint8_t _vic20_via2_in(int port_id, void* user_data) {
    // FIXME
    return ~0;
}

static uint16_t _vic20_vic_fetch(uint16_t addr, void* user_data) {
    // FIXME
    return 0xFFFF;
}

static void _vic20_init_key_map(vic20_t* sys) {
    kbd_init(&sys->kbd, 1);
    // FIXME
}

bool vic20_quickload(vic20_t* sys, const uint8_t* ptr, int num_bytes) {
    CHIPS_ASSERT(sys && sys->valid && ptr && (num_bytes > 0));
    // FIXME
    return false;
}

#endif /* CHIPS_IMPL */