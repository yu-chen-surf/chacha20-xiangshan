# Optimization of the ChaCha20 Encryption Algorithm on XiangShan Processor

## Problem Description

ChaCha20 is a stream cipher algorithm used to generate pseudorandom number streams. It is one of the stream encryption algorithms in the TLS protocol, which is commonly used when accessing HTTPS web pages in daily life. The design goal of the ChaCha20 algorithm is to provide an efficient and secure software encryption solution when hardware does not support encryption algorithms such as AES.

The core of the ChaCha20 algorithm is a 4x4 matrix, where each element is a 32-bit unsigned integer. The input of the ChaCha20 algorithm includes a 256-bit key, a 64-bit counter, and a 64-bit nonce. **In our program, these inputs are represented by a 16-element array state of 32-bit unsigned integers**. The output of the ChaCha20 algorithm is a pseudorandom number stream; by performing an XOR operation between the data to be encrypted and this pseudorandom number stream, the encrypted data can be obtained.

In this repository, we provide a C-language reference implementation of the ChaCha20 encryption algorithm from OpenSSL, which can be found in the file `src/host/chacha20_c.c`. We expect you to optimize this algorithm for the XiangShan Processor, following the algorithm definition format below:

```c
typedef union {
    u32 u[16];
    u8 c[64];
} chacha_buf;

void chacha20(chacha_buf *output, const u32 input[16]);
```

Modify the file `src/asm/chacha20.s` to make the algorithm run faster on the XiangShan Processor.

Currently, the baseline implementation is generated using `GCC 14.2` with the following command:
`riscv64-linux-gnu-gcc -O3 -march=rv64gc -Isrc -S -fverbose-asm src/host/chacha20_c.c`

## Framework Usage

### Environment Preparation

For x86-64 Debian / Ubuntu systems, you can install the required dependencies using the following command:

