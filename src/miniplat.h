// The native LG build with FFmpeg generates seek-bar thumbnails on-device.
// Other builds use Seekr.
#ifndef NV_MINIPLAT_H
#define NV_MINIPLAT_H
#if defined(NV_WEBOS) && defined(NV_DTS_FFMPEG)
#define NV_MINIATURAS 1
#endif
#endif
