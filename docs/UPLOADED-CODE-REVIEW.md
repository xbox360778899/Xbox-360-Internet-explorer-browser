# Review of the supplied video and GPU files

Reviewed in full: **video.c 264.txt** (679 lines, 21,086 bytes) and **gpu.c** (243 lines, 8,532 bytes). Comments in the files were treated as source documentation, not instructions. The supplied Orbita GPU, decoder and UI modules were also used as references.

## What they provide

The first file is a Moonlight-style H.264 elementary-stream decoder/presentation pipeline using FFmpeg and libxenon. The second uploads decoded Y, U and V planes to Xenos and draws them with a YUV-to-RGB shader. Neither file contains HTML parsing, CSS layout, a JavaScript engine, an MP4 demuxer or an AAC audio pipeline.

## video.c 264.txt, function by function

| Function / block | Assessment |
|---|---|
| Packet ring / frame mailbox | Eight packet slots and a three-frame ownership handoff separate receiving, decoding and rendering. A useful design for a future native decoder. |
| video_renderer_name | Describes the GPU, CPU and experimental display-matrix presentation paths. |
| video_set_options | Copies playback options; callers should change them outside active decode work or provide synchronization. |
| video_get_display_size | Reads a libxenon-specific hardware structure at a fixed address; not copied into the XDK application. |
| video_get_stats | Returns approximate counters, not an atomic multi-field snapshot. |
| video_has_overlay | Always returns false. It does not provide a functioning browser/watermark overlay. |
| video_set_overlay | Stub: ignores all arguments. A real compositor must be implemented separately. |
| build_color_luts | Precomputes BT.601 limited-range conversion and clamps. Good CPU fallback, but range/matrix must match the decoded stream. |
| csc_encode | Converts coefficients to the display controller's register representation. Specific to the low-level backend. |
| csc_decode | Converts the same representation back for composing output transforms. |
| csc_restore | Restores saved display registers; resource/state restoration is important. |
| csc_program_yuv | Detects selected output modes and changes the scanout color matrix. Kept as reference; the XEX uses XDK rendering rather than direct register writes. |
| tile_row | Computes a tiled framebuffer row offset. Depends on the framebuffer's exact layout. |
| setup_display | Computes aspect-fit scaling maps and initializes framebuffer storage. Allocation failures need centralized cleanup; dimensions and multiplication bounds should be validated before allocation. |
| render_rows_scaled | CPU YUV conversion and nearest-neighbor scaling. Assumes the expected YUV420 plane geometry. |
| render_rows_1to1 | Processes 2x2 blocks sharing chroma samples. Requires the even-dimension constraints checked by the caller. |
| render_frame | Divides work into tiled bands and flushes CPU cache ranges. These cache operations are libxenon-specific. |
| decoder_publish | Moves AVFrame ownership on the decoder side and publishes the newest slot. This keeps FFmpeg reference handling out of the renderer. |
| decoder_task | Sends compressed packets to FFmpeg and receives decoded frames. It accepts YUV420P and YUVJ420P, but the fixed limited-range conversion does not distinguish full-range YUVJ. |
| render_task | Picks the latest queued frame and marks rendering ownership. Polling loops should be measured for CPU cost and bounded shutdown behavior. |
| video_setup | Selects only H.264 and configures slice threading. AVPacket/AVFrame allocations need explicit null checks; partial setup failures need complete cleanup. |
| video_start | Starts libxenon hardware-thread tasks after publishing state. Requires the missing hw abstraction. |
| video_stop | Signals stop and waits for both tasks. A blocked decoder/GPU path can make this wait indefinite; a production port needs cancellation/timeouts. |
| video_cleanup | Releases codec/frame/packet resources and restores display state. Setup-failure cleanup should converge on equivalent ownership logic. |
| video_submit | Copies a Moonlight decode unit into the packet ring and requests a keyframe on overflow. It should enforce an overall packet limit, checked allocation arithmetic and that the sum of linked buffer lengths never exceeds the advertised fullLength/capacity. |
| video_init_callbacks | Registers Moonlight callbacks and advertises four slices per frame. This contract describes a controlled streaming source, not arbitrary website media. |

## gpu.c, function by function

| Function / block | Assessment |
|---|---|
| ps_yuv / vs_passthrough | Precompiled shader arrays attributed in the source to the GPL-2.0-or-later MPlayer/libxenon port. They were not copied into the XEX. The fixed matrix assumes BT.601 limited-range video. |
| vertex layout / renderer state | Separate plane textures and two scanout buffers are distinct from the decoder's three-frame mailbox. Their lifetimes must remain explicit. |
| gpu_init | Initializes libxenon Xenos state, shaders, vertex buffer and scanout surfaces. Partial initialization failures should release allocations already made. |
| gpu_ready | Reports completed initialization. |
| make_plane | Creates a sampled 8-bit plane with clamping and filtering. |
| destroy_planes | Releases Y/U/V textures and nulls their pointers. |
| clear_into | Resolves a clear to a scanout surface and waits for completion. |
| gpu_stream_begin | Recreates planes and builds an aspect-fit quad. Should validate nonzero/even dimensions and all size limits before creating half-resolution chroma textures. |
| upload_plane | Copies rows respecting source linesize and destination pitch, then unlocks the surface for cache synchronization. This is the important pattern to retain when adapting graphics APIs. |
| gpu_present | Uploads planes, resets render state, draws, resolves and flips the hidden buffer. It explicitly waits for GPU completion; its vblank spin loop has no timeout. |
| gpu_stream_end | Restores the framebuffer used by the text console. It does not supply an XDK browser compositor. |

## Dependency and porting requirements

The files need video.h, gpu.h, hw.h, Moonlight declarations/callbacks, libxenon Xenos APIs, PPC cache/atomic operations, FFmpeg libraries and a compatible worker-thread implementation. The two files alone cannot be built as this project's Visual Studio 2010/XDK XEX. The Orbita archive also targets a libxenon ELF runtime; its built ELF libraries are not drop-in XDK libraries.

For MP4/H.264 website video, a port would additionally need container demuxing, stream metadata/extradata and timestamp handling, AAC/other audio decoding, audio-clock synchronization, buffering/seeking, and an XDK-compatible texture/compositor backend. This review does **not** claim that those features were added.

## Changes this review informed

1. Added an explicit XDK GPU-idle synchronization before overwriting the shared UI texture. This addresses texture ownership rather than pasting libxenon register/cache code into an XEX.
2. Retained native WMV/XMV playback and its separate overlay. H.264 support is not marked implemented.
3. Added a separate litehtml-based HTML/CSS renderer. CSS cannot be obtained from either supplied video file.
4. Rebuilt browser symbols on a common antialiased grid and kept the cursor artwork separate from video textures.

This is a source review, not a hardware validation of the attached code. The shader attribution is explicit in gpu.c; a complete upstream source/license chain is still needed before redistributing any additional video dependencies.
