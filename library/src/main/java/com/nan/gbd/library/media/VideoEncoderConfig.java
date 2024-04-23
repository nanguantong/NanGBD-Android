package com.nan.gbd.library.media;

public class VideoEncoderConfig {
    public static class Dimension {
        public int width;
        public int height;

        public Dimension(int width, int height) {
            this.width = width;
            this.height = height;
        }

        public Dimension() {
            this.width = 640;
            this.height = 480;
        }
    }

    public static final Dimension VD_120x120 = new Dimension(120, 120);
    public static final Dimension VD_160x120 = new Dimension(160, 120);
    public static final Dimension VD_180x180 = new Dimension(180, 180);
    public static final Dimension VD_240x180 = new Dimension(240, 180);
    public static final Dimension VD_320x180 = new Dimension(320, 180);
    public static final Dimension VD_240x240 = new Dimension(240, 240);
    public static final Dimension VD_320x240 = new Dimension(320, 240);
    public static final Dimension VD_424x240 = new Dimension(424, 240);
    public static final Dimension VD_360x360 = new Dimension(360, 360);
    public static final Dimension VD_480x360 = new Dimension(480, 360);
    public static final Dimension VD_640x360 = new Dimension(640, 360);
    public static final Dimension VD_480x480 = new Dimension(480, 480);
    public static final Dimension VD_640x480 = new Dimension(640, 480);
    public static final Dimension VD_840x480 = new Dimension(840, 480);
    public static final Dimension VD_960x720 = new Dimension(960, 720);
    public static final Dimension VD_1280x720 = new Dimension(1280, 720);
    public static final Dimension VD_1920x1080 = new Dimension(1920, 1080);
    public static final Dimension VD_2540x1440 = new Dimension(2540, 1440);
    public static final Dimension VD_3840x2160 = new Dimension(3840, 2160);

    public static final int STANDARD_BITRATE = 0;
    public static final int COMPATIBLE_BITRATE = -1;
    public static final int DEFAULT_MIN_BITRATE = -1;
    public static final int DEFAULT_MIN_FRAMERATE = -1;
    public Dimension dimensions;
    public int frameRate;
    public int minFrameRate;
    public int bitrate;
    public int minBitrate;
    public int gop;
    public OrientationMode orientationMode;
    public DegradationPreference degradationPrefer;
    public int mirrorMode;

    public enum FrameRate {
        FRAME_RATE_FPS_1(1),
        FRAME_RATE_FPS_7(7),
        FRAME_RATE_FPS_10(10),
        FRAME_RATE_FPS_15(15),
        FRAME_RATE_FPS_24(24),
        FRAME_RATE_FPS_30(30);

        private int value;
        FrameRate(int fps) {
            this.value = fps;
        }
        public int getValue() { return this.value; }
    }

    public enum OrientationMode {
        ORIENTATION_MODE_ADAPTIVE(0),
        ORIENTATION_MODE_FIXED_LANDSCAPE(1),
        ORIENTATION_MODE_FIXED_PORTRAIT(2);

        private int value;
        OrientationMode(int mode) { this.value = mode; }
        public int getValue() { return this.value; }
    }

    // 带宽受限时，视频编码降级偏好
    public enum DegradationPreference
    {
        MAINTAIN_QUALITY(0),
        MAINTAIN_FRAMERATE(1),
        MAINTAIN_BALANCED(2);

        private int value;
        DegradationPreference(int pre) { this.value = pre; }
        public int getValue() { return this.value; }
    }

    public VideoEncoderConfig() {
        this.dimensions = new Dimension(640, 480);
        this.frameRate = FrameRate.FRAME_RATE_FPS_15.getValue();
        this.minFrameRate = -1;
        this.bitrate = 0;
        this.minBitrate = -1;
        this.orientationMode = OrientationMode.ORIENTATION_MODE_ADAPTIVE;
        this.degradationPrefer = DegradationPreference.MAINTAIN_QUALITY;
        this.mirrorMode = 0;
    }

    public VideoEncoderConfig(Dimension dim, FrameRate fps, int bitrate, int gop, OrientationMode mode) {
        this.dimensions = dim;
        this.frameRate = fps.getValue();
        this.minFrameRate = -1;
        this.bitrate = bitrate;
        this.minBitrate = -1;
        this.gop = gop;
        this.orientationMode = mode;
        this.degradationPrefer = DegradationPreference.MAINTAIN_QUALITY;
        this.mirrorMode = 0;
    }

    public VideoEncoderConfig(int width, int height, FrameRate fps, int bitrate, int gop, OrientationMode mode) {
        this.dimensions = new Dimension(width, height);
        this.frameRate = fps.getValue();
        this.minFrameRate = -1;
        this.bitrate = bitrate;
        this.minBitrate = -1;
        this.gop = gop;
        this.orientationMode = mode;
        this.degradationPrefer = DegradationPreference.MAINTAIN_QUALITY;
        this.mirrorMode = 0;
    }
}
