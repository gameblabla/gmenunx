#ifndef DESKTOPFILE_H_
#define DESKTOPFILE_H_

#include <string>
#include <istream>

using namespace std;

class DesktopFile {
    private:

        void reset();
        void parse(std::istream & instream);

        bool isDirty_;
        string path_; 

        string title_; //=ReGBA
        string description_; //=Game Boy Advance Emulator
        string icon_; //=/home/mat/.esoteric/skins/Default/icons/regba-silver.png
        string exec_; //=/bin/bash
        string params_;  //=-m default.gcw0.desktop /home/mat/Downloads/games/rg\-350/regba_fast\ \(1\).opk 
        string selectordir_; //=/home/mat/Downloads
        string selectorfilter_; //=.opk
        string provider_; //=/home/mat/Downloads/games/rg\-350/regba_fast\ \(1\).opk
        string providerMetadata_; //=default.gcw0.desktop
        bool consoleapp_; //=false

    public:
        DesktopFile();
        DesktopFile(const string & file);
        ~DesktopFile();

        bool fromFile(const string & file);
        bool save(const string & path = "");
        DesktopFile * clone();
        void remove();

        string toString();

        bool isDirty() { return this->isDirty_; }

        string path() const { return this->path_; }
        void path(string val) { 
            if (val != this->path_) {
                this->path_ = val;
                this->isDirty_ = true;
            }
        }
        string title() const { return this->title_; }
        void title(string val) { 
            if (val != this->title_) {
                this->title_ = val;
                this->isDirty_ = true;
            }
        }
        string description() const { return this->description_; }
        void description(string val) { 
            if (val != this->description_) {
                this->description_ = val;
                this->isDirty_ = true;
            }
        }
        string icon() const { return this->icon_; }
        void icon(string val) { 
            if (val != this->icon_) {
                this->icon_ = val;
                this->isDirty_ = true;
            }
        }
        string exec() const { return this->exec_; }
        void exec(string val) { 
            if (val != this->exec_) {
                this->exec_ = val;
                this->isDirty_ = true;
            }
        }
        string params() const { return this->params_; }
        void params(string val) { 
            if (val != this->params_) {
                this->params_ = val;
                this->isDirty_ = true;
            }
        }
        string selectordir() const { return this->selectordir_; }
        void selectordir(string val) { 
            if (val != this->selectordir_) {
                this->selectordir_ = val;
                this->isDirty_ = true;
            }
        }
        string selectorfilter() const { return this->selectorfilter_; }
        void selectorfilter(string val) { 
            if (val != this->selectorfilter_) {
                this->selectorfilter_ = val;
                this->isDirty_ = true;
            }
        }
        string provider() const { return this->provider_; }
        void provider(string val) { 
            if (val != this->provider_) {
                this->provider_ = val;
                this->isDirty_ = true;
            }
        }
        string providerMetadata() const { return this->providerMetadata_; }
        void providerMetadata(string val) { 
            if (val != this->providerMetadata_ && !val.empty()) {
                this->providerMetadata_ = val;
                this->isDirty_ = true;
            }
        }
        bool consoleapp() const { return this->consoleapp_; }
        void consoleapp(bool val) { 
            if (val != this->consoleapp_) {
                this->consoleapp_ = val;
                this->isDirty_ = true;
            }
        }
};

#endif

