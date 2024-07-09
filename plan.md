# Analytic base on type and configuration
- Define supported high level analysis.
    - Vehicle crossing line counting.
- Define configuration for each high level analysis.
    - Configuration must be per source. If this source doesn't want this analysis, don't do that.
- For each high level analysis
    - Everything must be per source.
    - Define data structure that hold necessary measurement tool (i.e. Line).
    - Define state of analysis. If it is vehicle crossing line counting, there must be count variable.

# Buffering frames
- Store frame in NV12/YUV420.
- N layers of classes that work together.
    - Memory buffer layer.  This layer keeps track of the a chunk of memory from start pointer, with size (and possibly max size if we allow size). Must support signaling back to the allocator that no one is using itself anymore.
    - Frame buffer allocator. This layer pre-allocates all memory buffers. Keep track which one is in use or not. Have method to give empty buffer (whether from pre-allocated or create new one if no memory buffer is available).
    - Frame buffering layer. Interface with application directly to copy data from source to memory buffer. Keep track of source id, frame id, untrack the old frame which no one use.

# Detail of buffer
On PGIE's sink, I got the following numbers.
- GPU ID 0, batch size 1, numFilled 1, isContiguous 0, cuda device memory.
- Color in copied surface is NVBUF_COLOR_FORMAT_NV12_709, data size is 3317760
On Tiler's sink, I got the following numbers.
- GPU ID 0, batch size 1, numFilled 1, isContiguous 0, cuda device memory.
- Color in copied surface is NVBUF_COLOR_FORMAT_NV12_709, data size is 3317760

## FPS
Video at 30 FPS (human and bus video), live-source = 0.
- 470: Not using frame buffer at all. No probe attach.
- 155-165: Use OpenCV Mat for buffer.
Video 4K at 25 FPS (highway vehicle counting video), live-source = 0
- 210: Not using frame buffer at all. No probe attach.
- 134-145: Use OpenCV Mat for buffer.
