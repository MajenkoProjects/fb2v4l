#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <stdlib.h>
#include <string.h>
#include "fb.h"

#define NAME "fb2v4l"
const uint32_t format = V4L2_PIX_FMT_ARGB32;

int main(int argc, char **argv) {

    uint32_t width = 800; 
    uint32_t height = 600;
    char *fbdev = NULL;
    char *v4ldev = NULL;

    int c;
    while ((c = getopt(argc, argv, "w:h:f:v:")) != -1) {
        switch (c) {
            case 'w':
                width = atoi(optarg);
                break;
            case 'h':
                height = atoi(optarg);
                break;
            case 'f':
                fbdev = optarg;
                break;
            case 'v':
                v4ldev = optarg;
                break;
        }
    }

    if ((fbdev == NULL) || (v4ldev == NULL)) {
        fprintf(stderr, "Usage: " NAME " [-w width] [-h height] -f /dev/fb0 -v /dev/video4\n");
        return -1;
    }

    struct fb_var_screeninfo screen;

    screen.xres             = width;
    screen.yres             = height;
    screen.xres_virtual     = screen.xres;
    screen.yres_virtual     = screen.yres;
    screen.xoffset          = 0;
    screen.yoffset          = 0;
    screen.bits_per_pixel   = 32;
    screen.grayscale        = 0;

    screen.red.offset       = 16;
    screen.red.length       = 8;
    screen.red.msb_right    = 0;

    screen.green.offset     = 8;
    screen.green.length     = 8;
    screen.green.msb_right  = 0;

    screen.blue.offset      = 0;
    screen.blue.length      = 8;
    screen.blue.msb_right   = 0;

    screen.transp.offset    = 24;
    screen.transp.length    = 8;
    screen.transp.msb_right = 0;

    screen.nonstd           = 0;
    screen.activate         = 0;
    screen.height           = screen.yres;
    screen.width            = screen.xres;
    screen.accel_flags      = 0;

    // {{ This is the "timings" in fbset
    screen.pixclock         = 25000;
    screen.left_margin      = 88;
    screen.right_margin     = 40;
    screen.upper_margin     = 23;
    screen.lower_margin     = 1;
    screen.hsync_len        = 128;
    // }}

    screen.vsync_len        = 4;
    screen.sync             = 0;
    screen.vmode            = 0;
    screen.rotate           = 0;
    screen.colorspace       = 0;
    screen.reserved[0]      = 0;
    screen.reserved[1]      = 0;
    screen.reserved[2]      = 0;
    screen.reserved[3]      = 0;

    int fbfd = open(fbdev, O_RDWR);
    if (fbfd < 0) {
        fprintf(stderr, NAME ": Unable to open %s: %s\n", fbdev, strerror(errno));
        return -1;
    }
    ioctl(fbfd, FBIOPUT_VSCREENINFO, &screen);

    uint8_t *fb = (uint8_t *)mmap(NULL, (width * height * 4), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fbfd, 0);

    
    struct v4l2_capability vid_caps;
    struct v4l2_format vid_format;

    int fdv4l = open(v4ldev, O_RDWR);
    if (fdv4l < 0) {
        fprintf(stderr, NAME ": Can't open %s: %s\n", v4ldev, strerror(errno));
        return -1;
    }

    int ret_code = 0;
    ret_code = ioctl(fdv4l, VIDIOC_QUERYCAP, &vid_caps);

    if (ret_code < 0) {
        fprintf(stderr, NAME ": Can't get video device capabilities: %s\n", strerror(errno));
        return -1;
    }

    memset(&vid_format, 0, sizeof(vid_format));

    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    vid_format.fmt.pix.width = width;
    vid_format.fmt.pix.height = height;
    vid_format.fmt.pix.sizeimage = width * height * 4;
    vid_format.fmt.pix.pixelformat = format;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;
    vid_format.fmt.pix.bytesperline = width * 4;
    vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    ret_code = ioctl(fdv4l, VIDIOC_S_FMT, &vid_format);
    if (ret_code < 0) {
        fprintf(stderr, NAME ": Can't set video device parameters: %s\n", strerror(errno));
        return -1;
    }

    while (1) {
        if (write(fdv4l, fb, vid_format.fmt.pix.sizeimage) < vid_format.fmt.pix.sizeimage) {
            return -1;
        }
            
        usleep(10000);
    }

    return 0;
}
