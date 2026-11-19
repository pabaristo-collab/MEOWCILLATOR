#pragma once
// MEOWCILLATOR: Exact 32-step drum patterns from the original 0.9.5 WEB.
// Each bit represents a sixteenth note. Separate open hats and accents are retained.
#include <array>
#include <cstdint>

struct MeowRhythmPattern {
    const char* name;
    std::uint32_t kick;
    std::uint32_t snare;
    std::uint32_t hat;
    std::uint32_t openHat;
    std::uint32_t accent;
};

inline constexpr std::array<MeowRhythmPattern, 11> meowRhythms = {{
    {"", 0x49090941u, 0x10101010u, 0x55555555u, 0x80008000u, 0x01010101u},
    {"", 0x11111111u, 0x10101010u, 0x44444444u, 0x40004000u, 0x01010101u},
    {"", 0x45094509u, 0x90101010u, 0x55555555u, 0x00800080u, 0x04010401u},
    {"", 0x51915191u, 0x10101010u, 0xFFFFFFFFu, 0x80808080u, 0x11111111u},
    {"", 0x45454545u, 0x11101110u, 0x55555555u, 0x80008000u, 0x00110011u},
    {"", 0x13FF13FFu, 0x11551155u, 0x13FF13FFu, 0x10001000u, 0x01110111u},
    {"", 0x45454545u, 0x14101410u, 0x55555555u, 0x80008000u, 0x00410041u},
    {"", 0x21010901u, 0x10101010u, 0x44444444u, 0x80008000u, 0x00010001u},
    {"", 0x49494949u, 0x10081010u, 0x5D5D5D5Du, 0x80800080u, 0x00410041u},
    {"", 0x14919491u, 0x18101810u, 0x53535353u, 0x80008000u, 0x00810081u},
    {"", 0x47474747u, 0x14101410u, 0xFFFFFFFFu, 0x80008000u, 0x01010101u},
}};
