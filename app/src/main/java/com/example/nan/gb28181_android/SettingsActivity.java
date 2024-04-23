package com.example.nan.gb28181_android;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.databinding.DataBindingUtil;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Toast;

import com.example.nan.gb28181_android.databinding.ActivitySettingsBinding;
import com.example.nan.gb28181_android.utils.SPUtil;
import com.nan.gbd.library.osd.OsdConfig;
import com.nan.gbd.library.sip.GBDevice;
import com.nan.gbd.library.sip.SipConfig;
import com.nan.gbd.library.utils.DeviceUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * @author NanGBD
 * @date 2019.6.8
 */
public class SettingsActivity extends AppCompatActivity implements Toolbar.OnMenuItemClickListener {

    public static final int REQUEST_OVERLAY_PERMISSION = 1002;

    private Context mContext;
    private ActivitySettingsBinding mBinding;

    private SipConfig mSipConfig;
    private int mWidth, mHeight;

    private String mVideoEncodec;
    private int mFramerate;
    private String mVideoProfile;
    private String mVideoBitrateMode;
    private GBDevice.GmLevel mGmLevel;
    private GBDevice.Charset mCharset;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mBinding = DataBindingUtil.setContentView(this, R.layout.activity_settings);
        setSupportActionBar(mBinding.toolbarMain);
        Objects.requireNonNull(getSupportActionBar()).setDisplayHomeAsUpEnabled(true);
        mBinding.toolbarMain.setOnMenuItemClickListener(this);
        mBinding.toolbarMain.setNavigationIcon(R.drawable.nav_back);

        mContext = getApplicationContext();

        Intent intent = getIntent();
        mWidth = intent.getIntExtra("width", 640);
        mHeight = intent.getIntExtra("height", 480);

        mSipConfig = SPUtil.getSipConfig();
        GBDevice gbDevice = mSipConfig.getGbDevice();
        mBinding.txtServerIp.setText(mSipConfig.getServerIp());
        mBinding.txtServerPort.setText(String.valueOf(mSipConfig.getServerPort()));
        mBinding.txtServerId.setText(mSipConfig.getServerId());
        mBinding.txtServerDomain.setText(mSipConfig.getServerDomain());
        mBinding.txtServerCert.setText(mSipConfig.getServerCert());
        mBinding.txtName.setText(gbDevice.getName());
        mBinding.txtManufacturer.setText(gbDevice.getManufacturer());
        mBinding.txtModel.setText(gbDevice.getModel());
//        try {
//            mBinding.txtName.setText(new String(gbDevice.getName().getBytes(), gbDevice.getCharset().getValue()));
//            mBinding.txtManufacturer.setText(new String(gbDevice.getManufacturer().getBytes(), gbDevice.getCharset().getValue()));
//            mBinding.txtModel.setText(new String(gbDevice.getModel().getBytes(), gbDevice.getCharset().getValue()));
//        } catch (UnsupportedEncodingException e) { }
        mBinding.txtFirmware.setText(gbDevice.getFirmware());
        mBinding.txtSerialNumber.setText(gbDevice.getSerialNumber());
        mBinding.txtUsername.setText(gbDevice.getUsername());
        mBinding.txtUserid.setText(gbDevice.getUserid());
        mBinding.txtPassword.setText(gbDevice.getPassword());
        mBinding.txtDevicePort.setText(String.valueOf(gbDevice.getPort()));
        mBinding.txtRegExpiry.setText(String.valueOf(gbDevice.getRegExpiry()));
        mBinding.txtHeartbeatInterval.setText(String.valueOf(gbDevice.getHeartbeatInterval()));
        mBinding.txtHeartbeatCount.setText(String.valueOf(gbDevice.getHeartbeatCount()));
        if (gbDevice.getLongitude() != GBDevice.INVALID_LONGITUDE && gbDevice.getLatitude() != GBDevice.INVALID_LATITUDE) {
            mBinding.txtLongitude.setText(String.valueOf(gbDevice.getLongitude()));
            mBinding.txtLatitude.setText(String.valueOf(gbDevice.getLatitude()));
        }
        mBinding.txtVideoId.setText(gbDevice.getVideoId());
        mBinding.txtAudioId.setText(gbDevice.getAudioId());
        mBinding.txtAlarmId.setText(gbDevice.getAlarmId());
        if (gbDevice.getProtocol() == GBDevice.Protocol.UDP) {
            mBinding.sipProtocolUdp.setChecked(true);
        } else {
            mBinding.sipProtocolTcp.setChecked(true);
        }

