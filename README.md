# Instructions

1. Get the Wikipedia database at
https://meta.wikimedia.org/wiki/Data_dump_torrents#English_Wikipedia

Unless you have 100 GB disk space available, try Simple English Wikipedia.

2. Run the following to decompress the file.

```
bzip2 -d [the compressed db file you downloaded on step #2]
```

This takes a long time, potentially more several. If you'd like to see the progress, you can run something like:

```
while : ; do wc -c [name of xml file being decompressed] ; sleep 1 ; done
```

3. To compile, simply run the "make" command. This program should (!) run on all
UNIX systems with the less command and ncurses.

4. To run it, just type

```
wikireader [wikipedia database download] [index]
```

The index file can be created in Wikireader, if you don't have one already, just put in a file name that you'd like an index file to go to.

**NOTE: This is not the same index file as can be downloaded with the database download, this is a custom format.**
