#include "android/camera/camera-capture.h"
#include "android/camera/camera-format-converters.h"
#include <stdio.h>
#include <assert.h>

unsigned long   android_verbose;

struct Camera {
	CameraDevice* device;
	// CameraInfo camera_info;
	unsigned width, height, pixel_format, frame_size;
	uint8_t* frame;
};

void read_frame(struct Camera* camera/*CameraDevice* cd, unsigned pixel_format, unsigned width, unsigned height, uint8_t* video_frame*/) {
	ClientFrameBuffer fbs[2];
    int fbs_num = 0, repeat = 0;
    size_t payload_size;
    uint64_t tick;
    float r_scale = 1.0f, g_scale = 1.0f, b_scale = 1.0f, exp_comp = 1.0f;

    /* --------- camera-service.c --------------------------- */
	/* ------------------------------------------------------ */
    fbs[fbs_num].pixel_format = camera->pixel_format;
    fbs[fbs_num].framebuffer = camera->frame;
    fbs_num++;

    /* Capture new frame. */
    tick = _get_timestamp();
    repeat = camera_device_read_frame(camera->device, fbs, fbs_num,
                                      r_scale, g_scale, b_scale, exp_comp);

    /* Note that there is no (known) way how to wait on next frame being
     * available, so we could dequeue frame buffer from the device only when we
     * know it's available. Instead we're shooting in the dark, and quite often
     * device will response with EAGAIN, indicating that it doesn't have frame
     * ready. In turn, it means that the last frame we have obtained from the
     * device is still good, and we can reply with the cached frames. The only
     * case when we need to keep trying to obtain a new frame is when frame cache
     * is empty. To prevent ourselves from an indefinite loop in case device got
     * stuck on something (observed with some Microsoft devices) we will limit
     * the loop by 2 second time period (which is more than enough to obtain
     * something from the device) */
    while (repeat == 1 /*&& !cc->frames_cached*/ &&
           (_get_timestamp() - tick) < 2000000LL) {
        /* Sleep for 10 millisec before repeating the attempt. */
        _camera_sleep(10);
        repeat = camera_device_read_frame(camera->device, fbs, fbs_num,
                                          r_scale, g_scale, b_scale, exp_comp);
    }
    /* ------------------------------------------------------ */
    if (repeat == 1) {
    	printf("Camera read timeout\n");
    } else if (repeat < 0) {
    	printf("Camera read error\n");
    } else {
    	printf("A frame is read\n");
    }
}

int andy_camera_connect() {

}

int main(int argc, char** argv) {
	CameraDevice* cd;
	const int max = 10;
	int cis_n, i, width, height;
	unsigned target_pixel_format = V4L2_PIX_FMT_NV21;
	CameraInfo cis[10];
	struct Camera camera;

	cis_n = enumerate_camera_devices(cis, max);
	assert(cis_n >= 0);
	if(cis == 0) {
		printf("No cameras found\n");
		return 1;
	}
	printf("Cameras(%d): \n", cis_n);
	for(i=0; i<cis_n; i++) {
		CameraInfo inf = cis[i];
		char ss[256] = {0}, *s = ss, fourcc[5] = {0};
		int j, pf = inf.pixel_format;

		for(j = 0; j < inf.frame_sizes_num; j++) {
			s += sprintf(s, "%dx%d; ", inf.frame_sizes[j].width, inf.frame_sizes[j].height);
			if(j == 0) {
				width = inf.frame_sizes[0].width;
				height = inf.frame_sizes[0].height;
			}
		}
		for(j=0; j < 4; j++) {
			fourcc[j] = pf & 0xFF;
			pf >>= 8;
		}
		printf("  display_name: %s; device_name: %s; pixel_format: %.4s; frame_sizes(%d): %s\n", inf.display_name, inf.device_name, (char*)&inf.pixel_format, inf.frame_sizes_num, ss);
	}

	if (!has_converter(cis[0].pixel_format, target_pixel_format) ||
        !has_converter(cis[0].pixel_format, V4L2_PIX_FMT_RGB32)) {
        printf("No conversion exist between %.4s and %.4s (or RGB32) pixel formats\n",
          		(char*)&cis[0].pixel_format, (char*)&target_pixel_format);
	}

	cd = camera_device_open(cis[0].display_name, cis[0].inp_channel);
	assert(cd != NULL);

	{
		camera.device = cd;
		camera.width = width;
		camera.height = height;
		camera.pixel_format = target_pixel_format;
		unsigned pixel_num = width * height;
		FILE* out = fopen("camera.bin", "wb");

	    switch (target_pixel_format) {
	        case V4L2_PIX_FMT_YUV420:
	        case V4L2_PIX_FMT_YVU420:
	        case V4L2_PIX_FMT_NV12:
	        case V4L2_PIX_FMT_NV21:
	            camera.frame_size = (pixel_num * 12) / 8;
	            break;
	        case V4L2_PIX_FMT_BGR32:
	        case V4L2_PIX_FMT_RGB32:
	        	camera.frame_size = pixel_num * 4;
	            break;
	        // TODO: add more formats
	    }
		camera.frame = (uint8_t*)malloc(camera.frame_size);
		assert( camera_device_start_capturing(camera.device, camera.pixel_format, camera.width, camera.height) == 0 );

		while(true) {
			read_frame(&camera);
			fwrite(camera.frame, camera.frame_size, 1, out);
			fflush(out);
		}
	}

	return 0;
}