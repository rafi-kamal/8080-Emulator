#include <stdio.h>
#include <stdlib.h>

char* getLittleIndian2HexBytes(FILE* fp) {
    static char hexBytes[3];
    int lowBit = fgetc(fp);
    int highBit = fgetc(fp);
    sprintf(hexBytes,"%02X%02X", highBit, lowBit);
    return hexBytes;
}

char *getLxiInxRegisterPair(int opCode) {
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
        }
            // 0x01, 0x11, 0x21, 0x31
        else if ((opCode & 0xCF) == 0x01) {
            // Format: LXI rp, data
            // rp can be B, D, H, or SP
            // data is a 16-bit quantity.
            // Loads immediate data into the register pair.
            // The higher 8 bits of the immediate data is loaded into the first register of the pair (e.g. C),
            // while the lower 8 bits of the immediate data is loaded into the second register of the pair (e.g. D).
            printf("LXI %s, #$%s", getLxiInxRegisterPair(opCode), getLittleIndian2HexBytes(binaryFile));
        }
            // 0x02, 0x12
        else if ((opCode & 0xEF) == 0x02) {
            // Format: STAX rp
            // rp can be B or C.
            // Stores the content of the accumulator to the memory location addressed by the registers B and C,
            // or C and D.
            printf("STAX %s", getStaxLdaxRegisterPair(opCode));
        }
            // 0x03, 0x13, 0x23, 0x33
        else if ((opCode & 0xCF) == 0x03) {
            // Format: INX rp
            // rp can be B, D, H, or SP
            // Increments the 16 bit data held in the specified register by one.
            printf("INX %s", getLxiInxRegisterPair(opCode));
        }
        printf("\n");
    }

    fclose(binaryFile);

    return 0;
}
