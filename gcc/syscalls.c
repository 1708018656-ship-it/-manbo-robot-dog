/*
 * Minimal newlib syscalls for the GCC build (Keil does not use this file).
 * Without them newlib-nano links its "nosys" stubs and the linker prints
 * "_write is not implemented" warnings for every retargeting hook.
 * Nothing in the firmware reads files; printf output (if any) is dropped.
 */
#include <errno.h>
#include <sys/stat.h>

int _write(int fd, const char *buf, int len) { (void)fd; (void)buf; return len; }
int _read(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return 0; }
int _close(int fd) { (void)fd; return -1; }
int _lseek(int fd, int off, int whence) { (void)fd; (void)off; (void)whence; return 0; }
int _fstat(int fd, struct stat *st) { (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
