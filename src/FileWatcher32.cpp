#include "../includes/FileWatcher32.h"

namespace NSFW {

  FileWatcher32::FileWatcher32(std::string path, std::queue<Event> &eventsQueue)
  : mEventsQueue(eventsQueue), mPath(path) {
    DWORD dwThreadId;
    CreateThread(
      NULL,
      0,
      FileWatcher32::mainLoop,
      this,
      0,
      &dwThreadId
    );
  }

  FileWatcher32::~FileWatcher32() {}

  DWORD WINAPI FileWatcher32::mainLoop(LPVOID lpParam) {
    FileWatcher32 *fw32 = (FileWatcher32 *)lpParam;
    fw32->watchDirectory();
    return 0;
  }

  void FileWatcher32::watchDirectory() {
    DWORD dwWaitStatus;
    HANDLE dwChangeHandles[2];
    TCHAR lpDrive[4];
    TCHAR lpFile[_MAX_FNAME];
    TCHAR lpExt[_MAX_EXT];

    LPSTR lpDir = const_cast<char *>(mPath.c_str());

    _tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

    lpDrive[2] = (TCHAR)'\\';
    lpDrive[3] = (TCHAR)'\0';

    // Watch the directory for file creation and deletion.

    dwChangeHandles[0] = FindFirstChangeNotification(
       lpDir,                         // directory to watch
       TRUE,                         // do not watch subtree
       FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_DIR_NAME); // watch file name changes

    if (dwChangeHandles[0] == INVALID_HANDLE_VALUE)
    {
      printf("\n ERROR: FindFirstChangeNotification function failed.\n");
      return;
    }

    // Watch the subtree for directory creation and deletion.

    dwChangeHandles[1] = FindFirstChangeNotification(
       lpDrive,                       // directory to watch
       TRUE,                          // watch the subtree
       FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_DIR_NAME);  // watch dir name changes

    if (dwChangeHandles[1] == INVALID_HANDLE_VALUE)
    {
      printf("\n ERROR: FindFirstChangeNotification function failed.\n");
      return;
    }


    // Make a final validation check on our handles.

    if ((dwChangeHandles[0] == NULL) || (dwChangeHandles[1] == NULL))
    {
      printf("\n ERROR: Unexpected NULL from FindFirstChangeNotification.\n");
      return;
    }

    // Change notification is set. Now wait on both notification
    // handles and refresh accordingly.

    while (TRUE)
    {
    // Wait for notification.

       printf("\nWaiting for notification...\n");

       dwWaitStatus = WaitForMultipleObjects(2, dwChangeHandles,
          FALSE, INFINITE);

      FILE_NOTIFY_INFORMATION strFileNotifyInfo[1024];
      DWORD dwBytesReturned = 0;
       switch (dwWaitStatus)
       {
          case WAIT_OBJECT_0:
          // A file was created, renamed, or deleted in the directory.
          // Refresh this directory and restart the notification.
          if( ReadDirectoryChangesW ( dwChangeHandles[0], (LPVOID)&strFileNotifyInfo, sizeof(strFileNotifyInfo), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION, &dwBytesReturned, NULL, NULL) == 0)
          {
              printf("\n ERROR: Reading Directory Change \n");
          }
          else
          {
            std::cout << "File Modified: "; std::wcout << strFileNotifyInfo[0].FileName << std::endl;
          }
              std::cout << "Change in the directory" << std::endl;
              if ( FindNextChangeNotification(dwChangeHandles[0]) == FALSE )
              {
                printf("\n ERROR: FindNextChangeNotification function failed.\n");
                return;
              }
              break;

          case WAIT_OBJECT_0 + 1:

          // A directory was created, renamed, or deleted.
          // Refresh the tree and restart the notification.
          // if( ReadDirectoryChangesW ( dwChangeHandles[1], (LPVOID)&strFileNotifyInfo, sizeof(strFileNotifyInfo), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION, &dwBytesReturned, NULL, NULL) == 0)
          // {
          //     printf("\n ERROR: Reading Directory Change \n");
          // }
          // else
          // {
          //     std::cout << "File Modified: "; std::wcout << strFileNotifyInfo[0].FileName << std::endl;
          // }
          //     std::cout << "Change in the tree" << std::endl;
          //     if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE )
          //     {
          //       printf("\n ERROR: FindNextChangeNotification function failed.\n");
          //       return;
          //     }
              break;

          case WAIT_TIMEOUT:

          // A timeout occurred, this would happen if some value other
          // than INFINITE is used in the Wait call and no changes occur.
          // In a single-threaded environment you might not want an
          // INFINITE wait.

             printf("\nNo changes in the timeout period.\n");
             break;

          default:
             printf("\n ERROR: Unhandled dwWaitStatus.\n");
             break;
       }
    }
  }

}
