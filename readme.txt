This is a reader for the wikipeia database download. There is no way I'm including the entire database dump in this github repository, so here's the link to get it:
https://en.wikipedia.org/wiki/Wikipedia:Database_download

To use this:
reader -d [database].xml -l [some lookup file]
If the lookup file isn't found, one is created.
The reason for the lookup files is that the database download is 80GB. Parsing through the entire thing and finding all the titles every time would be ridiculous, so a lookup table is made to help out.

The database download is such a massive file that I ran out of disk space. I can no longer test this program, so this is the end until further notice.
