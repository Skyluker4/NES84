# NES84

NES emulator for the TI-84+ CE.

In development, but not currently being actively worked on.

## Releases

None at this time.

## Use

As of right now, the program does not do anything, since is in development. Currently, the CPU *should* work, but it doesn't have anything to execute, so nothing happens.

Requires the [CE C Standard Libraries](https://github.com/CE-Programming/libraries/releases/latest) to be installed on the calculator.

## Project Structure

All necessary source files are in the src/ folder. The CPU, PPU, APU, ROM, and input are all split up into different files. The CPU is the only one that is finished right now, though. The APU may eventually be removed depending on if it is still required to emulate even though the calculator has no speaker.

## Building

Requires the [CE C Software Development Kit](https://github.com/CE-Programming/toolchain/releases/latest). Build using ```make```.
