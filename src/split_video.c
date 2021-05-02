#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

// I am using fgetc() for simplicity here; C++ stream operation might be faster due to buffering

long get_jpg_end(FILE *fh) {
    int next_segment_type = -1;
    while(fgetc(fh) == 0xff) {
        unsigned char segment_type;
        if(next_segment_type >= 0) {
            segment_type = (unsigned char)next_segment_type;
        } else {
            segment_type = (unsigned char)fgetc(fh);
        }
//        printf("Found segment: %x\n", segment_type);
        switch(segment_type) {
            case 0xd8:  // start segment
                // expect next 0xff directly after this
                break;
            case 0xd9:  // end segment
//                printf("Found jpeg end: %x\n", segment_type);
                return ftell(fh);
            case 0xda:  // scan compressed stream
            {
//                printf("Found compressed stream: %x\n", segment_type);
                do {
                    while(fgetc(fh) != 0xff) {
                        if(feof(fh)) {
                            fprintf(stderr, "Failed detecting end of jpeg stream\n");
                            return -1;
                        }
                    }
                    next_segment_type = fgetc(fh);
//                    printf("potential next segment: %x\n", next_segment_type);
                } while(next_segment_type == 0 || next_segment_type == 0xff ||
                    (next_segment_type >= 0xd0 && next_segment_type <= 0xd7));
//                printf("next segment will be: %x\n", next_segment_type);
                ungetc(0xff, fh);
                break;
            }
            default:  // expect variable length segment with given length
            {
                long len1 = fgetc(fh);
                long len2 = fgetc(fh);
//                printf("Found variable length segment: %x: %x %x\n", segment_type, len1, len2);
                fseek(fh, len1 * 256 + len2 - 2, SEEK_CUR);
                break;
            }
        }
    }
    return -1;
}

long get_mp4_begin(FILE *fh) {
    char *find_str = "ftyp";
    char *ptr = find_str;
    char *end_ptr = find_str + 4;
    do{
        if(feof(fh)) return -1;
        if(fgetc(fh) == *ptr) ++ptr;
        else ptr = find_str;
    } while(ptr != end_ptr);
    return ftell(fh) - 8;
}

char *mystrcpy(char *str) {
    char *ret = (char*)malloc(strlen(str)+1);
    strcpy(ret, str);
    return ret;
}

char *_get_last(char *filename, char chr) {
    char *dot = strchr(filename, chr);
    if(dot == NULL) {
        return NULL;
    }
    char *next;
    while((next = strchr(dot+1, chr)) != NULL) {
        dot = next;
    }
    return dot;
}

// Get basename of file by stripping off dirname and part after last comma
char *get_basename(char *filename) {
    char *sep = _get_last(filename, '/');
    char *sep2 = _get_last(filename, '\\');  // fallback to Windows path
    if(sep == NULL) sep = sep2;
    if(sep == NULL) sep = filename;  // fallback to just use filename
    else ++sep;
    char *ret = mystrcpy(sep);
    char *dot = _get_last(ret, '.');
    *dot = 0;  // terminate string earlier at last '.' character
    return ret;
}

// Get basename of file by stripping off part after last comma
char *get_fullbasename(char *filename) {
    char *ret = mystrcpy(filename);
    char *dot = _get_last(ret, '.');
    *dot = 0;  // terminate string earlier at last '.' character
    return ret;
}

// Get dirname of path by stripping off part after last '/'
char *get_dirname(char *filename) {
    char *ret = mystrcpy(filename);
    char *sep = _get_last(ret, '/');
    char *sep2 = _get_last(ret, '\\');  // fallback to Windows path
    if(sep == NULL) sep = sep2;
    if(sep != NULL) *sep = 0;  // terminate string earlier at last '/' character
    else return mystrcpy("");
    return ret;
}

char *path_join3(char *part1, char *part2, char* part3) {
    int len1 = strlen(part1);
    int len2 = strlen(part2);
    int len3 = strlen(part3);
    char *ret = (char*)malloc(len1 + len2 + len3 + 3);
    strcpy(ret, part1);
    ret[len1] = '/';
    strcpy(ret+len1+1, part2);
    ret[len1+len2+1] = '/';
    strcpy(ret+len1+len2+1, part3);
    return ret;
}

void copy_part(char *in_filename, char *out_filename, long start, long end) {
    FILE *fhout = fopen(out_filename, "wb");
    if(fhout == NULL) {
        fprintf(stderr, "Failed opening file for writing: %s: %s\n", out_filename, strerror(errno));
        exit(-1);
    }
    FILE *fhin = fopen(in_filename, "rb");
    if(fhin == NULL) {
        fprintf(stderr, "Failed opening file for reading: %s: %s\n", in_filename, strerror(errno));
        exit(-1);
    }
    fseek(fhin, start, SEEK_SET);
    const int buffer_size = 0x10000; // 65536
    char buffer[buffer_size];
    long len = -1;
    long cum_len = 0;
//    printf("Buffer: %p\n", (void*)buffer);
    while((len = fread(buffer, 1, buffer_size, fhin)) > 0) {
//        printf("Read %ld bytes\n", len);
        if(end >=0 && cum_len + len > end) {
            len = end - cum_len;
            if(len <= 0) {
                break;
            }
        }
//        printf("Buffer: %p %ld %p\n", (void*)buffer, len, (void*)fhout);
        fwrite(buffer, len, 1, fhout);
        cum_len += len;
    }
    fclose(fhout);
    fclose(fhin);
}

void process_file(char *filename, char *dir) {
    FILE *fh = fopen(filename, "rb");
    long jpg_end = get_jpg_end(fh);
    long mp4_begin = get_mp4_begin(fh);
    fclose(fh);
    if(jpg_end < 0) {
        fprintf(stderr, "Failed processing file (jpeg end not found): %s", filename);
        exit(-1);
    }
    printf("Processing file: %s (jpg end=%ld; mp4 start=%ld)\n", filename, jpg_end, mp4_begin);
    if(mp4_begin >= 0) {
        char *basename;
        if(dir == NULL) {
             basename = get_fullbasename(filename);
             dir = ".";
        } else {
             basename = get_basename(filename);
        }
        char *out_jpg = path_join3(dir, basename, "_pic.jpg");
        copy_part(filename, out_jpg, 0, jpg_end);
        free(out_jpg);
        char *out_mp4 = path_join3(dir, basename, "_vid.mp4");
        copy_part(filename, out_mp4, mp4_begin, -1);
        free(out_mp4);
        free(basename);
    }
}

int isdir(char *path) {
    DIR *dh = opendir(path);
    int isdir = dh != NULL;
    closedir(dh);
    return isdir;
}

int main(int argc, char *argv[]) {
    if(argc <= 1) {
        printf("Synopsis:\n");
        printf("  split_video <filename> [<filename> ...] [<output dir>]\n");
        printf("Example:\n");
        printf("  split_video PXL_20210221_151046209.MP.jpg\n");
        printf("Example if shell expands parameters:\n");
        printf("  split_video *.MP.jpg out/\n");
    } else {
        int n_files = argc - 1;
        char *dir = NULL;
        if(isdir(argv[argc - 1])) {
            --n_files;
            dir = argv[argc - 1];
            printf("Writing output to directory: %s\n", dir);
        }
        for(int i=1; i<=n_files; ++i) {
            process_file(argv[i], dir);
        }
    }
    return 0;
}