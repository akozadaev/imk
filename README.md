IMK
============
Simple file watcher similar to fswatch or inotify-wait.


Usage:
------
```bash
$ ./imk -h
usage: ./imk [-h] [-v] -c <command> [-t <sec>] [-o] <file ...>

   The options are as follows:
      -h          - display this text and exit
      -v          - display the version
      -c <cmd>    - command to execute when event is triggered
      -o          - exit after first iteration
      -t <sec>    - number of seconds to skip after the last executed command (default 0)
      -s <ms>     - number of milliseconds to sleep before reattaching in case of DELETE event (default 300)
      <file ...>  - list of files to monitor

   Please use quotes around the command if it is composed of multiple words
```

To monitor all .c files and run make run the following:

```bash
$ ./imk -c 'make release' *.c
:: [20:00:00] start monitoring: cmd[make release] files[log.c main.c]
```

If any of the monitored files are modified, the command will be executed.

Known Issues:
-------------
 - when monitoring a directory, the command may be triggered more then once for every change of the contained files.
 - imk may not detect a change if the monitored file was edited with VIM with swapfiles enabled.
