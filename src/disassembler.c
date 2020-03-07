#include <stdio.h>
#include <stdlib.h>

int readNextByte(FILE *fp, int *instructionPointer) {
    (*instructionPointer)++;
    return fgetc(fp);
}

char *getLittleIndian2HexBytes(FILE *fp, int *instructionPointer) {
    static char hexBytes[3];
    int lowBit = readNextByte(fp, instructionPointer);
    int highBit = readNextByte(fp, instructionPointer);
    sprintf(hexBytes, "%02X%02X", highBit, lowBit);
    return hexBytes;
}

char *getOneHexByte(FILE *fp, int *instructionPointer) {
    static char hexByte[2];
    sprintf(hexByte, "%02X", readNextByte(fp, instructionPointer));
    return hexByte;
}

char *getRegisterPairInBits23(int opCode) {
    switch ((opCode & 0x30) >> 4) {
        case 0b00:
            return "B";
        case 0b01:
            return "D";
        case 0b10:
            return "H";
        case 0b11:
            return "SP";
        default:
            fprintf(stderr, "Invalid opcode %02X", opCode);
            exit(1);
    }
}

char *getRegisterPairForStackOperations(int opCode) {
    switch ((opCode & 0x30) >> 4) {
        case 0b00:
            return "B";
        case 0b01:
            return "D";
        case 0b10:
            return "H";
        case 0b11:
            return "PSW";
        default:
            fprintf(stderr, "Invalid opcode %02X", opCode);
            exit(1);
    }
}

char *getStaxLdaxRegisterPair(int opCode) {
    if (opCode & 0x10) {
        return "D";
    } else {
        return "B";
    }
}

char *getRegister(int threeBits) {
    switch (threeBits) {
        case 0:
            return "B";
        case 1:
            return "C";
        case 2:
            return "D";
        case 3:
            return "E";
        case 4:
            return "H";
        case 5:
            return "L";
        case 6:
            return "M";
        case 7:
            return "A";
        default:
            fprintf(stderr, "Invalid register bits %02X", threeBits);
            exit(1);
    }
}

char *getInrDcrLxiRegister(int opCode) {
    return getRegister((opCode & 0x38) >> 3);
}

