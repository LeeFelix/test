#ifndef _FB_NUMBER_H_
#define _FB_NUMBER_H_

/******************************************************************************/

#define FONTX 16
#define FONTY 20

#define FONTXD8 ((FONTX) / 8)

const unsigned char alphabet[26][FONTY][FONTXD8] = {

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01, 0xC0, 0x03,
    0xC0, 0x03, 0xE0, 0x07, 0x60, 0x06, 0x70, 0x0E, 0x30, 0x0E,
    0x38, 0x1C, 0xF8, 0x1F, 0x1C, 0x18, 0x1C, 0x38, 0x0E, 0x38,
    0x0E, 0x70, 0x07, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // A//0

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x07, 0xFE, 0x1F,
    0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x1C, 0xFE, 0x07,
    0x0E, 0x1E, 0x0E, 0x38, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x38,
    0xFE, 0x1F, 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // B//1

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x07, 0xF0, 0x1F,
    0x38, 0x38, 0x1C, 0x70, 0x0C, 0x70, 0x0E, 0x00, 0x0E, 0x00,
    0x0E, 0x00, 0x0E, 0x70, 0x0E, 0x70, 0x1C, 0x30, 0x3C, 0x38,
    0xF0, 0x1F, 0xC0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // C//2

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x01, 0xFE, 0x0F,
    0x0E, 0x1E, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x70, 0x0E, 0x70,
    0x0E, 0x70, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x1E,
    0xFE, 0x07, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // D//3

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x3F, 0xFC, 0x3F,
    0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0xFC, 0x1F,
    0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00,
    0xFC, 0x7F, 0xFC, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // E//4

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x7F, 0xFE, 0x7F,
    0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0xFE, 0x0F,
    0xFE, 0x0F, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00,
    0x0E, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // F//5

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0xF8, 0x1F,
    0x1C, 0x38, 0x1C, 0x38, 0x0E, 0x38, 0x0E, 0x00, 0x0E, 0x00,
    0x0E, 0x3F, 0x0E, 0x30, 0x0E, 0x30, 0x1C, 0x30, 0x3C, 0x38,
    0xF0, 0x3F, 0xC0, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // G//6

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x38, 0x0E, 0x38,
    0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0xFE, 0x3F,
    0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38,
    0x0E, 0x38, 0x0E, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // H//7

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01, 0xC0, 0x01,
    0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01,
    0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01,
    0xC0, 0x01, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // I//8

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x38,
    0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38,
    0x00, 0x38, 0x1C, 0x38, 0x1C, 0x38, 0x1C, 0x38, 0x1C, 0x1C,
    0xF8, 0x1F, 0xE0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // J//9

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x38, 0x0E, 0x1C,
    0x0E, 0x0E, 0x8E, 0x07, 0xCE, 0x01, 0xEE, 0x00, 0xFE, 0x01,
    0xBE, 0x03, 0x0E, 0x07, 0x0E, 0x0F, 0x0E, 0x1E, 0x0E, 0x1C,
    0x0E, 0x38, 0x0E, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // K//10

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x0E, 0x00,
    0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00,
    0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00,
    0xFE, 0x7F, 0xFE, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // L//11

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x78, 0x1E, 0x7C,
    0x3E, 0x7C, 0x3E, 0x7C, 0x3E, 0x7E, 0x7E, 0x76, 0x6E, 0x76,
    0x6E, 0x77, 0xEE, 0x73, 0xCE, 0x73, 0xCE, 0x73, 0xCE, 0x73,
    0x8E, 0x71, 0x8E, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // M//12

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x38, 0x1E, 0x38,
    0x3E, 0x38, 0x7E, 0x38, 0x6E, 0x38, 0xEE, 0x38, 0xCE, 0x39,
    0x8E, 0x3B, 0x8E, 0x3B, 0x0E, 0x3F, 0x0E, 0x3E, 0x0E, 0x3C,
    0x0E, 0x3C, 0x0E, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // N//13

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0xF8, 0x1F,
    0x1C, 0x38, 0x0E, 0x38, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70,
    0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x38, 0x1C, 0x38, 0x1C, 0x3C,
    0xF8, 0x1F, 0xE0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // O//14

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x07, 0xFC, 0x1F,
    0x1C, 0x38, 0x1C, 0x70, 0x1C, 0x70, 0x1C, 0x70, 0x1C, 0x3C,
    0xFC, 0x1F, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00,
    0x1C, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // P//15

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0xF8, 0x1F,
    0x1C, 0x38, 0x0E, 0x38, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70,
    0x0E, 0x70, 0x0E, 0x72, 0x0E, 0x3F, 0x1C, 0x3E, 0x1C, 0x3E,
    0xF8, 0x1F, 0xE0, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Q//16

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x03, 0xFE, 0x1F,
    0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0xFE, 0x1F,
    0xFE, 0x07, 0x0E, 0x07, 0x0E, 0x0E, 0x0E, 0x1C, 0x0E, 0x1C,
    0x0E, 0x38, 0x0E, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // R//17

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0xF8, 0x1F,
    0x1C, 0x1C, 0x1C, 0x38, 0x1C, 0x00, 0x78, 0x00, 0xF0, 0x07,
    0x00, 0x1F, 0x00, 0x3C, 0x0E, 0x38, 0x0E, 0x30, 0x1C, 0x38,
    0xF8, 0x1F, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // S//18

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x3F, 0xFE, 0x3F,
    0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01,
    0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01,
    0xC0, 0x01, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // T//19

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x38, 0x0E, 0x38,
    0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38,
    0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x1C, 0x38,
    0xF8, 0x1F, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // U//20

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x70, 0x0E, 0x70,
    0x0E, 0x38, 0x1C, 0x38, 0x1C, 0x1C, 0x18, 0x1C, 0x38, 0x0C,
    0x30, 0x0E, 0x70, 0x06, 0x60, 0x07, 0xE0, 0x07, 0xE0, 0x03,
    0xC0, 0x03, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // V//21

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x61, 0xC7, 0x71,
    0xC6, 0x73, 0xC6, 0x73, 0x6E, 0x73, 0x6E, 0x33, 0x6E, 0x37,
    0x6C, 0x3E, 0x6C, 0x3E, 0x3C, 0x1E, 0x3C, 0x1E, 0x3C, 0x1C,
    0x38, 0x1C, 0x18, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // W//22

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x38, 0x1C, 0x1C,
    0x38, 0x1C, 0x70, 0x0E, 0x60, 0x07, 0xE0, 0x03, 0xC0, 0x01,
    0xC0, 0x03, 0x60, 0x07, 0x70, 0x0E, 0x38, 0x0C, 0x1C, 0x1C,
    0x0E, 0x38, 0x0E, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // X//23

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x70, 0x0E, 0x38,
    0x1C, 0x18, 0x38, 0x1C, 0x70, 0x0E, 0x60, 0x07, 0xE0, 0x03,
    0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01,
    0xC0, 0x01, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Y//24

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x3F, 0xFC, 0x3F,
    0x00, 0x18, 0x00, 0x0C, 0x00, 0x06, 0x00, 0x03, 0x80, 0x01,
    0xC0, 0x01, 0xE0, 0x00, 0x70, 0x00, 0x38, 0x00, 0x1C, 0x00,
    0xFE, 0x7F, 0xFE, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Z//25

};

