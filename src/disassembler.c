#include <stdio.h>
#include <stdlib.h>

char* getLittleIndian2HexBytes(FILE* fp) {
    static char hexBytes[3];
    int lowBit = fgetc(fp);
    int highBit = fgetc(fp);
    sprintf(hexBytes,"%02X%02X", highBit, lowBit);
    return hexBytes;
}

char* getLxiRegisterPair(int opCode) {
    switch ((opCode & 0x30) >> 4) {
        case 0b00:
            return "BC";
        case 0b01:
            return "DE";
        case 0b10:
            return "HL";
        case 0b11:
            return "SP";
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

    char* binaryFileName = argv[1];

    FILE* binaryFile = fopen(binaryFileName, "rb");
    int opCode;
    while ((opCode = fgetc(binaryFile)) != EOF) {
        if (opCode == 0x00) {
            printf("NOP");
        } else if ((opCode & 0xCF) == 0x01) {
            printf("LXI %s, #$%s", getLxiRegisterPair(opCode), getLittleIndian2HexBytes(binaryFile));
        }
        printf("\n");
    }

    fclose(binaryFile);

    return 0;
}
