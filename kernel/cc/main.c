#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "DOOM/DOOM.h"
#include "DOOM/m_random.h"
#include "DOOM/doom_config.h"
#include "DOOM/i_video.h"

extern const uint8_t *wad_bytes();
extern size_t wad_len();

static const uint8_t *WAD_BYTES;
static size_t POSITION;

void* dangling() {
    return (void*)16;
}

void *DoomMalloc(int n) {
    return malloc(n);
}

void DoomPrint(const char *s) {
    puts(s);
}

void DoomGetTime(int *sec, int *usec) {
    *sec = 1;
    *usec = 1;
}

void *DoomOpen(const char *file, const char *mode) {
    if (strcmp(file, "/doom1.wad") == 0) {
        return dangling();
    } else {
        return nullptr;
    }
}

void DoomClose(void *handle) {}

int DoomTell(void *handle) {
    return POSITION;
}

int DoomEof(void *handle) {
    return POSITION >= wad_len();
}

int DoomSeek(void *handle, int offset, doom_seek_t type) {
    if (type == DOOM_SEEK_SET) {
        POSITION = offset;
    }
    if (type == DOOM_SEEK_CUR) {
        POSITION += offset;
    }
    if (type == DOOM_SEEK_END) {
        POSITION = wad_len() - offset;
    }
    if (POSITION > wad_len()) {
        return -1;
    } else {
        return 0;
    }
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

int DoomRead(void *handle, void *buf, int count) {
    if (POSITION >= wad_len()) {
        return 0;
    }
    int a = MIN(count, wad_len() - POSITION);
    int read = MAX(0, a);
    memcpy(buf, WAD_BYTES + POSITION, read);
    POSITION += read;
    return read;
}

char *DoomGetEnv(const char *env) {
    return "";
}

#define WIDTH 320
#define HEIGHT 200

extern void kernel_main(uint8_t *buf, uint32_t len) {
    WAD_BYTES = wad_bytes();

    char* argv[] = {};

    doom_set_file_io(DoomOpen, DoomClose, DoomRead, nullptr, DoomSeek, DoomTell, DoomEof);
    doom_set_malloc(DoomMalloc, free);
    doom_set_exit(exit);
    doom_set_getenv(DoomGetEnv);
    doom_set_gettime(DoomGetTime);
    doom_set_print(DoomPrint);

    doom_init(0, argv, 0);

    uint8_t palette[256 * 3] = {};

    for (int i = 0; i < 256 * 3; i++) {
        palette[i] = M_Random();
    }

    I_SetPalette(palette);

    int magic = 0;
    while (true) {
        doom_update();


        uint8_t* frame = doom_get_framebuffer(1);
        for (int i = 0; i < WIDTH * HEIGHT - 1; i++) {
            buf[i] = frame[i] + 100 + magic++;
        }

        doom_key_down(DOOM_KEY_ENTER);
        doom_key_up(DOOM_KEY_ENTER);
    }
}