        OsdConfig osdConfig = SPUtil.getOsdConfig();
        mBinding.ckbOsdEnable.setChecked(osdConfig.isEnable());
        mBinding.txtOsdFontSize.setText(String.valueOf(osdConfig.getFontSize()));
        mBinding.txtOsdCharWidth.setText(String.valueOf(osdConfig.getCharWidth()));
        mBinding.txtOsdCharHeight.setText(String.valueOf(osdConfig.getCharHeight()));

        mBinding.ckbEnableBackgroundCamera.setChecked(SPUtil.getEnableBackgroundCamera(mContext));
        mBinding.ckbEnableBackgroundCamera.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(final CompoundButton buttonView, boolean isChecked) {
                if (!isChecked) {
                    SPUtil.setEnableBackgroundCamera(mContext, false);
                    return;
                }
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    if (Settings.canDrawOverlays(SettingsActivity.this)) {
                        SPUtil.setEnableBackgroundCamera(mContext, true);
                    } else {
                        new AlertDialog
                                .Builder(SettingsActivity.this)
                                .setTitle("后台上传视频")
                                .setMessage("后台摄像头上传视频需要App出现在顶部.是否确定?")
                                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialogInterface, int i) {
                                        // 在Android 6.0后，Android需要动态获取权限，若没有权限，提示获取.
                                        final Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                                                Uri.parse("package:" + BuildConfig.APPLICATION_ID));
                                        startActivityForResult(intent, REQUEST_OVERLAY_PERMISSION);
                                    }
                                }).setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialogInterface, int i) {
                                SPUtil.setEnableBackgroundCamera(mContext, false);
                                buttonView.toggle();
                            }
                        }).setCancelable(false).show();
                    }
                } else {
                    SPUtil.setEnableBackgroundCamera(mContext, true);
                }
            }
        });

