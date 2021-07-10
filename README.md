# A RISCV Emulator written in C

This is written as a personal project, with a view to learning more about
computer architecture. This is being implemented following the RISCV specs in
pure C. The ultimate goal is to run linux for riscv on this emulator.

Currently, the following has been implemented:
    1. RV32i base instruction set
    2. RV64i base instruction set
    3. RV32/64 Zicsr standard extension
    4. RV32M standart extension
    5. RV64M standart extension

### TODO
    1. Fully implement RV64G (IMAFD extensions)
    2. Interrupt handling
    3. Trap handling
    4. UART
    5. VIRTIO
    6. Run xv6 unix 
    7. Run linux for riscv

## Build and run

In order to build and run the emulator, clone the repo and simply do the following

```bash
make
./main <binary.bin>
```

The ```binary.bin``` is the binary file to be run. A test code can be written
in c in the ```tests``` directory. Then in the tests directory, you can run
```make``` which will produce the required binary ```test.bin``` file to be
supplied to the emulator. Then you can run the produced file.

```bash
cd ./tests && make
cd ../ & ./main tests/test.bin
```

## Testing with riscv-tests

There is an official repo by riscv [here](https://github.com/riscv/riscv-tests/).
This contains the tests for each cpu operation for different sets of modular
impplemnetations. In order to run the riscv-tests on this emulator, clone the
repo and build the test executables in the ```isa``` folder. The tests are
grouped in directories.

For example, in order to test the rv64ui(user mode, integer), run the following

```bash
git clone https://github.com/riscv/riscv-tests/
cd riscv-tests/isa
make rv64ui
```

This is going to produce the riscv executables for all the instructions for the
rv64ui implmenetation and dumpfiles to check the operation for the respective
instructions. In order to produce the bin file, objcopy for riscv is required.
For that, install ```riscv-tools``` from [here](https://github.com/riscv/riscv-tests/);
Simply run:

```bash
git  clone https://github.com/riscv/riscv-tools/
cd riscv-tools & make & make linux
```

The executables going to be installed under ```/opt/riscv/bin``` by default.
Addn the path to your PATH variable in your ```.bashrc``` file.

```bash
export PATH=$PATH:/opt/riscv/bin
```

Now, you can generate the binary file running the following

```bash
riscv64-unknown-elf-objcopy -O binary riscv-tests/isa/rv32ui-p-ori tests/ori.bin
```

### test.py

A simple python script that I wrote to build all the tests in the riscv-tests
directory. You can run the following to get all the binary files in you
provided destination folder

```bash
./test.py <riscv-tests-dir> <destination-dir>
```

Currently, the emulator passes the following tests:
    1. rv32ui
    2. rv64ui
    3. rv64um
