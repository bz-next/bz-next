#ifndef OBSTACLEBROWSER_H
#define OBSTACLECROWSER_H

class ObstacleBrowser {
    public:
    ObstacleBrowser();
    void draw(const char* title, bool* p_open);
    private:
    int itemCurrent;
};

#endif