const unsigned char number[13][FONTY][FONTXD8] = {

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0x78, 0x0E,
    0x1C, 0x18, 0x0C, 0x38, 0x0E, 0x30, 0x0E, 0x30, 0x0E, 0x30,
    0x0E, 0x30, 0x0E, 0x30, 0x0E, 0x30, 0x1C, 0x38, 0x38, 0x1C,
    0xF0, 0x0F, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"0",0

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x03,
    0xF0, 0x03, 0xB8, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03,
    0x80, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03,
    0x80, 0x03, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"1",1

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0xF8, 0x1F,
    0x1C, 0x38, 0x0E, 0x38, 0x00, 0x38, 0x00, 0x1C, 0x00, 0x0E,
    0x00, 0x07, 0x80, 0x03, 0xC0, 0x01, 0x70, 0x00, 0x38, 0x00,
    0xFC, 0x3F, 0xFE, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"2",2

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x07, 0x78, 0x1E,
    0x1C, 0x38, 0x0C, 0x38, 0x00, 0x38, 0x00, 0x1E, 0xC0, 0x07,
    0x00, 0x1E, 0x00, 0x38, 0x00, 0x38, 0x0E, 0x38, 0x1C, 0x3C,
    0xF8, 0x0F, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"3",3

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x0E,
    0x00, 0x0F, 0x80, 0x0F, 0xC0, 0x0C, 0x60, 0x0C, 0x38, 0x0C,
    0x1C, 0x0C, 0x0E, 0x0C, 0xFF, 0x7F, 0xFF, 0x7F, 0x00, 0x0C,
    0x00, 0x0C, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"4",4

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x1F, 0xF8, 0x1F,
    0x18, 0x00, 0x1C, 0x00, 0x0C, 0x00, 0xFC, 0x0F, 0x0E, 0x1C,
    0x00, 0x38, 0x00, 0x30, 0x00, 0x30, 0x06, 0x38, 0x0E, 0x1C,
    0xFC, 0x0F, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"5",5

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x03,
    0xC0, 0x01, 0xE0, 0x00, 0x70, 0x00, 0xF8, 0x0F, 0x3C, 0x3C,
    0x1C, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x1C, 0x38,
    0xF8, 0x1F, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"6",6

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x7F, 0xFE, 0x7F,
    0x00, 0x38, 0x00, 0x18, 0x00, 0x0C, 0x00, 0x06, 0x00, 0x07,
    0x80, 0x03, 0x80, 0x01, 0xC0, 0x01, 0xC0, 0x00, 0xE0, 0x00,
    0x60, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"7",7

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x07, 0x78, 0x1F,
    0x0C, 0x38, 0x0C, 0x38, 0x0C, 0x18, 0x3C, 0x1E, 0xF0, 0x07,
    0x3C, 0x1E, 0x0E, 0x38, 0x06, 0x30, 0x0E, 0x30, 0x0E, 0x38,
    0xFC, 0x1F, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"8",8

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x07, 0xFC, 0x1F,
    0x0E, 0x38, 0x0E, 0x38, 0x06, 0x38, 0x06, 0x38, 0x0E, 0x1C,
    0xFC, 0x0F, 0xE0, 0x0F, 0x00, 0x07, 0x80, 0x03, 0xC0, 0x01,
    0xE0, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"9",9

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x7F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //"-",10

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0x01, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //":",11

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //" ",12

};
/******************************************************************************/

