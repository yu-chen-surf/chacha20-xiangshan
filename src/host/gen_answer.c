#include <chacha20.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_LEN 1024

chacha_buf buf[BUF_LEN];

void gen_chacha_state(u32 *state, u32 seed) {
    u32 buf[16];
    for (int i = 0; i < 16; i++)
        buf[i] = seed ^ i;
    chacha20((chacha_buf *)state, buf);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("%s [len(d)] [seed(x)]\n", argv[0]);
        return 1;
    }
    int len = atoi(argv[1]);
    int seed = strtol(argv[2], NULL, 16);
    printf("len = %d, seed = 0x%08x\r\n", len, seed);
    u32 state[16];
    gen_chacha_state(state, seed);
    for (int i = 0; i < len; i++) {
        chacha20(&buf[i%BUF_LEN], state);
        state[15] ++;
    }
    for (int i = 0; i < 16; i++) {
        printf("ans[%d] = 0x%08x\r\n", i, buf[(len-1)%BUF_LEN].u[i]);
    }
    return 0;
}
