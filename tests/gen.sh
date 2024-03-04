#!/usr/bin/bash

#cut -d '#' -f '1' "$1" | sed 's/\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/g' | xxd -e -r -ps > "$2"

BASENAME="$(echo "$1" | rev | cut -f 2- -d '.' | rev)"

riscv64-unknown-elf-as "$1" -o "${BASENAME}.o"
riscv64-unknown-elf-ld -T ./linker.ld "${BASENAME}.o" -o "${BASENAME}.elf"
riscv64-unknown-elf-objcopy -O binary "${BASENAME}.elf" "${BASENAME}.bin"
chmod -x "${BASENAME}.bin"

rm "${BASENAME}.o" "${BASENAME}.elf"
