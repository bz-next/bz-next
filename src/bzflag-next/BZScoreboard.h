#ifndef BZSCOREBOARD_H
#define BZSCOREBOARD_H



#include <functional>
#include <cstring>
#include <cctype>
#include <Magnum/ImGuiIntegration/Context.hpp>

#include "Player.h"



class BZScoreboard {
    using CommandCallback = std::function<void(const char*)>;
    public:
        BZScoreboard();
        ~BZScoreboard();

        void draw(const char* title, bool* p_open);

        static Player* getLeader(std::string *label = NULL);

        static const int HUNT_NONE = 0;
        static const int HUNT_SELECTING = 1;
        static const int HUNT_ENABLED = 2;
        void    setHuntState(int _state);
        int         getHuntState() const;
        void      setHuntNextEvent ();    // invoked when 'down' button pressed
        void      setHuntPrevEvent ();    // invoked when 'up' button pressed
        void      setHuntSelectEvent ();      // invoked when 'fire' button pressed
        void    huntKeyEvent (bool isAdd);  // invoked when '7' or 'U' is pressed
        void    clearHuntedTanks ();
        void    huntReset ();        // invoked when joining a server

        static void    setAlwaysTeamScore (bool onoff);
        static bool    getAlwaysTeamScore ();

        static void    setSort (int _sortby);
        static int     getSort ();
        static const char **getSortLabels();
        static const int SORT_SCORE = 0;
        static const int SORT_NORMALIZED = 1;
        static const int SORT_REVERSE = 2;
        static const int SORT_CALLSIGN = 3;
        static const int SORT_TKS = 4;
        static const int SORT_TKRATIO = 5;
        static const int SORT_TEAM = 6;
        static const int SORT_MYRATIO = 7;
        static const int SORT_MAXNUM = SORT_MYRATIO;

        void setRoaming ( bool val )
        {
            roaming = val;
        }

        // does not include observers
        static void getPlayerList(std::vector<Player*>& players);
    protected:
        void renderTeamScores (float y, float x, float dy);
        void renderScoreboard();
        void drawRoamTarget(float x0, float y0, float x1, float y1);
        void drawPlayerScore(const Player*, int mottoLen, bool huntInd);
        void renderTeamScores (int ind);
        static const char *sortLabels[SORT_MAXNUM+2];
        static int sortMode;
        static bool alwaysShowTeamScore;
        void   stringAppendNormalized (std::string *s, float n);

    private:
        void setMinorFontSize(float height);
        void setLabelsFontSize(float height);
        static int teamScoreCompare(const void* _a, const void* _b);
        static int sortCompareCp(const void* _a, const void* _b);
        static int sortCompareI2(const void* _a, const void* _b);
        static Player** newSortedList(int sortType, bool obsLast, int *_numPlayers=NULL);
        void exitSelectState (void);
        void sortTeamScores();

    private:
        bool dim;
        int huntPosition;
        bool huntSelectEvent;
        int huntPositionEvent;
        int huntState;
        bool huntAddMode;    // valid only if state == SELECTING
        float teamScoreYVal;
        bool roaming;

        GLfloat messageColor[3];
        int minorFontFace;
        float minorFontSize;
        int labelsFontFace;
        float labelsFontSize;

        float scoreLabelWidth;
        float killsLabelWidth;
        float teamScoreLabelWidth;
        float teamCountLabelWidth;
        float huntArrowWidth;
        float huntPlusesWidth;
        float huntedArrowWidth;
        float tkWarnRatio;

        int teamCount;
        int sortedTeamsForTeamScore[NumTeams];

        static std::string scoreSpacingLabel;
        static std::string scoreLabel;
        static std::string killSpacingLabel;
        static std::string killLabel;
        static std::string teamScoreSpacingLabel;
        static std::string playerLabel;
        static std::string teamCountSpacingLabel;
        int numHunted;
};

#endif
