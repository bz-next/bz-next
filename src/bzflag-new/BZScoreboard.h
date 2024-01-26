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

        void clear();
        void add(const char* fmt, ...) IM_FMTARGS(2);
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
        void drawPlayerScore(const Player*,
                            float x1, float x2, float x3, float xs, float y,
                            int mottoLen, bool huntInd);
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

    private:
        ImVector<char*> items;

        static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
        static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
        static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
        static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

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

        static std::string scoreSpacingLabel;
        static std::string scoreLabel;
        static std::string killSpacingLabel;
        static std::string killLabel;
        static std::string teamScoreSpacingLabel;
        static std::string playerLabel;
        static std::string teamCountSpacingLabel;
        int numHunted;
};
