package com.nan.gbd.library.utils;

import com.squareup.otto.Bus;
import com.squareup.otto.ThreadEnforcer;

/**
 * @author NanGBD
 * @date 2020.1.1
 */
public class BusUtils {
    public static final Bus BUS = new Bus(ThreadEnforcer.ANY);
}
