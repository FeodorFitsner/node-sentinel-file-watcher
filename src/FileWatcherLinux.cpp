#include "../includes/FileWatcherLinux.h"

namespace NSFW {

  FileWatcherLinux::FileWatcherLinux(std::string path, std::queue<Event> &eventsQueue, bool &watchFiles)
    : mEventsQueue(eventsQueue), mInotify(0), mPath(path), mWatchFiles(watchFiles) {}
  FileWatcherLinux::~FileWatcherLinux() {}

  void FileWatcherLinux::addEvent(std::string action, inotify_event *inEvent) {
    Directory *parent = mWDtoDirNode[inEvent->wd];
    addEvent(action, parent->path + "/" + parent->name, new std::string(inEvent->name));
  }

  void FileWatcherLinux::addEvent(std::string action, std::string directory, std::string *file) {
    Event event;
    event.action = action;
    event.directory = directory;
    event.file = file;
    mEventsQueue.push(event);
  }

  Directory *FileWatcherLinux::buildDirTree(std::string path, bool queueFileEvents = false) {
    std::queue<Directory *> dirQueue;
    Directory *topRoot = new Directory;

    // create root of snapshot
    if (path[path.length() - 1] == '/') {
      path = path.substr(0, path.length() - 1);
    }

    size_t lastSlash = path.find_last_of("/");
    if (lastSlash != std::string::npos) {
      topRoot->name = path.substr(lastSlash + 1);
      topRoot->path = path.substr(0, lastSlash);
    } else {
      topRoot->name = "";
      topRoot->path = "/";
    }

    topRoot->watchDescriptor = -1;

    dirQueue.push(topRoot);
    bool checkRootOnExit = false;

    while (!dirQueue.empty()) {
      Directory *root = dirQueue.front();
      dirent ** directoryContents = NULL;
      std::string fullPath = root->path + "/" + root->name;

      root->watchDescriptor = inotify_add_watch(
        mInotify,
        fullPath.c_str(),
        IN_ATTRIB | IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO
      );

      int n = scandir(fullPath.c_str(), &directoryContents, NULL, alphasort);

      if (n < 0 || root->watchDescriptor < 0) {
        if (topRoot == root)
        {
          return NULL; // top directory no longer exists
        }
        else {
          checkRootOnExit = true;
          dirQueue.pop();
          continue;
        }
      }

      mWDtoDirNode[root->watchDescriptor] = root;

      // find all the directories within this directory
      // this breaks the alphabetical sorting of directories
      for (int i = 0; i < n; ++i) {
        if (!strcmp(directoryContents[i]->d_name, ".") || !strcmp(directoryContents[i]->d_name, "..")) {
          continue; // skip navigation folder
        }

        // Certain *nix do not support dirent->d_type and may return DT_UNKOWN for every file returned in scandir
        // in order to make this work on all *nix, we need to stat the file to determine if it is a directory
        std::string filePath = root->path + "/" + root->name + "/" + directoryContents[i]->d_name;

        struct stat file;

        if (stat(filePath.c_str(), &file) < 0) {
          continue;
        }

        if (S_ISDIR(file.st_mode))
        {
          // create the directory struct for this directory and add a reference of this directory to its root
          Directory *dir = new Directory;
          dir->path = root->path + "/" + root->name;
          dir->name = directoryContents[i]->d_name;
          dir->watchDescriptor = -1;
          root->childDirectories[dir->name] = dir;
          dirQueue.push(dir);
        } else {
          root->files.insert(directoryContents[i]->d_name);
        }

        if (queueFileEvents) {
          addEvent("CREATED", root->path + "/" + root->name, new std::string(directoryContents[i]->d_name));
        }
      }

      for (int i = 0; i < n; ++i) {
        delete directoryContents[i];
      }

      delete[] directoryContents;

      dirQueue.pop();
    }

    if (checkRootOnExit) {
      struct stat rootStat;
      if (!stat((topRoot->path + "/" + topRoot->name).c_str(), &rootStat) || !S_ISDIR(rootStat.st_mode)) {
        // delete tree as far as we can go
        destroyWatchTree(topRoot);
        return NULL;
      }
    }

    return topRoot;
  }

  void FileWatcherLinux::destroyWatchTree(Directory *tree) {
    std::queue<Directory *> dirQueue;
    dirQueue.push(tree);

    if (fcntl(mInotify, F_GETFD) != -1 || errno != EBADF) {
      // need to pass errors back here so that the next call to poll
      // can clean up after this type of error
      return; // panic
    }

    while (!dirQueue.empty()) {
      Directory *root = dirQueue.front();

      inotify_rm_watch(mInotify, root->watchDescriptor);

      // Add directories to the queue to continue listing events
      for (std::map<std::string, Directory *>::iterator dirIter = root->childDirectories.begin();
        dirIter != root->childDirectories.end(); ++dirIter)
      {
        dirQueue.push(dirIter->second);
      }

      dirQueue.pop();

      delete root;
    }
  }

  std::string FileWatcherLinux::getPath() {
    return mPath;
  }