char *getMoveRegisters(int opCode) {
    char *src = getRegister(opCode & 0x7);
    char *dst = getRegister((opCode >> 3) & 0x7);
    static char moveRegisters[4];
    sprintf(moveRegisters, "%s,%s", dst, src);
    return moveRegisters;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: program FILE");
        return 1;
    }

    char *binaryFileName = argv[1];

    FILE *binaryFile = fopen(binaryFileName, "rb");
    int opCode;
    int instructionPointer = 0;
    while ((opCode = readNextByte(binaryFile, &instructionPointer)) != EOF) {
        printf("%04X: ", instructionPointer - 1);
        // OxOO
        if (opCode == 0x00) {
            // No operation
            printf("NOP");
        } // 0x01, 0x11, 0x21, 0x31
        else if ((opCode & 0xCF) == 0x01) {
            // Format: LXI rp,data
            // rp can be B, D, H, or SP
            // data is a 16-bit quantity.
            // Loads 2 bytes immediate data into the register pair.
            // The higher 8 bits of the immediate data is loaded into the first register of the pair (e.g. C),
            // while the lower 8 bits of the immediate data is loaded into the second register of the pair (e.g. D).
            printf("LXI %s,#$%s", getRegisterPairInBits23(opCode),
                   getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0x02, 0x12
        else if ((opCode & 0xEF) == 0x02) {
            // Format: STAX rp
            // rp can be B or C.
            // Stores the content of the accumulator to the memory location addressed by the registers B and C,
            // or C and D.
            printf("STAX %s", getStaxLdaxRegisterPair(opCode));
        } // 0x03, 0x13, 0x23, 0x33
        else if ((opCode & 0xCF) == 0x03) {
            // Format: INX rp
            // rp can be B, D, H, or SP
            // Increments the 16 bit data held in the specified register by one.
            printf("INX %s", getRegisterPairInBits23(opCode));
        } // 0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C
        else if ((opCode & 0xC7) == 0x04) {
            // Format: INR reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: Z, S, P, AC
            // Increments the specified register or memory location by one.
            // If a memory reference is specified, then the memory byte addressed by H and L registers is operated upon.
            printf("INR %s", getInrDcrLxiRegister(opCode));
        } // 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D
        else if ((opCode & 0xC7) == 0x05) {
            // Format: DCR reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: Z, S, P, AC
            // Decrements the specified register or memory location by one.
            // If a memory reference is specified, then the memory byte addressed by H and L registers is operated upon.
            printf("DCR %s", getInrDcrLxiRegister(opCode));
        } // 0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E
        else if ((opCode & 0xC7) == 0x06) {
            // Format: MVI reg,data
            // reg can be B, C, D, E, H, L, M (memory), or A
            // data is a 8-bit quantity.
            // Loads 1 byte immediate data into the register or memory location.
            // If a memory reference is specified, then the memory byte addressed by H and L registers is operated upon.
            printf("MVI %s,#$%s", getInrDcrLxiRegister(opCode), getOneHexByte(binaryFile, &instructionPointer));
        } // 0x07
        else if (opCode == 0x07) {
            // Rotate the content of the accumulator one bit to the left.
            // The carry bit is set equal to the high-order bit of the accumulator.
            printf("RLC");
        } // 0x09, 0x19, 0x29, 0x39
        else if ((opCode & 0xCF) == 0x09) {
            // Format: DAD rp
            // rp can be B, D, H, or SP
            // Flags affected: CY
            // The 16 bit number in the specified register pair is added to the 16 bit number held in the H and L
            // registers. The result replaces the contents of the H and L registers.
            printf("DAD %s", getRegisterPairInBits23(opCode));
        } // 0x0A, 0x1A
        else if ((opCode & 0xEF) == 0x0A) {
            // Format: LDAX rp
            // rp can be B or C.
            // Loads the content of the memory location addressed by the registers B and C or C and D to the
            // accumulator.
            printf("LDAX %s", getStaxLdaxRegisterPair(opCode));
        } // 0x0B, 0x1B, 0x2B, 0x3B
        else if ((opCode & 0xCF) == 0x0B) {
            // Format: DCX rp
            // rp can be B, D, H, or SP
            // Decrements the 16 bit data held in the specified register by one.
            printf("DCX %s", getRegisterPairInBits23(opCode));
        } // 0xF
        else if (opCode == 0x0F) {
            // Rotate the content of the accumulator one bit to the right.
            // The carry bit is set equal to the low-order bit of the accumulator.
            printf("RRC");
        } // 0x17
        else if (opCode == 0x17) {
            // Rotate the content of the accumulator one bit to the left, through the carry bit.
            // The high-order bit of the accumulator replaces the carry bit,
            // while the carry bit replaces the low-order bit of the accumulator.
            printf("RAL");
        } // 0x1F
        else if (opCode == 0x1F) {
            // Rotate the content of the accumulator one bit to the right, through the carry bit.
            // The low-order bit of the accumulator replaces the carry bit,
            // while the carry bit replaces the high-order bit of the accumulator.
            printf("RAR");
        } // 0x22
        else if (opCode == 0x22) {
            // Format: SHLD addr
            // addr is a 16-bit value
            // The content of the L register is stored at the 16-bit memory address.
            // The content of the H register is stored at the next higher memory address.
            printf("SHLD %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0x27
        else if (opCode == 0x27) {
            // The 8-bit hexadecimal number in the accumulator is converted to two 4-bit binary coded decimal digits.
            // This is a two step process:
            // 1. If the least significant 4 bits in the accumulator is greater than 9, or, if the AC (auxiliary carry)
            //   flag is set, then the accumulator is incremented by six. If a carry out of the least four significant
            //   bits occurs, then AC is set. Otherwise it is reset.
            // 2. If the most significant 4 bits in the accumulator is greater than 9, or, if the CY (carry) flag is
            //   set, then the most significant 4 bits of the accumulator are incremented by six. If a carry out of the
            //   most significant four bits occurs, then CY is set. Otherwise it is unaffected.
            //
            // Flags affected: Z, S, P, CY, AC
            printf("DAA");
        } // 0x2A
        else if (opCode == 0x2A) {
            // Format: LHLD addr
            // addr is a 16-bit value
            // The byte at the 16-bit memory address is stored in the L register.
            // The byte at the next higher memory address is stored in the H register.
            printf("LHLD %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0x2F
        else if (opCode == 0x2F) {
            // Each bit in the accumulator is complemented
            printf("CMA");
        } // 0x32
        else if (opCode == 0x32) {
            // Format: STA addr
            // addr is a 16-bit value
            // The contents of the accumulator replaces the byte at the specified memory address
            printf("STA %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0x4A
        else if (opCode == 0x4A) {
            // Format: LDA addr
            // addr is a 16-bit value
            // The byte at the specified memory address replaces the contents of the accumulator
            printf("LDA %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0x37
        else if (opCode == 0x37) {
            // Set the carry bit to 1
            printf("STC");
        } // 0x3F
        else if (opCode == 0x3F) {
            // Complement the carry bit
            printf("CMC");
        } // 0x76
        else if (opCode == 0x76) {
            printf("HLT");
        } // 0x40-0x7F, except 0x76
        else if ((opCode & 0xC0) == 0x40) {
            // Format MOV dst, src
            // dst or src can be B, C, D, E, H, L, M (memory), or A
            printf("MOV %s", getMoveRegisters(opCode));
        } // 0x80-0x87
        else if ((opCode & 0xF8) == 0x80) {
            // Format ADD, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P, AC
            // The specified byte is added to the content of the accumulator
            printf("ADD %s", getRegister(opCode & 0x7));
        } // 0x88-0x8f
        else if ((opCode & 0xF8) == 0x88) {
            // Format ADC, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P, AC
            // The specified byte plus the carry bit is added to the content of the accumulator
            printf("ADC %s", getRegister(opCode & 0x7));
        } // 0x90-0x97
        else if ((opCode & 0xF8) == 0x90) {
            // Format SUB, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P, AC
            // The specified byte is subtracted from the content of the accumulator.
            // If there is no carry out of the highest order bit, it indicates that a borrow occurred.
            // In that case the carry bit is set, otherwise it is reset.
            printf("SUB %s", getRegister(opCode & 0x7));
        } // 0x98-0x9F
        else if ((opCode & 0xF8) == 0x98) {
            // Format SBB, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P, AC
            // The specified byte and the carry bit is subtracted from the content of the accumulator.
            // If there is no carry out of the highest order bit, it indicates that a borrow occurred.
            // In that case the carry bit is set, otherwise it is reset.
            printf("SBC %s", getRegister(opCode & 0x7));
        } // 0xA0-0xA7
        else if ((opCode & 0xF8) == 0xA0) {
            // Format ANA, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P
            // The specified byte is ANDed to the content of the accumulator. The carry bit is reset to zero.
            printf("ANA %s", getRegister(opCode & 0x7));
        } // 0xA8-0xAF
        else if ((opCode & 0xF8) == 0xA8) {
            // Format XRA, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P, AC
            // The specified byte is XORed to the content of the accumulator. The carry bit is reset to zero.
            printf("XRA %s", getRegister(opCode & 0x7));
        } // 0xB0-0xB7
        else if ((opCode & 0xF8) == 0xB0) {
            // Format ORA, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P
            // The specified byte is ORed to the content of the accumulator. The carry bit is reset to zero.
            printf("ORA %s", getRegister(opCode & 0x7));
        } // 0xB8-0xBF
        else if ((opCode & 0xF8) == 0xB8) {
            // Format CMP, reg
            // reg can be B, C, D, E, H, L, M (memory), or A
            // Flags affected: CY, S, Z, P
            // The specified byte is compared to the content of the accumulator. This is done by subtracting the byte
            // from the accumulator content, but leaving both reg and accumulator unchanged. The condition bits are
            // set according to the result, in particular, the zero bit set if the contents are equal, otherwise it
            // is reset.
            printf("CMP %s", getRegister(opCode & 0x7));
        } // 0xC0
        else if (opCode == 0xC0) {
            // Returns if the zero bit is not set
            printf("RNZ");
        } // 0xC1, 0xD1, 0xE1, 0xF1
        else if ((opCode & 0xCF) == 0xC1) {
            // Format: POP rp
            // reg can be B, D, H, or PSW
            // Flags affected: CY, S, Z, P only if reg is PSW
            // The contents of the specified registers are restored from the stack. The content of the first register
            // is restored from the byte addressed by the stack pointer, and the content of the second register is
            // restored from the byte at the address one greater than address indicated by the stack pointer.
            // If PSW is specified, then the state of the five condition bits are restored.
            // The stack pointer is incremented by two after this operation.
            printf("POP %s", getRegisterPairForStackOperations(opCode));
        } // 0xC5, 0xD5, 0xE5, 0xF5
        else if ((opCode & 0xCF) == 0xC5) {
            // Format: PUSH rp
            // reg can be B, D, H, or PSW
            // Flags affected: CY, S, Z, P only if reg is PSW
            // The contents of the specified registers are stored in the stack. The content of the first register
            // is stored at the byte addressed by the stack pointer, and the content of the second register is
            // is stored at the byte at the address one greater than address indicated by the stack pointer.
            // If PSW is specified, then the state of the five condition bits is stored.
            // The stack pointer is decremented by two after this operation.
            printf("PUSH %s", getRegisterPairForStackOperations(opCode));
        } // 0xC2
        else if (opCode == 0xC2) {
            // Jump to the specified address if zero bit is unset.
            printf("JNZ %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xC3
        else if (opCode == 0xC3) {
            // Jump to the specified address.
            printf("JMP %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xC4
        else if (opCode == 0xC4) {
            // A call operation is performed to the address if the zero bit is unset.
            printf("CNZ %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xC6
        else if (opCode == 0xC6) {
            // Format: ADI data
            // data is a 8 byte value
            // The byte of immediate data is added to the accumulator.
            printf("ADI %s", getOneHexByte(binaryFile, &instructionPointer));
        } // 0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF
        else if ((opCode & 0xC7) == 0xC7) {
            // Format: RST exp
            // exp is a 3-bit value.
            // The content of the program counter is pushed onto the stack, so that a subsequent RETURN instruction can
            // return to that address. Program execution continues at the memory address 0b000000000EXP000B.
            // Normally this instruction is used for interrupt handling.
            printf("RST %x", (opCode & 0x38) >> 3);
        } // 0xC8
        else if (opCode == 0xC8) {
            // Returns if the zero bit set.
            printf("RZ");
        } // 0xC9
        else if (opCode == 0xC9) {
            // Returns to the instruction immediately following the last call instruction.
            printf("RET");
        } // 0xCA
        else if (opCode == 0xCA) {
            // Jump to the specified address if zero bit is set.
            printf("JZ %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xCC
        else if (opCode == 0xCC) {
            // A call operation is performed to the address if the zero bit is set.
            printf("CZ %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xCD
        else if (opCode == 0xCD) {
            // A call operation is unconditionally performed.
            printf("CALL %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xCE
        else if (opCode == 0xCE) {
            // Format: ACI data
            // data is a 8 byte value
            // The byte of immediate data is added to the accumulator along with carry bit.
            printf("ACI %s", getOneHexByte(binaryFile, &instructionPointer));
        } // 0xD0
        else if (opCode == 0xD0) {
            // Returns if the carry bit is unset.
            printf("RNC");
        } // 0xD2
        else if (opCode == 0xD2) {
            // Jump to the specified address if carry bit is not set.
            printf("JNC %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xD8
        else if (opCode == 0xD8) {
            // Returns if the carry bit is set.
            printf("RC");
        } // 0xDA
        else if (opCode == 0xDA) {
            // Jump to the specified address if carry bit is set.
            printf("JC %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xE0
        else if (opCode == 0xE0) {
            // Returns if the parity bit is zero (odd parity).
            printf("RPO");
        } // 0xE2
        else if (opCode == 0xE2) {
            // Jump to the specified address if the parity bit is zero.
            printf("JPO %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xE8
        else if (opCode == 0xE8) {
            // Returns if the parity bit is set (even parity).
            printf("RPE");
        } // 0xEA
        else if (opCode == 0xEA) {
            // Jump to the specified address if the parity bit is set.
            printf("JPE %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xF0
        else if (opCode == 0xF0) {
            // Returns if the sign bit is zero (indicating a positive result).
            printf("RP");
        } // 0xF2
        else if (opCode == 0xF2) {
            // Jump to the specified address if the sign bit is zero.
            printf("JP %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        } // 0xF8
        else if (opCode == 0xF8) {
            // Returns if the sign bit is one (indicating a minus result).
            printf("RM");
        } // 0xFA
        else if (opCode == 0xFA) {
            // Jump to the specified address the sign bit is one.
            printf("JM %s", getLittleIndian2HexBytes(binaryFile, &instructionPointer));
        }
        printf("\n");
    }

    fclose(binaryFile);

    return 0;
}
