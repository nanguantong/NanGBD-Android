// IScreenSharing.aidl
package com.nan.gbd.library.aidl;

import com.nan.gbd.library.aidl.INotification;

// Declare any non-default types here with import statements

interface IScreenSharing {
    void registerCallback(INotification callback);
    void unregisterCallback(INotification callback);
    void startShare();
    void stopShare();
    void renewToken(String token);
}
