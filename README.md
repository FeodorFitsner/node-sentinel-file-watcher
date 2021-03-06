# node-sentinel-file-watcher
<table>
  <thead>
    <tr>
      <th>Linux</th>
      <th>OS X</th>
      <th>Windows</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td colspan="2" align="center">
      <a href="https://travis-ci.org/Axosoft/node-sentinel-file-watcher"><img src="https://travis-ci.org/Axosoft/node-sentinel-file-watcher.svg?branch=master"></a>
      </td>
      <td align="center">
        <a href="https://ci.appveyor.com/project/implausible/node-simple-file-watcher"><img src="https://ci.appveyor.com/api/projects/status/79ejlq7e60kjmbl6?svg=true"></a>
      </td>
    </tr>
  </tbody>
</table>
A simple file watcher library for node.

## Usage

```js
var simple = require('node-sentinel-file-watcher');
var watcherOne = new nsfw("dir1", function(events) {
  // handle events
});
var watcherTwo = new nsfw("dir2", function(events) {
  // handles other events
}, 5000); // every 5 seconds

```

## Callback Argument

An array of events as they have happened in a directory, it's children, or to a file.
```js
[
  {
    "action": "CHANGED",
    "directory": "/home/nsfw/watchDir",
    "file": "file1.ext"
  },
  {
    "action": "CREATED",
    "directory": "/home/nsfw/watchDir",
    "file": "folder"
  },
  {
    "action": "DELETED",
    "directory": "home/nsfw/watchDir/testFolder",
    "file": "test.ext"
  },
  {
    "action": "RENAMED",
    "directory": "home/nsfw/watchDir",
    "oldFile": "oldname.ext",
    "newFile": "newname.ext"
  }
]
```
