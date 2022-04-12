#ifndef APP_PINBALL_BEEPER_H
#define APP_PINBALL_BEEPER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>
#include <etl/span.h>

#include "Rtos/Rtos.h"

namespace App::Pinball {
/**
 * @brief Exposes a high level interface to beeper
 *
 * Provides helper methods for playing "melodies," which are lists of notes (which in turn are
 * frequency, duration, and volume tuples) that are automatically timed and output on the beeper
 * on the board.
 */
class Beeper {
    public:
        /**
         * @brief A single note in a melody
         *
         * Notes are defined by a frequency, relative amplitude, and delay until the next note.
         */
        struct Note {
            /**
             * @brief Note frequency (in Hz)
             *
             * If zero, the frequency is not changed.
             */
            uint16_t frequency{0};
            /**
             * @brief Relative amplitude
             *
             * The relative loudness of the note, where 0 is silent, and 0xFF is full volume.
             */
            uint8_t amplitude;

            /**
             * @brief Duration (msec)
             *
             * How long is this note sustained?
             */
            uint16_t duration;
        };

    public:
        /// "Invalid button input" beep
        constexpr static const etl::array<const Note, 3> kInvalidButtonMelody{{
            { 1400,     0x80, 33 },
            { 0,        0x00, 33 },
            { 1200,     0x80, 33 },
        }};

    public:
        static void Init();
        static void Process();

        static void Play(etl::span<const Note> melody);

    private:
        static void PlayNextNote();

    private:
        /// Are we currently playing a melody?
        static bool gIsActive;
        /// Absolute (maximum) volume for melodies
        static float gVolume;

        /// Currently playing melody
        static etl::span<const Note> gCurrentMelody;
        /// Offset into the current melody
        static size_t gMelodyOffset;

        /// Timer used to drive melodies
        static TimerHandle_t gTimer;
};
}

#endif
