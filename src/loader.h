#ifndef LOADER_H
#define LOADER_H

#include <string>
#include "surface.h"
#include "gmenu2x.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

using namespace std;

static const string LOADER_MARKER_FILE = "/tmp/gmenunx.marker";
static const string LOADER_FOLDER = "loader";
static const string LOADER_CONFIG_FILE = "loader.conf";

class Loader {
    private:
        GMenu2X *app;
        int interval; //=500;
        string soundFile; //="deffreem.wav";
        int volume; //=50;
        int maxRunLength; //=5500;
        vector<string> images;

        bool fromFile();
        void showLoader();

    public:
        Loader(GMenu2X *app);
        ~Loader();
        void run();
};

#endif