//        // TODO: 使能aac编码
//        mBinding.enableAac.setChecked(SPUtil.getAACCodec(mContext));
//        mBinding.enableAac.setOnCheckedChangeListener(
//                (buttonView, isChecked) -> SPUtil.setAACCodec(mContext, isChecked)
//        );

        // 推流选择音视频参数
        boolean enableVideo = SPUtil.getEnableVideo(mContext);
        if (enableVideo) {
            boolean enableAudio = SPUtil.getEnableAudio(mContext);
            if (enableAudio) {
                mBinding.rbPushAv.setChecked(true);
            } else {
                mBinding.rbPushV.setChecked(true);
            }
        } else {
            mBinding.rbPushA.setChecked(true);
        }
        mBinding.rgPush.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                boolean enableVideo = true, enableAudio = false;
                if (checkedId == R.id.rb_push_av) {
                    enableVideo = true;
                    enableAudio = true;
                } else if (checkedId == R.id.rb_push_v) {
                    enableVideo = true;
                    enableAudio = false;
                } else if (checkedId == R.id.rb_push_a) {
                    enableVideo = false;
                    enableAudio = true;
                }
                SPUtil.setEnableVideo(mContext, enableVideo);
                SPUtil.setEnableAudio(mContext, enableAudio);
            }
        });

        mBinding.ckbAutoThread.setChecked(SPUtil.getThreadNum(mContext) == -1);
        mBinding.ckbAutoThread.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(final CompoundButton buttonView, boolean isChecked) {
                if (!isChecked) {
                    int num = DeviceUtils.getNumCores();
                    SPUtil.setThreadNum(mContext, num / 2);
                } else {
                    SPUtil.setThreadNum(mContext, -1);
                }
            }
        });

        mBinding.ckbAudioStartup.setChecked(SPUtil.getAudioStartup(mContext));
        mBinding.ckbAudioStartup.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(final CompoundButton buttonView, boolean isChecked) {
                if (!isChecked) {
                    SPUtil.setAudioStartup(mContext, false);
                } else {
                    SPUtil.setAudioStartup(mContext, true);
                }
            }
        });

        // 输出码率
        int bitrate_added_kbps = SPUtil.getBitrateKbps(mContext);
        int kbps = 72000 + bitrate_added_kbps;
        mBinding.tvBitrate.setText(kbps/1000 + "kbps");
        mBinding.seekbarBitrate.setMax(6000000);
        mBinding.seekbarBitrate.setProgress(bitrate_added_kbps);
        mBinding.seekbarBitrate.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int kbps = 72000 + progress;
                mBinding.tvBitrate.setText(kbps/1000 + "kbps");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                SPUtil.setBitrateKbps(mContext, seekBar.getProgress());
            }
        });

        // 视频编码器
        initVideoEncodecSpinner();

        // 视频码率控制
        initVideoBitrateModeSpinner();

        // 帧率
        initFramerateSpinner();

        // 视频profile
        initVideoProfileSpinner();

        // 字符集
        initCharsetSpinner();

        // 国密35114安全等级
        initGmLevelSpinner();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_OVERLAY_PERMISSION) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                boolean canDraw = Settings.canDrawOverlays(this);
                SPUtil.setEnableBackgroundCamera(mContext, canDraw);
                if (!canDraw) {
                    mBinding.ckbEnableBackgroundCamera.setChecked(false);
                }
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return true;
    }

    @Override
    public boolean onMenuItemClick(MenuItem menuItem) {
        return false;
    }

    /**
     * 返回上一级
     * @param item
     * @return
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            updateOsdConfig();
            boolean changed = updateSipConfig();
            if (changed) {
                Intent intent = getIntent();
                Bundle bundle = new Bundle();
                bundle.putBoolean(SPUtil.KEY_CONFIG_CHANGED, true);
                intent.putExtras(bundle);
                setResult(RESULT_OK, intent);
            }
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * 导入服务平台证书
     */
    public void onImportServerCert(View view) {
        String serverId = mBinding.txtServerId.getText().toString();
        String serverCert = mBinding.txtServerCert.getText().toString();
        if (serverId.isEmpty() || serverCert.isEmpty()) {
            return;
        }
        int ret = MainActivity.mSipUa.setServerCert(serverId, serverCert, false, null);
        Toast.makeText(mContext, "导入服务证书" + (ret == 0 ? "成功" : "错误"), Toast.LENGTH_SHORT).show();
    }

    /**
     * 更新osd水印配置项
     */
    private void updateOsdConfig() {
        boolean enable = mBinding.ckbOsdEnable.isChecked();
        int fontSize = Integer.parseInt(mBinding.txtOsdFontSize.getText().toString());
        int charWidth = Integer.parseInt(mBinding.txtOsdCharWidth.getText().toString());
        int charHeight = Integer.parseInt(mBinding.txtOsdCharHeight.getText().toString());
        OsdConfig osdConfig = SPUtil.getOsdConfig();

        osdConfig.setEnable(enable);
        osdConfig.setFontSize(fontSize);
        osdConfig.setCharWidth(charWidth);
        osdConfig.setCharHeight(charHeight);

        SPUtil.setOsdConfig(osdConfig);
    }

    /**
     * 更新sip配置项
     * @return true: 配置有修改变化, 否则配置没有被修改
     */
    private boolean updateSipConfig() {
        SipConfig sipConfig = SPUtil.getSipConfig();
        GBDevice gbDevice = sipConfig.getGbDevice();

        GBDevice.GmLevel gmLevel = (GBDevice.GmLevel) mBinding.spnGmLevel.getSelectedItem();
        GBDevice.Charset charset = (GBDevice.Charset) mBinding.spnCharset.getSelectedItem();

        String serverIp = mBinding.txtServerIp.getText().toString().trim();
        if (serverIp.isEmpty()) {
            return false;
        }
        String str = mBinding.txtServerPort.getText().toString().trim();
        if (str.isEmpty()) {
            return false;
        }
        int serverPort = Integer.parseInt(str);
        String serverId = mBinding.txtServerId.getText().toString().trim();
        if (serverId.isEmpty()) {
            return false;
        }
        String serverDomain = mBinding.txtServerDomain.getText().toString().trim();
        if (serverDomain.isEmpty()) {
            return false;
        }
        String serverCert = mBinding.txtServerCert.getText().toString().trim();

        String name = mBinding.txtName.getText().toString().trim();
        String manufacturer = mBinding.txtManufacturer.getText().toString().trim();
        String model = mBinding.txtModel.getText().toString().trim();
        String firmware = mBinding.txtFirmware.getText().toString().trim();
        String serialNumber = mBinding.txtSerialNumber.getText().toString().trim();
        String username = mBinding.txtUsername.getText().toString().trim();
        String userid = mBinding.txtUserid.getText().toString().trim();
        String password = mBinding.txtPassword.getText().toString().trim();

        str = mBinding.txtDevicePort.getText().toString().trim();
        int devicePort = !str.isEmpty() ? Integer.parseInt(str) : 0;

        GBDevice.Protocol protocol = mBinding.sipProtocolUdp.isChecked() ? GBDevice.Protocol.UDP : GBDevice.Protocol.TCP;

        str = mBinding.txtRegExpiry.getText().toString().trim();
        int regExpiry = !str.isEmpty() ? Integer.parseInt(str) : 3600;

        str = mBinding.txtHeartbeatInterval.getText().toString().trim();
        int heartbeatInterval = !str.isEmpty() ? Integer.parseInt(str) : 30;

        str = mBinding.txtHeartbeatCount.getText().toString().trim();
        int heartbeatCount = !str.isEmpty() ? Integer.parseInt(str) : 3;

        str = mBinding.txtLongitude.getText().toString().trim();
        double longitude = !str.isEmpty() ? Float.parseFloat(str) : GBDevice.INVALID_LONGITUDE;
        str = mBinding.txtLatitude.getText().toString().trim();
        double latitude = !str.isEmpty() ? Float.parseFloat(str) : GBDevice.INVALID_LATITUDE;

        String videoId = mBinding.txtVideoId.getText().toString().trim();
        if (videoId.isEmpty()) {
            return false;
        }
        String audioId = mBinding.txtAudioId.getText().toString().trim();
        String alarmId = mBinding.txtAlarmId.getText().toString().trim();

//        try {
//            name = new String(name.getBytes(), charset.getValue());
//            manufacturer = new String(manufacturer.getBytes(), charset.getValue());
//            model = new String(model.getBytes(), charset.getValue());
//        } catch (UnsupportedEncodingException e) {
//        }

        boolean changed = false;
        if (0 != serverIp.compareTo(sipConfig.getServerIp())) {
            sipConfig.setServerIp(serverIp);
            changed = true;
        }
        if (serverPort != sipConfig.getServerPort()) {
            sipConfig.setServerPort(serverPort);
            changed = true;
        }
        if (0 != serverId.compareTo(sipConfig.getServerId())) {
            sipConfig.setServerId(serverId);
            changed = true;
        }
        if (0 != serverDomain.compareTo(sipConfig.getServerDomain())) {
            sipConfig.setServerDomain(serverDomain);
            changed = true;
        }
        if (0 != serverCert.compareTo(sipConfig.getServerCert())) {
            sipConfig.setServerCert(serverCert);
            changed = true;
        }

        if (gmLevel != gbDevice.getGmLevel()) {
            gbDevice.setGmLevel(gmLevel);
            changed = true;
        }
        if (0 != name.compareTo(gbDevice.getName())) {
            gbDevice.setName(name);
            changed = true;
        }
        if (0 != manufacturer.compareTo(gbDevice.getManufacturer())) {
            gbDevice.setManufacturer(manufacturer);
            changed = true;
        }
        if (0 != model.compareTo(gbDevice.getModel())) {
            gbDevice.setModel(model);
            changed = true;
        }
        if (0 != firmware.compareTo(gbDevice.getFirmware())) {
            gbDevice.setFirmware(firmware);
            changed = true;
        }
        if (0 != serialNumber.compareTo(gbDevice.getSerialNumber())) {
            gbDevice.setSerialNumber(serialNumber);
            changed = true;
        }
        if (0 != username.compareTo(gbDevice.getUsername())) {
            gbDevice.setUsername(username);
            changed = true;
        }
        if (0 != userid.compareTo(gbDevice.getUserid())) {
            gbDevice.setUserid(userid);
            changed = true;
        }
        if (0 != password.compareTo(gbDevice.getPassword())) {
            gbDevice.setPassword(password);
            changed = true;
        }
        if (devicePort != gbDevice.getPort()) {
            gbDevice.setPort(devicePort);
            changed = true;
        }
        if (protocol != gbDevice.getProtocol()) {
            gbDevice.setProtocol(protocol);
            changed = true;
        }
        if (regExpiry != gbDevice.getRegExpiry()) {
            gbDevice.setRegExpiry(regExpiry);
            changed = true;
        }
        if (heartbeatInterval != gbDevice.getHeartbeatInterval()) {
            gbDevice.setHeartbeatInterval(heartbeatInterval);
            changed = true;
        }
        if (heartbeatCount != gbDevice.getHeartbeatCount()) {
            gbDevice.setHeartbeatCount(heartbeatCount);
            changed = true;
        }
        if (0 != videoId.compareTo(gbDevice.getVideoId())) {
            gbDevice.setVideoId(videoId);
            changed = true;
        }
        if (0 != audioId.compareTo(gbDevice.getAudioId())) {
            gbDevice.setAudioId(audioId);
            changed = true;
        }
        if (0 != alarmId.compareTo(gbDevice.getAlarmId())) {
            gbDevice.setAlarmId(alarmId);
            changed = true;
        }
        if (0 != charset.compareTo(gbDevice.getCharset())) {
            gbDevice.setCharset(charset);
            changed = true;
        }
        if (longitude != gbDevice.getLongitude()) {
            gbDevice.setLongitude(longitude);
            changed = true;
        }
        if (latitude != gbDevice.getLatitude()) {
            gbDevice.setLatitude(latitude);
            changed = true;
        }

        if (changed) {
            SPUtil.setSipConfig(sipConfig);
        }

        return changed;
    }

    /**
     * 初始化视频编码器支持列表
     */
    private void initVideoEncodecSpinner() {
        mVideoEncodec = SPUtil.getVideoEncodec(mContext);
        List<String> mListEncodec = SPUtil.getVideoEncodecs();
        int position = mListEncodec.indexOf(mVideoEncodec);

        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.spinner_common_item, mListEncodec);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mBinding.spnVencodec.setAdapter(adapter);
        mBinding.spnVencodec.setSelection(position, false);
        mBinding.spnVencodec.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String encodec = mListEncodec.get(position);
                if (!mVideoEncodec.equals(encodec)) {
                    mVideoEncodec = encodec;
                    SPUtil.setVideoEncodec(mContext, mVideoEncodec);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    /**
     * 初始化摄像头支持的帧率下拉控件的列表
     */
    private void initFramerateSpinner() {
        mFramerate = SPUtil.getFramerate(mContext);
        List<Integer> mListFramerate = SPUtil.getCameraFramerate(mContext);
        if (mListFramerate.isEmpty()) {
            return;
        }
        boolean supportedDefault = mListFramerate.contains(mFramerate);
        if (!supportedDefault) {
            mFramerate = mListFramerate.get(0);
            SPUtil.setFramerate(mContext, mFramerate);
        }
        int position = mListFramerate.indexOf(mFramerate);

        ArrayAdapter<Integer> adapter = new ArrayAdapter<>(this, R.layout.spinner_common_item, mListFramerate);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mBinding.spnFramerate.setAdapter(adapter);
        mBinding.spnFramerate.setSelection(position, false);
        mBinding.spnFramerate.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Integer framerate = mListFramerate.get(position);
                if (mFramerate != framerate) {
                    mFramerate = framerate;
                    SPUtil.setFramerate(mContext, mFramerate);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    /**
     * 初始化视频编码支持的profile列表
     */
    private void initVideoProfileSpinner() {
        mVideoProfile = SPUtil.getVideoProfile(mContext);
        List<String> mListProfile = SPUtil.getVideoProfiles();
        int position = mListProfile.indexOf(mVideoProfile);

        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.spinner_common_item, mListProfile);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mBinding.spnVprofile.setAdapter(adapter);
        mBinding.spnVprofile.setSelection(position, false);
        mBinding.spnVprofile.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String profile = mListProfile.get(position);
                if (!mVideoProfile.equals(profile)) {
                    mVideoProfile = profile;
                    SPUtil.setVideoProfile(mContext, mVideoProfile);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    /**
     * 初始化视频编码码率控制支持列表
     */
    private void initVideoBitrateModeSpinner() {
        mVideoBitrateMode = SPUtil.getBitrateMode(mContext);
        List<String> mListMode = SPUtil.getVideoBitrateModes();
        int position = mListMode.indexOf(mVideoBitrateMode);

        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.spinner_common_item, mListMode);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mBinding.spnVbitrateMode.setAdapter(adapter);
        mBinding.spnVbitrateMode.setSelection(position, false);
        mBinding.spnVbitrateMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String mode = mListMode.get(position);
                if (!mVideoBitrateMode.equals(mode)) {
                    mVideoBitrateMode = mode;
                    SPUtil.setBitrateMode(mContext, mVideoBitrateMode);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    /**
     * 初始化字符集支持列表
     */
    private void initCharsetSpinner() {
        mCharset = mSipConfig.getGbDevice().getCharset();
        List<GBDevice.Charset> mListCharset = new ArrayList<>();
        mListCharset.add(GBDevice.Charset.GB2312);
        mListCharset.add(GBDevice.Charset.UTF8);
        int position = mListCharset.indexOf(mCharset);

        ArrayAdapter<GBDevice.Charset> adapter = new ArrayAdapter<>(this, R.layout.spinner_common_item, mListCharset);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mBinding.spnCharset.setAdapter(adapter);
        mBinding.spnCharset.setSelection(position, false);
        mBinding.spnCharset.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                GBDevice.Charset charset = mListCharset.get(position);
                if (!mCharset.equals(charset)) {
                    mCharset = charset;
                    mSipConfig.getGbDevice().setCharset(mCharset);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    /**
     * 初始化国密35114安全等级支持列表
     */
    private void initGmLevelSpinner() {
        mGmLevel = mSipConfig.getGbDevice().getGmLevel();
        List<GBDevice.GmLevel> mListGmLevel = new ArrayList<>();
        mListGmLevel.add(GBDevice.GmLevel.N);
        mListGmLevel.add(GBDevice.GmLevel.A);
        mListGmLevel.add(GBDevice.GmLevel.B);
        mListGmLevel.add(GBDevice.GmLevel.C);
        int position = mListGmLevel.indexOf(mGmLevel);

        ArrayAdapter<GBDevice.GmLevel> adapter = new ArrayAdapter<>(this, R.layout.spinner_common_item, mListGmLevel);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mBinding.spnGmLevel.setAdapter(adapter);
        mBinding.spnGmLevel.setSelection(position, false);
        mBinding.spnGmLevel.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                GBDevice.GmLevel gmLevel = mListGmLevel.get(position);
                if (!mGmLevel.equals(gmLevel)) {
                    mGmLevel = gmLevel;
                    mSipConfig.getGbDevice().setGmLevel(mGmLevel);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }
}
