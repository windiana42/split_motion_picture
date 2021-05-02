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

# Installation
There is no install section in the Makefile. I did not want to pull of full autoconf/automake machinery for this little project.
Simply copy the generated ./split_video executable somehwere on the path (i.e. `~/bin/`).

# Run

The following command assumes running in a shell that expands *.jpg to individual file names as `bash` does it.
```bash
./split_video *.jpg
```

You can also put generated files in a different directory:
```bash
./split_video *.jpg out/
```

I will use this by first moving .MP.jpg files to a directory marked for deletion 
and then run this script to copy parts where the photos are kept initially:
```bash
cd <my photo & video directory>
mkdir to_delete/
mv *.MP.jpg to_delete/
split_video to_delete/*.jpg ./
```
