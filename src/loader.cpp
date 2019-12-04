#include <vector>
#include "loader.h"
#include "surface.h"
#include "utilities.h"
#include <string.h>
#include <fstream>

#include "messagebox.h"
#include "debug.h"

using namespace std;

Loader::Loader(Esoteric *app) {
    this->app = app;
    this->interval=1000;
    this->volume = 50;
    this->maxRunLength = 1000;
    this->soundFile = "";
    this->images.clear();
}

Loader::~Loader() {
    TRACE("~Loader");
}

bool Loader::fromFile() {
    string loaderPath = this->app->skin->currentSkinPath() + "/" + LOADER_FOLDER;
    string confFile = loaderPath + "/" + LOADER_CONFIG_FILE;
    
    TRACE("enter : %s", confFile.c_str());
    if (fileExists(confFile)) {
        TRACE("config exists");
        string tempImages;
		ifstream loaderConf(confFile.c_str(), std::ios_base::in);
		if (loaderConf.is_open()) {
			string line;
			while (getline(loaderConf, line, '\n')) {
				line = trim(line);
                if (0 == line.length()) continue;
                if ('#' == line[0]) continue;
				string::size_type pos = line.find("=");
                if (string::npos == pos) continue;
                
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

                if (0 == value.length()) continue;
                TRACE("key : value - %s : %s", name.c_str(), value.c_str());

                if (name == "interval") {
                    this->interval = atoi(value.c_str());
                } else if (name == "sound") {
                    if (value.at(0) == '"' && value.at(value.length() - 1) == '"') {
                        this->soundFile = value.substr(1, value.length() - 2);
                    } else this->soundFile = value;
                } else if (name == "images") {
                    // handle quotes
                    if (value.at(0) == '"' && value.at(value.length() - 1) == '"') {
                        tempImages = value.substr(1, value.length() - 2);
                    } else tempImages = value;
                } else if (name == "maxRunLength") {
                    this->maxRunLength = atoi(value.c_str());
                } else if (name == "volume") {
                    this->volume = atoi(value.c_str());
                }

            };
            loaderConf.close();
        }

        if (!tempImages.empty()) {
            TRACE("found images : %s", tempImages.c_str());
            vector<string> temp;
            split(temp, tempImages, ",");
            // load images into sc
            for(std::vector<string>::iterator it = temp.begin(); it != temp.end(); ++it) {
                string name = *it;
                string imagePath = loaderPath + "/" + name;
                TRACE("checking image exists : %s", imagePath.c_str());
                if (fileExists(imagePath)) {
                    TRACE("image exists");
                    this->app->sc->addIcon(imagePath);
                    this->images.push_back(imagePath);
                }
            }
        }
        TRACE("checking sound file");
        if (!this->soundFile.empty()) {
            string tempSoundFile = loaderPath + "/" + this->soundFile;
            TRACE("checking sound file : %s", tempSoundFile.c_str());
            if (fileExists(tempSoundFile)) {
                TRACE("found sound file at : %s", this->soundFile.c_str());
                this->soundFile = tempSoundFile;
            } else {
                this->soundFile = "";
            }
        }
        return !this->images.empty();
    }
    return false;
}

void Loader::run() {
    TRACE("enter, looking for marker : %s", LOADER_MARKER_FILE.c_str());
    if (!fileExists(LOADER_MARKER_FILE)) {
        TRACE("no marker file : %s", LOADER_MARKER_FILE.c_str());
        if (this->fromFile()) {
            this->showLoader();
        }
        TRACE("setting marker : %s", LOADER_MARKER_FILE.c_str());
        fstream fs;
        fs.open(LOADER_MARKER_FILE, ios::out);
        fs.close();
    } else {
        TRACE("not first boot, loader marker exists : %s", LOADER_MARKER_FILE.c_str());
    }
    TRACE("exit");
}

void Loader::showLoader() {
    TRACE("enter");
    //-----------------------
    // audio first
    bool playSound = false;
    Mix_Chunk *logosound;
    if (this->soundFile.size()) {
        Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024);
        Mix_AllocateChannels(2);
        Mix_Volume(-1, this->volume);
        logosound = Mix_LoadWAV(this->soundFile.c_str());
        Mix_PlayChannel(-1, logosound, 0);
        playSound = true;
    }

    uint32_t curr_time = SDL_GetTicks();
    uint32_t old_time = curr_time;
    uint32_t base_time = curr_time;

    Surface * currImg;
    for (int i = 0; i < this->images.size(); i++) {
        currImg = (*this->app->sc)[this->images.at(i)];

        this->app->screen->box(
            (SDL_Rect){ 0, 0, this->app->screen->raw->w, this->app->screen->raw->h }, 
            (RGBAColor){ 0, 0, 0, 255 }
        );

        currImg->blit(this->app->screen, 0, 0);
        this->app->screen->flip();

        // chew the time
        while (curr_time < old_time + this->interval) {
            curr_time = SDL_GetTicks();
        }
        old_time = curr_time;
        // obey max length if set
        if ((curr_time - base_time) > this->maxRunLength && this->maxRunLength > 0) {
            break;
        }

        // and any key to quit
        this->app->input.update(false);
        if (this->app->input[CONFIRM] || this->app->input[CANCEL]) {
            playSound = false;
            break;
        }
    }

    // hang onto the rest of the sound if max length is set
    int delayLeft = this->maxRunLength - (this->images.size() * this->interval);
    if (delayLeft > 0 && playSound) {

        // and fade out
        uint8_t alpha = 50;//255;
        int delta = 0;
        int fadeTime = 16; // 60 fps
        int fadeStep = 255 * (fadeTime / delayLeft);
        curr_time = SDL_GetTicks();
        old_time = curr_time;
        while (delayLeft > 0) {
            this->app->screen->box(
                (SDL_Rect){ 0, 0, this->app->screen->raw->w, this->app->screen->raw->h }, 
                (RGBAColor){ 0, 0, 0, 255 }
            );
            currImg->blit(this->app->screen, 0, 0, 0, alpha);
            this->app->screen->flip();

            while (curr_time < old_time + fadeTime) {
                curr_time = SDL_GetTicks();
            }
            old_time = curr_time;
                
            delayLeft -= fadeTime;
            alpha -= fadeStep;
            if (alpha < 0) alpha = 0;

            // and any key to quit
            this->app->input.update(false);
            if (this->app->input[CONFIRM] || this->app->input[CANCEL]) {
                break;
            }
        }
    }

    // ------------------------
    delete currImg;

    if (this->soundFile.size()) {
        Mix_FreeChunk(logosound);
        Mix_CloseAudio();
        SDL_CloseAudio();
    }

    TRACE("exit");
}

