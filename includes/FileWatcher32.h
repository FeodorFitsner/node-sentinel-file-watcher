#ifndef FILEWATCHER32_H
#define FILEWATCHER32_H

#include "FileWatcherInterface.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>

namespace NSFW {

  class FileWatcher32 : public FileWatcherInterface {
  public:
    FileWatcher32(std::string path, std::queue<Event> &eventsQueue);
    ~FileWatcher32();
    static DWORD WINAPI mainLoop(LPVOID lpParam);
    void watchDirectory();
  private:
    std::queue<Event> &mEventsQueue;
    std::string mPath;
  };

}

#endif