/*******************************************************************************
#define FONTX 16
#define FONTY 32

#define FONTXD8 ((FONTX) / 8)


const unsigned char number[13][FONTY][FONTXD8] = {

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xE0,0x07,0xF0,0x0F,
0x38,0x1C,0x1C,0x1C,0x1C,0x38,0x0C,0x38,
0x0E,0x30,0x0E,0x30,0x0E,0x30,0x0E,0x30,
0x0E,0x30,0x0E,0x30,0x0E,0x30,0x0E,0x30,
0x0E,0x30,0x0C,0x38,0x1C,0x38,0x1C,0x18,
0x38,0x1C,0xF8,0x0F,0xE0,0x07,0x80,0x01,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//0//0

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x80,0x03,0x80,0x03,
0xC0,0x03,0xE0,0x03,0xB8,0x03,0x98,0x03,
0x88,0x03,0x80,0x03,0x80,0x03,0x80,0x03,
0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,
0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,
0x80,0x03,0x80,0x03,0x80,0x03,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//1//1

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xF0,0x07,0xF8,0x1F,
0x3C,0x1C,0x1C,0x38,0x0E,0x38,0x00,0x38,
0x00,0x38,0x00,0x18,0x00,0x1C,0x00,0x1C,
0x00,0x0E,0x00,0x07,0x00,0x03,0x80,0x03,
0xC0,0x01,0xE0,0x00,0x70,0x00,0x38,0x00,
0x3C,0x00,0xFC,0x3F,0xFC,0x3F,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//2//2

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xE0,0x07,0xF0,0x1F,
0x38,0x1C,0x1C,0x38,0x0C,0x38,0x08,0x38,
0x00,0x38,0x00,0x18,0x00,0x1E,0xC0,0x07,
0xC0,0x0F,0x00,0x1E,0x00,0x38,0x00,0x38,
0x00,0x38,0x08,0x30,0x0E,0x38,0x1C,0x38,
0x3C,0x1C,0xF8,0x1F,0xF0,0x07,0x80,0x01,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//3//3

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x0C,0x00,0x0E,
0x00,0x0E,0x00,0x0F,0x80,0x0F,0x80,0x0D,
0xC0,0x0D,0xE0,0x0C,0x70,0x0C,0x30,0x0C,
0x38,0x0C,0x1C,0x0C,0x0C,0x0C,0x0E,0x0C,
0xFF,0x7F,0xFF,0x7F,0xFF,0x7F,0x00,0x0C,
0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//4//4

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xF8,0x1F,0xF8,0x1F,
0x18,0x00,0x18,0x00,0x18,0x00,0x1C,0x00,
0x0C,0x00,0xEC,0x03,0xFC,0x0F,0x1E,0x1E,
0x0E,0x1C,0x00,0x38,0x00,0x38,0x00,0x30,
0x00,0x30,0x04,0x38,0x06,0x38,0x0E,0x38,
0x0E,0x1E,0xFC,0x0F,0xF8,0x07,0xE0,0x01,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//5//5

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x07,
0x80,0x03,0xC0,0x01,0xC0,0x01,0xE0,0x00,
0x70,0x00,0x70,0x00,0xF8,0x0F,0xF8,0x1F,
0x3C,0x38,0x1C,0x70,0x0E,0x70,0x0E,0x70,
0x0E,0x70,0x0E,0x70,0x0E,0x70,0x1C,0x70,
0x3C,0x38,0xF8,0x1F,0xF0,0x0F,0x80,0x03,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//6//6

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xFE,0x7F,0xFE,0x7F,
0x00,0x70,0x00,0x38,0x00,0x18,0x00,0x1C,
0x00,0x0C,0x00,0x0E,0x00,0x06,0x00,0x07,
0x00,0x03,0x80,0x03,0x80,0x03,0x80,0x01,
0xC0,0x01,0xC0,0x01,0xC0,0x00,0xE0,0x00,
0xE0,0x00,0x60,0x00,0x70,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//7//7

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xF0,0x07,0xF8,0x0F,
0x1C,0x1C,0x0C,0x38,0x0C,0x38,0x0C,0x38,
0x0C,0x38,0x1C,0x1C,0x3C,0x1E,0xF0,0x07,
0xF8,0x0F,0x3C,0x1E,0x1E,0x38,0x0E,0x38,
0x06,0x30,0x06,0x30,0x0E,0x30,0x0E,0x38,
0x1E,0x3C,0xFC,0x1F,0xF8,0x0F,0xC0,0x01,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//8//8

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xF0,0x07,0xF8,0x0F,
0x3C,0x1E,0x0E,0x3C,0x0E,0x38,0x06,0x38,
0x06,0x38,0x06,0x38,0x06,0x38,0x0E,0x1C,
0x1E,0x1E,0xFC,0x1F,0xF8,0x0F,0x00,0x07,
0x00,0x07,0x80,0x03,0x80,0x03,0xC0,0x01,
0xC0,0x01,0xE0,0x00,0x60,0x00,0x70,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//9//9

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x7F,
0xFE,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//-//10

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xC0,0x01,0xC0,0x01,0xC0,0x01,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xC0,0x01,0xC0,0x01,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//://11

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,// //12

};
*******************************************************************************/

