void demod_init(int s, int c, int b, int sp);
void demod_callback(unsigned char *buf, uint32_t len, void *ctx);
void demod_close(void);
