import os


def split_motion_picture_jpeg(filename, jpg_file, mp4_file):
    with open(filename, "rb") as fh:
        data = fh.read()

    jpeg_end = get_jpeg_end(data)
    print(f"JPG ends: {jpeg_end}")
    try:
        mp4_start = get_mp4_start(data)
        print(f"MP4 starts: {mp4_start}")
    except AssertionError:
        print(f"No mp4 found within JPEG: {filename}")
        return

    with open(jpg_file, "wb") as fh:
        fh.write(data[0:jpeg_end])

    with open(mp4_file, "wb") as fh:
        fh.write(data[mp4_start:])


def get_mp4_start(data):
    pos = 0
    ftyp = bytes([ord(c) for c in "ftyp"])
    while True:
        if data[pos:pos+4] == ftyp:
            break
        pos += 1
        assert pos < len(data)
    return pos - 4


def get_jpeg_end(data):
    pos = 0
    last_pos = -1
    while data[pos] == 0xff:
        last_pos = pos
        if data[pos + 1] in [0xd8]:
            offset = 2
        elif data[pos + 1] == 0xd9:
            return pos + 2
        elif data[pos + 1] == 0xda:
            pos += 2
            # scan for next ff xx marker with xx not in (ff, d0-d7)
            while True:
                if data[pos] == 0xff and data[pos + 1] not in ([0, 0xff] + [0xd0 + i for i in range(8)]):
                    break
                pos += 1
                assert pos < len(data)
            continue
        else:
            offset = data[pos + 2] * 256 + data[pos + 3] + 2
        pos += offset
        assert pos < len(data)

    _ = last_pos
    # print(str(["%x" % c for c in data[last_pos:last_pos + 10]]))
    # print(str(["%x" % c for c in data[pos - 5:pos + 10]]))
    raise ValueError(f"Could not find end of JPEG")


if __name__ == "__main__":
    file1 = os.path.join(os.path.dirname(__file__), "..", "target", "PXL_20210221_151046209.MP.jpg")
    jpg_file = os.path.splitext(file1)[0] + "_pic.jpg"
    mp4_file = os.path.splitext(file1)[0] + "_vid.mp4"
    split_motion_picture_jpeg(file1, jpg_file, mp4_file)