  void *FileWatcherLinux::mainLoop(void *params) {
    FileWatcherLinux *fwLinux = (FileWatcherLinux *)params;

    // build the directory tree before listening for events
    Directory *dirTree = fwLinux->buildDirTree(fwLinux->getPath());

    // check that the directory can be watched before trying to watch it
    if (dirTree == NULL) {
      // throw error if the directory didn't exists
      return NULL;
    }

    fwLinux->setDirTree(dirTree);
    fwLinux->processEvents();

    return NULL;
  }


  void FileWatcherLinux::processEvents() {
    size_t count = sizeof(struct inotify_event) + NAME_MAX + 1;
    char *buffer = new char[1024*count];
    int watchDescriptor = -1;
    unsigned int bytesRead, position = 0, cookie = 0;
    Event lastMovedFromEvent;

    while(mWatchFiles && (bytesRead = read(mInotify, buffer, count)) > 0) {
      inotify_event *inEvent;
      do {
        inEvent = (inotify_event *)(buffer + position);
        Event event;

        // if the event is not a moved to event and the cookie exists
        // we should reset the cookie and push the last moved from event
        if (cookie != 0 && inEvent->mask != IN_MOVED_TO) {
          mEventsQueue.push(lastMovedFromEvent);
          cookie = 0;
          watchDescriptor = -1;
        }
        bool isDir = inEvent->mask & IN_ISDIR;
        inEvent->mask = isDir ? inEvent->mask ^ IN_ISDIR : inEvent->mask;

        switch(inEvent->mask) {
          case IN_ATTRIB:
          case IN_MODIFY:
            if (*inEvent->name > 31) // ignore control characters
              addEvent("CHANGED", inEvent);
            break;
          case IN_CREATE:
          {
            Directory *parent = mWDtoDirNode[inEvent->wd];
            // check stats on the item CREATED
            // if it is a dir, create a watch for all of its directories
            if (isDir && parent->childDirectories.find(inEvent->name) == parent->childDirectories.end()) {

              std::string newPath = parent->path + "/" + parent->name + "/" + inEvent->name;
              // add the directory tree
              Directory *child = buildDirTree(newPath, true);

              if (child == NULL)
                break;

              parent->childDirectories[child->name] = child;
              addEvent("CREATED", inEvent);
              break;
            }

            if (!isDir && parent->files.find(inEvent->name) == parent->files.end()) {
              parent->files.insert(inEvent->name);
              addEvent("CREATED", inEvent);
            }
            break;
          }
          case IN_DELETE:
          {
            Directory *parent = mWDtoDirNode[inEvent->wd];
            if (isDir) {
              if (parent->childDirectories.find(inEvent->name) == parent->childDirectories.end()) {
                continue;
              }
              Directory *child = parent->childDirectories[inEvent->name];
              parent->childDirectories.erase(child->name);
              destroyWatchTree(child);
              child = NULL;
            } else {
              parent->files.erase(inEvent->name);
            }

            addEvent("DELETED", inEvent);
            break;
          }
          case IN_MOVED_FROM:
            fd_set checkWD;
            FD_ZERO(&checkWD);
            FD_SET(mInotify, &checkWD);
            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 250000;

            if (!isDir) {
              mWDtoDirNode[inEvent->wd]->files.erase(inEvent->name);
            }

            if (position + sizeof(struct inotify_event) + inEvent->len < bytesRead || select(mInotify+1, &checkWD, 0, 0, &timeout) > 0) {
              lastMovedFromEvent.action = "DELETED";
              lastMovedFromEvent.directory = mWDtoDirNode[inEvent->wd]->path;
              lastMovedFromEvent.file = new std::string(inEvent->name);
              cookie = inEvent->cookie;
              watchDescriptor = inEvent->wd;
            } else {
              addEvent("DELETED", inEvent);
            }
            break;
          case IN_MOVED_TO:
            if (!isDir) {
              mWDtoDirNode[inEvent->wd]->files.insert(inEvent->name);
            }
            // check if this is a move event
            if (cookie != 0 && inEvent->cookie == cookie && inEvent->wd == watchDescriptor) {
              cookie = 0;
              watchDescriptor = -1;
              event.action = "RENAMED";
              event.directory = mWDtoDirNode[inEvent->wd]->path;
              event.file = new std::string[2];
              event.file[0] = *lastMovedFromEvent.file;
              event.file[1] = inEvent->name;
              delete lastMovedFromEvent.file;
              mEventsQueue.push(event);
            } else {
              addEvent("CREATED", inEvent);
            }
            break;
        }
      } while ((position += sizeof(struct inotify_event) + inEvent->len) < bytesRead);
      position = 0;
    }
  }

  bool FileWatcherLinux::start() {
    mInotify = inotify_init();
    if (mInotify < 0) {
      return false;
    }

    if (mWatchFiles && pthread_create(&mThread, 0, &FileWatcherLinux::mainLoop, (void *)this)) {
      return true;
    } else {
      return false;
    }
  }

  void FileWatcherLinux::stop() {
    int t;
    pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, &t);
    pthread_cancel(mThread);
    destroyWatchTree(mDirTree);
  }

  void FileWatcherLinux::setDirTree(Directory *tree) {
    mDirTree = tree;
  }

}
