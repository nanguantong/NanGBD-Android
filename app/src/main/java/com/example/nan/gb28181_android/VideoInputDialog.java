package com.example.nan.gb28181_android;

import android.app.Dialog;
import android.content.Context;
import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.example.nan.gb28181_android.utils.WindowUtil;
import com.nan.gbd.library.source.MediaSourceType;

/**
 * @author NanGBD
 * @date 2019.6.8
 */
public class VideoInputDialog extends Dialog implements View.OnClickListener {

    public interface OnSelectVideoInputListener {
        /**
         * Called when an external video input is selected
         * @param type MS_TYPE_CAMERA / MS_TYPE_SCREEN
         */
        void onVideoInputSelected(MediaSourceType type);
    }

    private View mReference;
    private ImageView mArrow;
    private OnSelectVideoInputListener mListener;

    VideoInputDialog(@NonNull Context context, View reference,
                     OnSelectVideoInputListener listener) {
        super(context, R.style.no_background_dialog);
        String[] text = context.getResources().getStringArray(R.array.video_input_array);
        mReference = reference;
        mListener = listener;

        setContentView(R.layout.choose_video_input_dialog);

        TextView textView = findViewById(R.id.input_screen_share);
        textView.setText(text[0]);
        textView.setOnClickListener(this);

        textView = findViewById(R.id.input_camera);
        textView.setText(text[1]);
        textView.setOnClickListener(this);

        mArrow = findViewById(R.id.video_input_dialog_arrow);
    }

    @Override
    public void onClick(View view) {
        MediaSourceType type;
        switch (view.getId()) {
            case R.id.input_screen_share:
                type = MediaSourceType.MS_TYPE_SCREEN;
                break;
            case R.id.input_camera:
                type = MediaSourceType.MS_TYPE_CAMERA;
                break;
            default: return;
        }
        if (mListener != null) {
            mListener.onVideoInputSelected(type);
        }
        dismiss();
    }

    @Override
    public void show() {
        doShow();
        super.show();
    }

    private void doShow() {
        WindowManager.LayoutParams params = getWindow().getAttributes();
        params.gravity = Gravity.START | Gravity.BOTTOM;

        int [] location = new int[2];
        mReference.getLocationOnScreen(location);
        params.x = location[0];
        DisplayMetrics metrics = new DisplayMetrics();
        getWindow().getWindowManager().getDefaultDisplay().getMetrics(metrics);
        Resources resources = getContext().getResources();
        params.width = resources.getDimensionPixelSize(R.dimen.video_input_dialog_width);
        params.height = resources.getDimensionPixelSize(R.dimen.video_input_dialog_height);
        int statusBarH = WindowUtil.getSystemStatusBarHeight(getContext());
        params.y = metrics.heightPixels - location[1] + statusBarH + 20;
        getWindow().setAttributes(params);

        LinearLayout.LayoutParams arrowParams = (LinearLayout.LayoutParams) mArrow.getLayoutParams();
        int refHalf = mReference.getMeasuredWidth() / 2;
        arrowParams.leftMargin += refHalf / 2;
        mArrow.setLayoutParams(arrowParams);
    }
}