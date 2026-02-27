#include <sys/stat.h>
#include <errno.h>

int _close(int file) {
    return -1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _read(int file, char *ptr, int len) {
    return 0;
}

int _write(int file, char *ptr, int len) {
    return len;
}

void *_sbrk(ptrdiff_t incr) {
    extern char _end;
    static char *heap_end;
    char *prev;

    if (heap_end == 0) {
        heap_end = &_end;
    }

    prev = heap_end;
    heap_end += incr;
    return prev;
}