package com.nan.gbd.library.media;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class MediaOutput {
    private String serverIp;
    private int serverPort;
    private int localPort;
    private String outputDir;
    private String outputName;
    private int outputType;
    private int ssrc;
    private int duration; // 录制时长(秒),[0, 60*60]

    public MediaOutput(String serverIp, int serverPort, String outputDir, String outputName, int outputType, int ssrc) {
        this.serverIp = serverIp;
        this.serverPort = serverPort;
        this.outputDir = outputDir;
        this.outputName = outputName;
        this.outputType = outputType;
        this.ssrc = ssrc;
    }

    public MediaOutput(String serverIp, int serverPort, int localPort, String outputDir, String outputName, int outputType, int ssrc) {
        this.serverIp = serverIp;
        this.serverPort = serverPort;
        this.localPort = localPort;
        this.outputDir = outputDir;
        this.outputName = outputName;
        this.outputType = outputType;
        this.ssrc = ssrc;
    }

    public String getServerIp() {
        return serverIp;
    }

    public void setServerIp(String serverIp) {
        this.serverIp = serverIp;
    }

    public int getServerPort() {
        return serverPort;
    }

    public void setServerPort(int serverPort) {
        this.serverPort = serverPort;
    }

    public int getLocalPort() {
        return localPort;
    }

    public void setLocalPort(int localPort) {
        this.localPort = localPort;
    }

    public String getOutputDir() {
        return outputDir;
    }

    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }

    public String getOutputName() {
        return outputName;
    }

    public void setOutputName(String outputName) {
        this.outputName = outputName;
    }

    public int getOutputType() {
        return outputType;
    }

    public void setOutputType(int outputType) {
        this.outputType = outputType;
    }

    public int getSsrc() {
        return ssrc;
    }

    public void setSsrc(int ssrc) {
        this.ssrc = ssrc;
    }

    public int getDuration() {
        return duration;
    }

    public void setDuration(int duration) {
        this.duration = duration;
    }
}
