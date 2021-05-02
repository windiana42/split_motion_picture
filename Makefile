split_video: src/split_video.c Makefile
	# gcc -o split_video -Wall -g src/split_video.c
	gcc -o split_video -Wall -O3 src/split_video.c

clean:
	rm -i split_video
