#include "gametank.h"

//must be power of 2
#define BANK_STACK_SIZE 8
//must be BANK_STACK_SIZE-1
#define BANK_WRAP_MASK 7

unsigned char romBankMirror;
unsigned char romBankStack[BANK_STACK_SIZE];
unsigned char romBankStackIdx;

void bank_shift_out(unsigned char banknum);

void change_rom_bank(unsigned char banknum) {
    if(banknum != romBankMirror)
        bank_shift_out(banknum);
}

void push_rom_bank() {
    romBankStackIdx = (romBankStackIdx + 1) & BANK_WRAP_MASK;
    romBankStack[romBankStackIdx] = romBankMirror;
}

void pop_rom_bank() {
    change_rom_bank(romBankStack[romBankStackIdx]);
    if(romBankStackIdx == 0)
        romBankStackIdx = BANK_STACK_SIZE;
    --romBankStackIdx;
}