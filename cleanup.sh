#!/bin/bash
rm -rf genindex.html objects.inv search.html searchindex.js _sources/
echo "<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 3.2//EN'>\n
<html>\n <head> <meta http-equiv='refresh' content='0; url=http://www.tc.umn.edu/~eeg/home.html'> <title> </title> </head> <body> redirecting... </body>\n </html>" > index.html
