struct _reent;
extern "C" void __malloc_lock(struct _reent*) {}
extern "C" void __malloc_unlock(struct _reent*) {}
extern "C" void blinker_wio_newlib_lock_anchor() {}
int main() { return 0; }