```bash
sudo apt install make wget gcc gcc-riscv64-linux-gnu qemu-user python3-venv
```
We have precompiled the XiangShan emulator (xs-emu) into a statically linked x86-64 Linux binary based on the [commit 1f23fd0f52e1d022ef0a6c2a206d3733263bac23](https://github.com/OpenXiangShan/XiangShan/tree/1f23fd0f52e1d022ef0a6c2a206d3733263bac23). The Makefile is already configured with a network URL to download this binary.

For participants using other platforms, you may compile the XiangShan emulator yourself using the aforementioned commit. Alternatively, configure binary translation tools such as qemu-user or Rosetta independently. If you encounter significant difficulties, please contact us to request an x86-64 cloud server.

### Makefile

Optional Parameters:

- `BENCH_LEN`: Length of the array used for testing.

Target Commands

- `make validate`: Verify whether the optimized algorithm runs correctly on QEMU.
- `make run`: Run the optimized algorithm on the XiangShan emulator and view performance metrics.
- `make validate-rtl`: Verify whether the optimized algorithm runs correctly on the XiangShan RTL (Register-Transfer Level) emulator.
- `make clean`: Clean up compiled artifacts.

### Performance Verification

For small-scale data verification, use the `make run` command. This command runs the optimized algorithm on the XiangShan Processor and outputs performance metrics.

```console
$ make run
./xs-emu -i ./chacha20_baremetal-256.bin --no-diff 2>/dev/null
xs-emu compiled at Mar 28 2025, 11:26:20
Using simulated 32768B flash
Using simulated 8386560MB RAM
The image is ./chacha20_baremetal-256.bin
Test Start!
Initial state generated for LEN=256 SEED=0xdeadbeef
Cycles: 143409
Final output:
ans[0] = 0xbe64d5a5
ans[1] = 0x83c6c3fa
ans[2] = 0xac710378
ans[3] = 0x858f0082
ans[4] = 0x207f2dc2
ans[5] = 0xeb5ee49e
ans[6] = 0x4f801257
ans[7] = 0x2e4f03c9
ans[8] = 0x74b07deb
ans[9] = 0x3e6dde54
ans[10] = 0x6dd3ddad
ans[11] = 0xee18cf6c
ans[12] = 0x053765e6
ans[13] = 0x79f814eb
ans[14] = 0x40b18db8
ans[15] = 0xa299c057
Core 0: HIT GOOD TRAP at pc = 0x80010002
Core-0 instrCnt = 452,265, cycleCnt = 171,941, IPC = 2.630350
Seed=0 Guest cycle spent: 171,945 (this will be different from cycleCnt if emu loads a snapshot)
Host time spent: 16,376ms
```

Among the output, we observe `Cycles: 143409`, which indicates that the time taken to process data of this size is `143409` clock cycles. A smaller value indicates better performance.

## Evaluation Criteria

The data size used for scoring is: `BENCH_LEN=4096`

Basic Requirement:
- The optimized algorithm must run correctly on the XiangShan Processor RTL emulator
    Verification method: Pass the test via `make validate-rtl BENCH_LEN=4096`.

Performance Requirement:
Participants will be ranked based on the number of clock cycles their optimized algorithm consumes when running on the XiangShan Processor RTL emulator. The clock cycle count is obtained using the command `make run BENCH_LEN=4096`. A smaller number of cycles results in a higher ranking.

## Rules

The submitted code must be the modified version of `src/chacha20.s`. If you need to use new extended instructions (that have been implemented in the XiangShan RTL of this version), you may modify the `-march` parameter in the Makefile. If you need to modify CSR (Control and Status Register) registers, you may adjust the entire framework, but you must provide a detailed explanation in the documentation.

Participants are allowed to submit code optimized via compiler auto-vectorization, or build further optimizations based on existing open-source implementations. A detailed documentation must be attached, specifying the source of the reference algorithm, compiler and parameter changes, and a comparison of performance before and after optimization.

Participants are prohibited from implementing any optimizations that break the algorithm’s generality. Examples of such invalid optimizations include:
- Designs that fail to support all possible state inputs.
- Directly outputting fixed results (instead of dynamic computation based on inputs).

## Problem Analysis

### ChaCha20 Algorithm & RISC-V Instruction Set

First, let’s examine the simplest C-language implementation of the ChaCha20 algorithm:

```c
# define ROTATE(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

/* QUARTERROUND updates a, b, c, d with a ChaCha "quarter" round. */
# define QUARTERROUND(a,b,c,d) ( \
x[a] += x[b], x[d] = ROTATE((x[d] ^ x[a]),16), \
x[c] += x[d], x[b] = ROTATE((x[b] ^ x[c]),12), \
x[a] += x[b], x[d] = ROTATE((x[d] ^ x[a]), 8), \
x[c] += x[d], x[b] = ROTATE((x[b] ^ x[c]), 7)  )

/* chacha_core performs 20 rounds of ChaCha on the input words in
 * |input| and writes the 64 output bytes to |output|. */
void chacha20(chacha_buf *output, const u32 input[16])
{
    u32 x[16];
    int i;

    for (int i = 0; i < 16; i++) {
        x[i] = input[i];
    }

    for (i = 20; i > 0; i -= 2) {
        QUARTERROUND(0, 4, 8, 12);
        QUARTERROUND(1, 5, 9, 13);
        QUARTERROUND(2, 6, 10, 14);
        QUARTERROUND(3, 7, 11, 15);
        QUARTERROUND(0, 5, 10, 15);
        QUARTERROUND(1, 6, 11, 12);
        QUARTERROUND(2, 7, 8, 13);
        QUARTERROUND(3, 4, 9, 14);
    }

    for (i = 0; i < 16; ++i)
        output->u[i] = x[i] + input[i];
}
```

We can see that the core computational operation of ChaCha20 is `ROTATE` (cyclic left shift). On the RV64GC instruction set, this operation is typically implemented using three instructions: `slli`, `srli`, and `or`. However, the Zbb extension of RISC-V provides the `rol` instruction, which can directly implement cyclic left shifts—this instruction can be used to optimize the performance of the ChaCha20 algorithm. Furthermore, the RISC-V Vector and Zvbb extensions also include instructions with similar functionalities; it is up to participants to conduct further optimizations based on relevant instruction set manuals and documentation.

We recommend that after implementing instruction set extensions, participants analyze performance bottlenecks using the performance counter information provided by the XiangShan emulator (this information is output to the stderr of xs-emu; you can modify the Makefile from `./xs-emu 2> /dev/null` to `./xs-emu 2> xs.log`). Further optimizations can be achieved by adjusting instruction scheduling, register allocation, Vector lmul parameters, and other settings to improve performance.

## XiangShan Single-Step Debugging

The XiangShan Processor is an out-of-order superscalar processor that can execute multiple instructions simultaneously per cycle. To understand execution details, you can perform interactive debugging using the following methods:

```bash
make xspdb-depends  # Download the Python-based XiangShan (same commit as the emulator), download spike-dasm, and install Python dependencies
make debug    # Start debugging
```

If everything works correctly, you will see output similar to the following:

```
> chacha20-xiangshan/test.py(26)test_sim_top()
-> while True:
(XiangShan)
```

Type the `xui` command and press Enter to enter the interactive debugging interface. Common debugging commands are as follows:

1. `xstep <n>` Execute n clock cycles (e.g., `xstep 10000`).
1. `xistep [n]` Execute until the next 1 or n instruction commits.
1. `xreset` Reset the circuit.
1. `xload` Load a specified binary file.
1. `xwatch SimTop_top.SimTop.timer` Monitor the current cycle count.

For other commands, use the `help` command to view details. Some commands support tab completion (e.g., xload). For reference on XSPdb, visit: [https://github.com/OpenXiangShan/XSPdb](https://github.com/OpenXiangShan/XSPdb). Users can refer to the [Pdb Manual](https://docs.python.org/3/library/pdb.html) and add custom commands in `XSPython/XSPdb/xspdb.py`.


## Recommended Reading

1. [RISC-V Zvbb Assembly Implementation of the ChaCha20 Encryption Algorithm in OpenSSL](https://github.com/openssl/openssl/blob/openssl-3.5.0-beta1/crypto/chacha/asm/chacha-riscv64-v-zbb.pl)

2. [Implementation of ChaCha20 Algorithm Optimization Using the Zbb Extension in OpenSSL](https://github.com/openssl/openssl/commit/ca6286c382a7eb527fac9aba2a018354acb27b16)
