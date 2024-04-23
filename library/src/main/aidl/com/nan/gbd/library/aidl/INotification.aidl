// INotification.aidl
package com.nan.gbd.library.aidl;

// Declare any non-default types here with import statements

interface INotification {
    void onError(int error);
    void onTokenWillExpire();
}
