//
// Created by nanuns on 2021/2/25 0025.
//

#ifndef NANGBD_OSD_H
#define NANGBD_OSD_H

#include <string>
#include <vector>

using namespace std;

struct osd_t {
    string text;
    int x;
    int y;
    int date_len;
};

struct osd_info_t {
    bool enable = false;
    int char_width = 0;
    int char_height = 0;
    vector<osd_t> osds;
    vector<string> chars;
};

static int get_index(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c == '-')
        return 10;
    else if (c == ' ')
        return 11;
    else if (c == ':')
        return 12;
    else if (c == 24180) //年
        return 13;
    else if (c == 26376) //'月'
        return 14;
    else if (c == 26085) //'日'
        return 15;
    else if (c == '_')
        return 16;
    else if (c >= 'a' && c <= 'z')
        return 17 + c - 'a';
    else if (c >= 'A' && c <= 'Z')
        return 17 + 26 + c - 'A';

    return 11;
}

#endif // NANGBD_OSD_H
