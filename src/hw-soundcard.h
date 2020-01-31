#ifndef _ISOUNDCARD_
#define _ISOUNDCARD_

#include <string>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <alsa/asoundlib.h>

#include "debug.h"
#include "constants.h"
#include "utilities.h"

// generic interface
class ISoundcard {
    protected:
        ISoundcard() {};

    public:
        ISoundcard(std::string card, std::string channel) {};
        virtual int getVolume() = 0;
        virtual bool setVolume(int val) = 0;
};

class DummySoundcard : ISoundcard {
    public:
        DummySoundcard(std::string card="blank", std::string channel="none") : ISoundcard() {};
        int getVolume() { return 100; };
        bool setVolume(int val) { return true; };
};

class AlsaSoundcard : ISoundcard {

    private:
        std::string card_;
        std::string channel_;
    protected:

    public:

        AlsaSoundcard(std::string card, std::string channel) : ISoundcard(card, channel) {
            this->card_ = card;
            this->channel_ = channel;
        }

        int getVolume() {
            TRACE("enter");
            int result = -1;
            long min, max;
            snd_mixer_t *handle;
            snd_mixer_selem_id_t *sid;
            const char *card = this->card_.c_str();
            const char *selem_name = this->channel_.c_str();

            int sndFd = snd_mixer_open(&handle, 0);
            if (0 == sndFd) {
                int cardFd = snd_mixer_attach(handle, card);
                if (0 == cardFd) {
                    snd_mixer_selem_register(handle, NULL, NULL);
                    snd_mixer_load(handle);

                    snd_mixer_selem_id_alloca(&sid);
                    snd_mixer_selem_id_set_index(sid, 0);
                    snd_mixer_selem_id_set_name(sid, selem_name);
                    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

                    if (NULL != elem) {
                        //TRACE("we're good to make the read");
                        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                        //TRACE("min : %lu, max : %lu", min, max);
                        long unscaled = -1;
                        snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &unscaled);
                        //TRACE("raw result : %lu", unscaled);
                        result = (int) ((unscaled * 100) / max);
                        //TRACE("scaled result : %i", result);
                    } else {
                        TRACE("couldn't find channel : '%s'", selem_name);
                    }
                } else {
                    TRACE("couldn't attach to card : '%s'", card);
                }
                snd_mixer_close(handle);
            } else {
                TRACE("couldn't open sound mixer");
            }

            TRACE("exit : %i", result);
            return result;
        }

        bool setVolume(int val) {
            bool result = false;
            long min, max;
            snd_mixer_t *handle;
            snd_mixer_selem_id_t *sid;
            const char *card = this->card_.c_str();
            const char *selem_name = this->channel_.c_str();

            int sndFd = snd_mixer_open(&handle, 0);
            if (0 == sndFd) {
                int cardFd = snd_mixer_attach(handle, card);
                if (0 == cardFd) {
                    snd_mixer_selem_register(handle, NULL, NULL);
                    snd_mixer_load(handle);

                    snd_mixer_selem_id_alloca(&sid);
                    snd_mixer_selem_id_set_index(sid, 0);
                    snd_mixer_selem_id_set_name(sid, selem_name);
                    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

                    if (NULL != elem) {
                        //TRACE("we're good to make the write");
                        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                        TRACE("min : %lu, max : %lu", min, max);
                        snd_mixer_selem_set_playback_volume_all(elem, val * max / 100);
                    } else {
                        TRACE("couldn't find channel : '%s'", selem_name);
                    }
                } else {
                    TRACE("couldn't attach to card : '%s'", card);
                }
                snd_mixer_close(handle);
            } else {
                TRACE("couldn't open sound mixer");
            }
            return result;
        }

};

#endif
