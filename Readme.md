# Split Motion Picture mp4 from Google Pixel generated JPEG

Google Pixel phones store a small video before and after taking a photo within the picture file itself. 
For processing your photos and videos it may be better to have both as separate files.

The python code src/split_video.py shows how it is done.
The C code does it fast.

# Build
This build script assumes a gcc compiler setup and make available on path.
```bash
make
```

# Run
The following command assumes running in a shell that expands *.jpg to individual file names as `bash` does it.
```bash
./split_video *.jpg
```