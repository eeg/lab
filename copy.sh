#!/bin/bash

# first, "cd ../ ; make clean ; make html"
rm -rf _images/ _static/*.* _static/reprints/ *.html
cp -r  ../_build/html/* .
cp     ../_build/html/_static/goldberg_cv.pdf _static/

rm -rf genindex.html objects.inv search.html searchindex.js _sources/ _static/logos.xcf

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
