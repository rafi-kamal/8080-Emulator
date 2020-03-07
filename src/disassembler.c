#include <stdio.h>
#include <stdlib.h>

char *getLittleIndian2HexBytes(FILE *fp) {
    static char hexBytes[3];
    int lowBit = fgetc(fp);
    int highBit = fgetc(fp);
    sprintf(hexBytes, "%02X%02X", highBit, lowBit);
    return hexBytes;
}

char *getOneHexByte(FILE *fp) {
    static char hexByte[2];
    sprintf(hexByte, "%02X", fgetc(fp));
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

char *getStaxLdaxRegisterPair(int opCode) {
    if (opCode & 0x10) {
        return "D";
    } else {
        return "B";
    }
}

char *getInrDcrLxiRegister(int opCode) {
    switch ((opCode & 0x38) >> 3) {
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
            fprintf(stderr, "Invalid opcode %02X", opCode);
            exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: program FILE");
        return 1;
    }

    char *binaryFileName = argv[1];

    FILE *binaryFile = fopen(binaryFileName, "rb");
    int opCode;
    while ((opCode = fgetc(binaryFile)) != EOF) {
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
            printf("LXI %s,#$%s", getRegisterPairInBits23(opCode), getLittleIndian2HexBytes(binaryFile));
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
            printf("MVI %s,#$%s", getInrDcrLxiRegister(opCode), getOneHexByte(binaryFile));
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
            printf("SHLD %s", getLittleIndian2HexBytes(binaryFile));
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
            printf("LHLD %s", getLittleIndian2HexBytes(binaryFile));
        } // 0x2F
        else if (opCode == 0x2F) {
            // Each bit in the accumulator is complemented
            printf("CMA");
        } // 0x32
        else if (opCode == 0x32) {
            // Format: STA addr
            // addr is a 16-bit value
            // The contents of the accumulator replaces the byte at the specified memory address
            printf("STA %s", getLittleIndian2HexBytes(binaryFile));
        } // 0x4A
        else if (opCode == 0x4A) {
            // Format: LDA addr
            // addr is a 16-bit value
            // The byte at the specified memory address replaces the contents of the accumulator
            printf("LDA %s", getLittleIndian2HexBytes(binaryFile));
        }
        printf("\n");
    }

    fclose(binaryFile);

    return 0;
}
