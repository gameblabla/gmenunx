#ifndef INOTIFIABLE_H_
#define INOTIFIABLE_H_

#include <string>

class INotifiable {
    public:
        virtual static void callback(void * this_pointer, std::string text) = 0;
};

#endif