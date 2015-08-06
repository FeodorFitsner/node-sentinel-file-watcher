#ifndef FILEWATCHERINTERFACE_H
#define FILEWATCHERINTERFACE_H

#include <string>
#include <queue>

namespace NSFW {

  struct Event {
    std::string action;
    std::string directory;
    std::string file;
  };

  class FileWatcherInterface {
  public:
    FileWatcherInterface() {}
    FileWatcherInterface(std::string path, std::queue<Event> &eventsQueue) {}
  };
}

#endif
