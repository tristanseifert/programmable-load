#include "Beeper.h"
#include "Hardware.h"
#include "Task.h"

#include "Drivers/TimerCounter.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

using namespace App::Pinball;

bool Beeper::gIsActive{false};
float Beeper::gVolume{0.15f};
etl::span<const Beeper::Note> Beeper::gCurrentMelody;
size_t Beeper::gMelodyOffset{0};
TimerHandle_t Beeper::gTimer{nullptr};

/**
 * @brief Initialize the shared beeper instance.
 *
 * This creates the timer used to synchronize melodies.
 */
void Beeper::Init() {
    static StaticTimer_t gBuf;

    // this timer forces processing of beeper
    gTimer = xTimerCreateStatic("beeper timer",
        pdMS_TO_TICKS(10), pdFALSE,
        nullptr, [](auto timer) {
            Task::NotifyTask(Task::TaskNotifyBits::ProcessMelody);
        }, &gBuf);
    REQUIRE(gTimer, "pinball: %s", "failed to allocate timer");
}

/**
 * @brief Update beeper state
 *
 * This is invoked periodically by the UI task (in request to a task notification) and will play
 * a note and arm the beeper timer.
 */
void Beeper::Process() {
    if(gIsActive) {
        PlayNextNote();
    }
    // ensure to silence the beeper
    else {
        Hw::gBeeperTc->setDutyCycle(0, 0.f);
    }
}

/**
 * @brief Begin playing the specified melody
 *
 * Set up the beeper to play the melody. Playback will begin next time the UI task gains control.
 *
 * @param melody List of note events to play
 */
void Beeper::Play(etl::span<const Note> melody) {
    // stop timer if we're already playing something
    xTimerStop(gTimer, 0);

    // reset state
    gIsActive = true;
    gCurrentMelody = melody;
    gMelodyOffset = 0;

    // request to update the next note
    Task::NotifyTask(Task::TaskNotifyBits::ProcessMelody);
}

/**
 * @brief Set up for the next note in the melody.
 *
 * This reads the next note from the melody, and configures the timer/counter duty cycle and
 * frequency values appropriately. If there is another note after this one, we'll re-arm the note
 * timer 
 */
void Beeper::PlayNextNote() {
    BaseType_t ok;

    // if we're at the end, stop the beeper and turn off timer
    if(gMelodyOffset == gCurrentMelody.size()) {
        Hw::gBeeperTc->setDutyCycle(0, 0.f);

        gIsActive = false;
        gMelodyOffset = 0;
        return;
    }

    // update the frequency output
    const auto &note = gCurrentMelody[gMelodyOffset++];

    if(note.frequency) {
        Hw::gBeeperTc->setFrequency(note.frequency);
    }

    Hw::gBeeperTc->setDutyCycle(0, gVolume * (static_cast<float>(note.amplitude) / 255.f));

    // re-arm timer
    ok = xTimerChangePeriod(gTimer, pdMS_TO_TICKS(note.duration), 0);
    REQUIRE(ok == pdPASS, "pinball: %s", "failed to re-arm note timer");
}