/*******************************************************************************
#define FONTX 8
#define FONTY 16

#define FONTXD8 ((FONTX) / 8)


extern const unsigned char number[13][FONTY][FONTXD8] = {

0x00,0x00,0x00,0x3C,0x26,0x42,0x42,0x42,
0x42,0x42,0x42,0x62,0x34,0x18,0x00,0x00,//0//0

0x00,0x00,0x00,0x10,0x18,0x1E,0x1A,0x18,
0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,//1//1

0x00,0x00,0x00,0x3C,0x66,0x42,0x60,0x20,
0x30,0x10,0x08,0x04,0x7E,0x7E,0x00,0x00,//2//2

0x00,0x00,0x00,0x3C,0x66,0x42,0x60,0x30,
0x30,0x60,0x40,0x42,0x26,0x18,0x00,0x00,//3//3

0x00,0x00,0x00,0x20,0x30,0x38,0x28,0x24,
0x26,0x22,0xFF,0x20,0x20,0x20,0x00,0x00,//4//4

0x00,0x00,0x00,0x7C,0x06,0x02,0x0A,0x3E,
0x62,0x40,0x40,0x63,0x36,0x1C,0x00,0x00,//5//5

0x00,0x00,0x00,0x10,0x18,0x08,0x0C,0x3E,
0x46,0xC2,0xC2,0x42,0x66,0x3C,0x00,0x00,//6//6

0x00,0x00,0x00,0x7E,0x40,0x60,0x20,0x30,
0x10,0x18,0x08,0x08,0x0C,0x0C,0x00,0x00,//7//7

0x00,0x00,0x00,0x3C,0x62,0x42,0x62,0x3E,
0x3E,0x42,0x43,0x42,0x66,0x3C,0x00,0x00,//8//8

0x00,0x00,0x00,0x3C,0x62,0x43,0x43,0x63,
0x36,0x3C,0x10,0x18,0x08,0x0C,0x00,0x00,//9//9

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//-//10

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,
0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,//://11

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,// //12

};
*******************************************************************************/

#endif
