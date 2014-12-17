#!/bin/bash

# first, "cd ../ ; make html"
rm -rf _*/ *.html
cp -r ../_build/html/* .

rm -rf genindex.html objects.inv search.html searchindex.js _sources/

echo "<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 3.2//EN'>
<html>
  <head>
    <meta http-equiv='refresh' content='0; url=home.html'>
    <title> </title>
  </head>
  <body>
    redirecting...
  </body>
</html>" > index.html
