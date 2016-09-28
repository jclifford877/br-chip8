#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned char chip8_fontset[80] =
{ 
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

chip8::chip8(){
}
chip8::~chip8(){
}

void chip8::init(){
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;

    for(int i=0; i<2048; i++){
        gfx[i] = 0;
    }

    for(int i=0; i<16; i++){
        V[i] = 0;
    }

    for(int i=0; i<16; i++){
        key[i] = 0;
    }

    for(int i=0; i<16; i++){
        stack[i] = 0;
    }

    for(int i=0; i<4096; i++){
        memory[i] = 0;
    }

    for(int i=0; i<80; i++){
        memory[i] = chip8_fontset[i];
    }

    delay_timer = 0;
    sound_timer = 0;

}

void chip8::emulateCycle(){
    opcode = memory[pc] << 8 | memory[pc+1];

    switch(opcode & 0xF000){
        case 0x0000:
            switch(opcode & 0x000F){
                case 0x0000: //0x00E0 clears the screen
                    for(int i=0; i<2048; i++){
                        gfx[i] = 0x0;
                    }
                    drawFlag = true;
                    pc += 2;
                break;

                case 0x000E: //0x00EE: returns from subroutine
                    sp--;
                    pc = stack[sp];
                    pc += 2;
                break;

                default:
                    printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
            }
        break;

        case 0x1000: //1NNN jumps to address NNN
            pc = opcode & 0x0FFF;
        break;

        case 0x2000: //0x2NNN calls the subroutine at address NNN
            stack[sp] = pc;
            sp++;
            pc = opcode & 0x0FFF;
        break;

        case 0x3000: //0x3XNN skips the next instruction if vx equals NN
            if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
        break;

        case 0x4000: //0x4XNN skips the next instruction if vx doesnt equal NN
            if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
        break;

        case 0x5000: //0x5XY0 skips the next instruction if vx equals vy
            if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc +=2;
        break;

        case 0x6000: //0x6XNN sets VX to NN
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
        break;

        case 0x7000: //0x7XNN adds NN to VX
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
        break;

        case 0x8000:
            switch(opcode & 0x000F){
                case 0x0000: //0x8XY0 sets the value of VX to VY
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0001: //0x8XY1 sets VX to VX or VY
                    V[(opcode & 0x0F00) >> 8] |=  V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0002: //0x8XY2 sets VX to VX and VY
                    V[(opcode & 0x0F00) >> 8] &=  V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0003: //0x8XY2 sets VX to VX xor VY
                    V[(opcode & 0x0F00) >> 8] ^=  V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0004: //0x8XY4 adds the value of VY to VX register VF is set to 1 when there is a carry and set to 0 when there isnt
                    if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x0005: //0x8XY5 VY is subtracted from VX. VF is set to 0 when there is a borrow and 1 when there isnt
                    if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
                        V[0xF] = 0;
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                break;

                case 0x006: //0x8XY6 shifts VX left by one, VF is set to the value of the most significant bit of VX before the shift
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0xF00) >> 8] >>= 1;
                    pc += 2;
                break;

                case 0x007: //0x8XY7 sets VX to VY minus VX, VF is set to 0 when theres a borrow and 1 when there isnt
                    if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
                        V[0xF] = 0;
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x000E: //0x8XYE shifts VX left by one VF is set to the value of the most significant bit of VX before the shift
                    V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                break;

                default:
                    printf("Unknown opcode [0x8000]: 0x%X\n", opcode);


            }
        break;

        case 0x9000: //0x9XY0 skips the next instruction if VX doesnt equal VY
            if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc +=2;
        break;

        case 0xA000: //ANNN: sets I to the address NNN
            I = opcode & 0x0FFF;
            pc+= 2;
        break;

        case 0xB000: //BNNN jumps to the address NNN + V0
            pc = (opcode & 0x0FFF) + V[0];
        break;

        case 0xC000: //CXNN  sets VX to the result of a bitwise and operation on a random number and NN
            V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            pc += 2;
        break;


        case 0xD000: //0xDXYN draws a sprite stored in I that is N rows high at position X, Y
            {
                unsigned short x = V[(opcode & 0x0F00) >> 8];
                unsigned short y = V[(opcode & 0x00F0) >> 4];
                unsigned short height = opcode & 0x000F;
                unsigned short pixel;
                V[0xF] = 0;
                for(int yline =0; yline<height; yline++){
                    pixel = memory[I+yline];
                    for(int xline=0; xline<8; xline++){
                        if((pixel & (0x80 >> xline)) != 0){
                            if(gfx[(x + xline + ((y + yline) * 64))] == 1)
                                V[0xF] = 1;
                            gfx[x + xline + ((y + yline) * 64)] ^= 1;
                        }
                    }
                }
                drawFlag = true;
                pc += 2;
            }
        break;

        case 0xE000:
            switch(opcode & 0x00FF){
                case 0x009E: //EX9E skips next instruction if the key stored in vx is pressed
                    if(key[V[(opcode & 0x0F00) >> 8]] != 0)
                        pc += 4;
                    else
                        pc += 2;
                break;

                case 0x00A1: //EXA1 skips next instruction if the key stored in vx isn't pressed
                    if(key[V[(opcode & 0x0F00) >> 8]] == 0)
                        pc += 4;
                    else
                        pc +=2;
                break;

                default:
                    printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
            }
        break;

        case 0xF000:
            switch(opcode & 0x00FF){
                case 0x0007: //FX07 sets vx to the value of the delay timer
                    V[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                break;

                case 0x000A: //FX0A a key press is awaited, then stored in VX
                {
                    bool keyPress = false;
                    for(int i=0; i<16; i++){
                        if(key[i] != 0){
                            V[(opcode & 0x0F00) >> 8] = i;
                            keyPress = true;
                        }
                    }

                    if(!keyPress) return;
                    pc += 2;
                }
                break;

                case 0x0015: //FX15 sets the delay timer to VX
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x0018: //FX18 sets the sound timer to VX
                    sound_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x001E: //FX1E adds VX to I
                    if(I+V[(opcode & 0x0F00) >> 8] > 0xFFF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                break;

                case 0x0029: //FX29 sets I to the location of the sprite for the character in VX characters 0-F in hex are represented by a 4x5 font
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;
                break;

                case 0x0033: //0xFX33 stores the binary coded decimal representation of vx at the addresses I, I plus 1, and I plus 2
                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I+1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I+2] = (V[(opcode & 0x0F00) >> 8] %100) %10;
                    pc += 2;
                break;

                case 0x0055: //FX55 stores V0 to VX (including VX) in memory starting at address I
                    for(int i=0; i<=((opcode & 0x0F00) >> 8); i++)
                        memory[I + i] = V[i];
                    I += ((opcode & 0x0F00) >> 8) +1;
                    pc += 2;
                break;

                case 0x0065: //FX65 fills V0 to VX (including VX) with values from memory starting at address I
                    for(int i=0; i<=((opcode & 0x0F00) >> 8); i++)
                        V[i] = memory[I + i];
                    I += ((opcode & 0x0F00) >> 8) +1;
                    pc += 2;
                break;

                default:
                    printf("Unknown opcode [0xF000]: 0x%X\n", opcode);

            }
            break;






        default:
            printf("Unknown opcode: 0x%X\n" ,opcode);
    }

    if(delay_timer > 0) delay_timer--;
    if(sound_timer > 0){
        if(sound_timer == 1) printf("BEEP!\n");
        sound_timer--;
    }
}

bool chip8::loadApplication(const char * filename){
    init();
    printf("loading: %s\n", filename);

    FILE * pFile = fopen(filename, "rb");
    if(pFile == NULL){
        fputs("File error", stderr);
        return false;
    }

    fseek(pFile, 0, SEEK_END);
    long lSize = ftell(pFile);
    rewind(pFile);
    printf("Filesize: %d\n", (int)lSize);

    char * buffer = (char*)malloc(sizeof(char) * lSize);
    if(buffer == NULL){
        fputs("Memory error", stderr);
        return false;
    }

    size_t result = fread(buffer, 1, lSize, pFile);
    if(result != lSize){
        fputs("reading error", stderr);
        return false;
    }

    if((4096-512) > lSize){
        for(int i=0; i<lSize; i++)
            memory[i + 512] = buffer[i];
    }else printf("Error: ROM too big for memory");

    fclose(pFile);
    free(buffer);
    return true;
}
