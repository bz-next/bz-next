#ifndef OBSTACLEBROWSER_H
#define OBSTACLEBROWSER_H

class ObstacleBrowser {
    public:
    ObstacleBrowser();
    void draw(const char* title, bool* p_open);
    private:
    int itemCurrent;
};

#endif