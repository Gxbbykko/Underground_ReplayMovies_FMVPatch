#pragma once

#include <cstddef>

namespace ReplayMoviesCatalog
{
    struct Entry
    {
        const char* madBasename;
        const char* displayTitle;
    };

    // Retail career-movie order, intentionally NOT sorted by filename.
    // Display titles are preserved exactly as supplied/extracted.
    inline constexpr Entry kCareerMovies[] =
    {
        { "01_DAY",   "Go Race!" },
        { "02_FAN_s", "Your Fantasy?" },
        { "02_FAN_e", "Get You Races" },
        { "04_GUM_s", "Get Respect" },
        { "05_ENG",   "Engine of Darkness" },
        { "06_SAM",   "Samantha's Uniques" },
        { "07_GOD",   "Eddie: Race God" },
        { "11_PNK",   "On the street, punk!" },
        { "13_KIT",   "Goodbye, Samantha" },
        { "12_BUD",   "Street Race Buddha" },
        { "09_GBE",   "The Good, the Bad, the Eddie" },
        { "14_MUT",   "Samantha's Cute Car" },
        { "16_RES",   "This is Respect" },
        { "15_DIK",   "Game Over" },
        { "08_EDD",   "Wrath of Eddie" },
        { "17_MET",   "Legend of Metal" },
        { "19_FIN",   "Legend Ascendent" },
    };

    inline constexpr std::size_t kCareerMovieCount =
        sizeof(kCareerMovies) / sizeof(kCareerMovies[0]);
}