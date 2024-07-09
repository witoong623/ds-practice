# How NV12 data is store, what is YUV 4:2:0
- Y which is intensity is 1 byte per pixel.
    - If image is 10x10. There is a plane of 10 x 10 = 100 bytes.
- U and V which are "8 bit 2x2 subsampled colour difference samples"
    - Image is divide into 2x2 block. If image is 10x10 pixels. There are 10x10 / 2x2 = 25 blocks.
    - Each block use 1 byte for U and 1 byte for V, so there are 25 blocks x 2 = 50 byte.
- In total, there are 150 bytes in this image.
    - RGB image use 300 bytes for 10x10 image.

## Example
These are values I got from NvBufSurface and how I create OpenCV in YUV image from it buffer.
- Frame size: width 1920, height 1080
- Arguments when creating from buffer: height = height * 3 / 2 = 1620, width = 1920, CV_8UC1 (one channel), pitch = 2048
- Number of bytes/pixels
  - Number of Y pixels: 1080 * 1920 = 2,073,600
  - Number of U: 2,073,600 / 4 = 518,400
  - Number of V: 2,073,600 / 4 = 518,400
  - Total number of pixels/bytes: 2,073,600 + (518,400 * 2) = 3,110,400

- [Pitch is the number of bytes per row on screen](https://jsandler18.github.io/extra/framebuffer.html)
  - Each row has 1920 pixels, so we already have 1920 bytes.
  - We can think of each pixel share 2 bits of U and 2 bits of V, So 4 bits per pixel for color.
  - 1920 * 4 bits = 7,680 bits / 8 bits = 960 bytes.
  - 1920 + 960

## Number of frame what we can buffer
In practice, each image is 3.3 MB. If we buffer at 15 FPS, 10 second is 150 frames * 3.3 = 495 MB.
