usage of functions like:
mkfifo open read write close
fork - 用了fork,只需要pipe_write就可以实现fifo通讯，当主进程结束后，fork出来的子进程，会变成孤儿进程。
下次再次调用pipe_write的时候，会先open fifo联系孤儿进程，以达成通讯的目的。
