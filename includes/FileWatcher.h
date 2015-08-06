#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <string>
#include <queue>
#include "FileWatcherInterface.h"

namespace NSFW {

  class FileWatcher {
  public:
    // Constructor
    FileWatcher(std::string path);
    ~FileWatcher();

    // Public methods
    void pushEvent(Event event);
    std::queue<Event> *pollEvents();

  private:
    std::queue<Event> mEventsQueue;
    FileWatcherInterface *fwInterface;
  };
}

#endif
