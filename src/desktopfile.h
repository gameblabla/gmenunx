#ifndef DESKTOPFILE_H_
#define DESKTOPFILE_H_

#include <string>
#include <istream>

class DesktopFile {
    private:

        void reset();
        void parse(std::istream & instream);

        bool isDirty_;
        std::string path_; 

        std::string title_; //=ReGBA
        std::string description_; //=Game Boy Advance Emulator
        std::string icon_; //=/home/mat/.esoteric/skins/Default/icons/regba-silver.png
        std::string exec_; //=/bin/bash
        std::string params_;  //=-m default.gcw0.desktop /home/mat/Downloads/games/rg\-350/regba_fast\ \(1\).opk 
        std::string selectorDir_; //=/home/mat/Downloads
        std::string selectorFilter_; //=.opk
        std::string selectorAlias_;
        std::string provider_; //=/home/mat/Downloads/games/rg\-350/regba_fast\ \(1\).opk
        std::string providerMetadata_; //=default.gcw0.desktop
        std::string manual_;
        std::string workdir_;
        bool consoleapp_; //=false

    public:
        DesktopFile();
        DesktopFile(const std::string & file);
        ~DesktopFile();

        bool fromFile(const std::string & file);
        bool save(const std::string & path = "");
        DesktopFile * clone();
        void remove();

        std::string toString();

        bool isDirty() { return this->isDirty_; }

        std::string path() const { return this->path_; }
        void path(std::string val) { 
            if (val != this->path_) {
                this->path_ = val;
                this->isDirty_ = true;
            }
        }
        std::string title() const { return this->title_; }
        void title(std::string val) { 
            if (val != this->title_) {
                this->title_ = val;
                this->isDirty_ = true;
            }
        }
        std::string description() const { return this->description_; }
        void description(std::string val) { 
            if (val != this->description_) {
                this->description_ = val;
                this->isDirty_ = true;
            }
        }
        std::string icon() const { return this->icon_; }
        void icon(std::string val) { 
            if (val != this->icon_) {
                this->icon_ = val;
                this->isDirty_ = true;
            }
        }
        std::string exec() const { return this->exec_; }
        void exec(std::string val) { 
            if (val != this->exec_) {
                this->exec_ = val;
                this->isDirty_ = true;
            }
        }
        std::string params() const { return this->params_; }
        void params(std::string val) { 
            if (val != this->params_) {
                this->params_ = val;
                this->isDirty_ = true;
            }
        }
        std::string selectorDir() const { return this->selectorDir_; }
        void selectorDir(std::string val) { 
            if (val != this->selectorDir_) {
                this->selectorDir_ = val;
                this->isDirty_ = true;
            }
        }
        std::string selectorFilter() const { return this->selectorFilter_; }
        void selectorFilter(std::string val) { 
            if (val != this->selectorFilter_) {
                this->selectorFilter_ = val;
                this->isDirty_ = true;
            }
        }
        std::string selectorAlias() const { return this->selectorAlias_; }
        void selectorAlias(std::string val) { 
            if (val != this->selectorAlias_) {
                this->selectorAlias_ = val;
                this->isDirty_ = true;
            }
        }
        std::string provider() const { return this->provider_; }
        void provider(std::string val) { 
            if (val != this->provider_) {
                this->provider_ = val;
                this->isDirty_ = true;
            }
        }
        std::string providerMetadata() const { return this->providerMetadata_; }
        void providerMetadata(std::string val) { 
            if (val != this->providerMetadata_ && !val.empty()) {
                this->providerMetadata_ = val;
                this->isDirty_ = true;
            }
        }
        std::string manual() const { return this->manual_; }
        void manual(std::string val) { 
            if (val != this->manual_ && !val.empty()) {
                this->manual_ = val;
                this->isDirty_ = true;
            }
        }
        std::string workdir() const { return this->workdir_; }
        void workdir(std::string val) { 
            if (val != this->workdir_ && !val.empty()) {
                this->workdir_ = val;
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

