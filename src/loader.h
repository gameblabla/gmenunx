#ifndef LOADER_H
#define LOADER_H

#include <string>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

#include "constants.h"
#include "surface.h"
#include "esoteric.h"

using namespace std;

static const string LOADER_MARKER_FILE = "/tmp/" + BINARY_NAME + ".has.run";
static const string LOADER_FOLDER = "loader";
static const string LOADER_CONFIG_FILE = "loader.conf";

class Loader {
    private:
        Esoteric *app;
        int interval; //=500;
        string soundFile; //="deffreem.wav";
        int volume; //=50;
        int maxRunLength; //=5500;
        vector<string> images;

        bool fromFile();
        void showLoader();

    public:
        Loader(Esoteric *app);
        ~Loader();
        void run();
        static bool isFirstRun();
        static void setMarker();
};

#endif
