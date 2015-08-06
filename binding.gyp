{
  "targets": [
    {
      "target_name": "FileWatcher",
      "sources": [
        "src/FileWatcher.cpp",
        "src/NodeSentinelFileWatcher.cpp",
        "src/FileWatcher32.cpp",
        "includes/FileWatcher.h",
        "includes/NodeSentinelFileWatcher.h",
        "includes/FileWatcher32.h",
        "includes/FileWatcherInterface.h"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "includes"
      ]
    }
  ]
}
