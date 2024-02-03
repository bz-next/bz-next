/* Based on code from the ImGui Interactive Manual, with the following license: */
/*
MIT License
Copyright (c) 2024 Pascal Thomet
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ImGuiANSITextRenderer.h"
#include "BZScoreboard.h"

/* common implementation headers */
#include "Bundle.h"
#include "Team.h"
#include "FontManager.h"
#include "BZDBCache.h"
#include "OpenGLGState.h"
#include "TextUtils.h"
#include "TimeKeeper.h"

/* local implementation headers */
#include "LocalPlayer.h"
#include "World.h"
#include "sound.h"
#include <imgui.h>

// because of the 'player' crap, we can't  #include "Roaming.h"  easily
extern Player* getRoamTargetTank();

#define DEBUG_SHOWRATIOS 1

std::string BZScoreboard::scoreSpacingLabel("88% 8888 888-888 [88]");
std::string BZScoreboard::scoreLabel("Score");
std::string BZScoreboard::killSpacingLabel("888~888 Hunt->");
std::string BZScoreboard::killLabel(" Kills");
std::string BZScoreboard::teamScoreSpacingLabel("88 (888-888) 88");
std::string BZScoreboard::teamCountSpacingLabel("888");
std::string BZScoreboard::playerLabel("Player");

// NOTE: order of sort labels must match SORT_ consts
const char* BZScoreboard::sortLabels[] =
{
    "[Score]",
    "[Normalized Score]",
    "[Reverse Score]",
    "[Callsign]",
    "[Team Kills]",
    "[TK ratio]",
    "[Team]",
    "[1on1]",
    NULL
};

int BZScoreboard::sortMode = 0;

bool BZScoreboard::alwaysShowTeamScore = 0;

BZScoreboard::BZScoreboard() :
    dim(false),
    huntPosition(0),
    huntSelectEvent(false),
    huntPositionEvent(0),
    huntState(HUNT_NONE),
    huntAddMode(false),
    roaming(false),
    minorFontFace(),
    minorFontSize(),
    labelsFontFace(),
    labelsFontSize(),
    scoreLabelWidth(),
    killsLabelWidth(),
    teamScoreLabelWidth(),
    teamCountLabelWidth(),
    huntArrowWidth(),
    huntPlusesWidth(),
    huntedArrowWidth(),
    tkWarnRatio(),
    numHunted(0) {

    // initialize message color (white)
    messageColor[0] = 1.0f;
    messageColor[1] = 1.0f;
    messageColor[2] = 1.0f;
    sortMode = BZDB.getIntClamped("scoreboardSort", 0, SORT_MAXNUM);
    alwaysShowTeamScore = (BZDB.getIntClamped("alwaysShowTeamScore", 0, 1) != 0);
}

BZScoreboard::~BZScoreboard() {
}

const char  **BZScoreboard::getSortLabels ()
{
    return sortLabels;
}


void BZScoreboard::setSort (int _sortby)
{
    sortMode = _sortby;
    BZDB.setInt ("scoreboardSort", sortMode);
}

int BZScoreboard::getSort ()
{
    return sortMode;
}

void    BZScoreboard::setAlwaysTeamScore (bool _onoff)
{
    alwaysShowTeamScore = _onoff;
    BZDB.set ("alwaysShowTeamScores", _onoff ? "1" : "0");
}

bool    BZScoreboard::getAlwaysTeamScore ()
{
    return alwaysShowTeamScore;
}

void    BZScoreboard::exitSelectState (void)
{
    //playLocalSound(SFX_HUNT_SELECT);
    if (numHunted > 0)
        setHuntState(HUNT_ENABLED);
    else
        setHuntState(HUNT_NONE);
}

int BZScoreboard::teamScoreCompare(const void* _c, const void* _d)
{
    Team* c = World::getWorld()->getTeams()+*(const int*)_c;
    Team* d = World::getWorld()->getTeams()+*(const int*)_d;
    return (d->getWins()-d->getLosses()) - (c->getWins()-c->getLosses());
}

// invoked by playing.cxx when 'prev' is pressed
void BZScoreboard::setHuntPrevEvent()
{
    huntPositionEvent = -1;
    --huntPosition;
}

// invoked by playing.cxx when 'next' is pressed
void BZScoreboard::setHuntNextEvent()
{
    huntPositionEvent = 1;
    ++huntPosition;
}

// invoked by playing.cxx when select (fire) is pressed
void BZScoreboard::setHuntSelectEvent ()
{
    huntSelectEvent = true;
}

// invoked by clientCommands.cxx when '7' or 'U' is pressed
void BZScoreboard::huntKeyEvent (bool isAdd)
{
    if (getHuntState() == HUNT_ENABLED)
    {
        if (isAdd)
        {
            setHuntState(HUNT_SELECTING);
            playLocalSound(SFX_HUNT_SELECT);
        }
        else
        {
            setHuntState(HUNT_NONE);
            playLocalSound(SFX_HUNT);
        }
        huntAddMode = isAdd;

    }
    else if (getHuntState() == HUNT_SELECTING)
        exitSelectState ();

    else
    {
        setHuntState(HUNT_SELECTING);
        playLocalSound(SFX_HUNT_SELECT);
        huntAddMode = isAdd;
        if (!BZDB.isTrue("displayScore"))
            BZDB.set("displayScore", "1");
    }
}


void BZScoreboard::setHuntState (int _huntState)
{
    if (huntState == _huntState)
        return;
    if (_huntState != HUNT_SELECTING)
        huntAddMode = false;
    if (_huntState==HUNT_NONE)
        clearHuntedTanks();
    else if (_huntState==HUNT_SELECTING)
        huntPosition = 0;
    huntState = _huntState;
}


int BZScoreboard::getHuntState() const
{
    return huntState;
}


// invoked when joining a server
void BZScoreboard::huntReset()
{
    huntState = HUNT_NONE;
    numHunted = 0;
}

void BZScoreboard::clearHuntedTanks ()
{
    World *world = World::getWorld();
    if (!world)
        return;
    const int curMaxPlayers = world->getCurMaxPlayers();
    Player *p;
    for (int i=0; i<curMaxPlayers; i++)
    {
        if ((p = world->getPlayer(i)))
            p->setHunted (false);
    }
    numHunted = 0;
}

void BZScoreboard::stringAppendNormalized (std::string *s, float n)
{
    char fmtbuf[10];
    sprintf (fmtbuf, "  [%4.2f]", n);
    *s += fmtbuf;
}

// get current 'leader' (NULL if no player)
Player*   BZScoreboard::getLeader(std::string *label)
{
    int sortType=sortMode;

    if (sortMode==SORT_CALLSIGN || sortMode==SORT_MYRATIO || sortMode==SORT_TEAM)
        sortType = SORT_SCORE;
    if (label != NULL)
    {
        if (sortMode==SORT_TKS)
            *label = "TK Leader ";
        else if (sortMode==SORT_TKRATIO)
            *label = "TK Ratio Leader ";
        else
            *label = "Leader ";
    }

    Player** list = newSortedList (sortType, true);

    Player* top;
    if (!list)
        top = NULL;
    else
        top = list[0];

    delete[] list;

    if (top==NULL || top->getTeam()==ObserverTeam)
        return NULL;
    return top;
}


/************************ Sort logic follows .... **************************/

struct st_playersort
{
    Player *player;
    int i1;
    int i2;
    const char *cp;
};
typedef struct st_playersort sortEntry;


int       BZScoreboard::sortCompareCp(const void* _a, const void* _b)
{
    const sortEntry *a = (const sortEntry *)_a;
    const sortEntry *b = (const sortEntry *)_b;
    return strcasecmp (a->cp, b->cp);
}


int       BZScoreboard::sortCompareI2(const void* _a, const void* _b)
{
    const sortEntry *a = (const sortEntry *)_a;
    const sortEntry *b = (const sortEntry *)_b;
    if (a->i1 != b->i1 )
        return b->i1 - a->i1;
    return b->i2 - a->i2;
}


// creates (allocates) a null-terminated array of Player*
Player **  BZScoreboard::newSortedList (int sortType, bool obsLast, int *_numPlayers)
{
    LocalPlayer *myTank = LocalPlayer::getMyTank();
    World *world = World::getWorld();

    if (!myTank || !world)
        return NULL;

    const int curMaxPlayers = world->getCurMaxPlayers() +1;
    int i,j;
    int numPlayers=0;
    int numObs=0;
    Player* p;
    sortEntry* sorter = new sortEntry [curMaxPlayers];

    // fill the array with remote players
    for (i=0; i<curMaxPlayers-1; i++)
    {
        if ((p = world->getPlayer(i)))
        {
            if (obsLast && p->getTeam()==ObserverTeam)
                sorter[curMaxPlayers - (++numObs)].player = p;
            else
                sorter[numPlayers++].player = p;
        }
    }
    // add my tank
    if (obsLast && myTank->getTeam()==ObserverTeam)
        sorter[curMaxPlayers - (++numObs)].player = myTank;
    else
        sorter[numPlayers++].player = myTank;

    // sort players ...
    if (numPlayers > 0)
    {
        for (i=0; i<numPlayers; i++)
        {
            p = sorter[i].player;
            switch (sortType)
            {
            case SORT_TKS:
                sorter[i].i1 = p->getTeamKills();
                sorter[i].i2 = 0 - (int)(p->getNormalizedScore() * 100000);
                break;
            case SORT_TKRATIO:
                sorter[i].i1 = (int)(p->getTKRatio() * 1000);
                sorter[i].i2 = 0 - (int)(p->getNormalizedScore() * 100000);
                break;
            case SORT_TEAM:
                sorter[i].i1 = p->getTeam();
                sorter[i].i2 = (int)(p->getNormalizedScore() * 100000);
                break;
            case SORT_MYRATIO:
                if (p == myTank)
                    sorter[i].i1 = -100001;
                else
                    sorter[i].i1 = 0 - (int)(p->getLocalNormalizedScore() * 100000);
                sorter[i].i2 = (int)(p->getNormalizedScore() * 100000);
                break;
            case SORT_NORMALIZED:
                sorter[i].i1 = (int)(p->getNormalizedScore() * 100000);
                sorter[i].i2 = 0;
                break;
            case SORT_CALLSIGN:
                sorter[i].cp = p->getCallSign();
                break;
            default:
                if (world->allowRabbit())
                    sorter[i].i1 = p->getRabbitScore();
                else
                    sorter[i].i1 = p->getScore();
                sorter[i].i2 = 0;
                if (sortType == SORT_REVERSE)
                    sorter[i].i1 *= -1;
            }
        }
        if (sortType == SORT_CALLSIGN)
            qsort (sorter, numPlayers, sizeof(sortEntry), sortCompareCp);
        else
            qsort (sorter, numPlayers, sizeof(sortEntry), sortCompareI2);
    }

    // TODO: Sort obs here (by time joined, when that info is available)

    Player** players = new Player *[numPlayers + numObs + 1];
    for (i=0; i<numPlayers; i++)
        players[i] = sorter[i].player;
    for (j=curMaxPlayers-numObs; j<curMaxPlayers; j++)
        players[i++] = sorter[j].player;
    players[i] = NULL;

    if (_numPlayers != NULL)
        *_numPlayers = numPlayers;

    delete[] sorter;
    return players;
}


void BZScoreboard::getPlayerList(std::vector<Player*>& players)
{
    players.clear();

    int playerCount;
    Player** pList = newSortedList(getSort(), true, &playerCount);
    if (pList == NULL)
        return;
    for (int i = 0; i < playerCount; i++)
    {
        Player* p = pList[i];
        if (p && (p->getTeam() != ObserverTeam))
            players.push_back(p);
    }
    delete[] pList;
}

ImU32 msgColorToImGuiColor(const GLfloat *mc) {
    ImU32 r = (ImU32)(mc[0]*0xff);
    ImU32 g = (ImU32)(mc[1]*0xff);
    ImU32 b = (ImU32)(mc[2]*0xff);
    return r | (g<<8) | (b<<16) | (0xff<<24);
}

void BZScoreboard::drawPlayerScore(const Player* player, int mottoLen, bool huntCursor)
{
    // score
    char score[40], kills[40];

    bool highlightTKratio = false;
    if (tkWarnRatio > 0.0)
    {
        if (((player->getWins() > 0) && (player->getTKRatio() > tkWarnRatio)) ||
                ((player->getWins() == 0) && (player->getTeamKills() >= 3)))
            highlightTKratio = true;
    }

    if (World::getWorld()->allowRabbit())
    {
        sprintf(score, "%2d%% %4d %3d-%-3d%s[%2d]", player->getRabbitScore(),
                player->getScore(), player->getWins(), player->getLosses(),
                highlightTKratio ? ColorStrings[CyanColor].c_str() : "",
                player->getTeamKills());
    }
    else if (World::getWorld()->allowTeams())
    {
        sprintf(score, "%4d %4d-%-4d%s[%2d]", player->getScore(),
                player->getWins(), player->getLosses(),
                highlightTKratio ? ColorStrings[CyanColor].c_str() : "",
                player->getTeamKills());
    }
    else
    {
        sprintf(score, "%4d %4d-%-4d%s", player->getScore(),
                player->getWins(), player->getLosses(),
                highlightTKratio ? ColorStrings[CyanColor].c_str() : "");
    }

    // kills
    if (LocalPlayer::getMyTank() != player)
        sprintf(kills, "%3d~%-3d", player->getLocalWins(), player->getLocalLosses());
    else
        sprintf(kills, "%4d", player->getSelfKills());


    // team color
    TeamColor teamIndex = player->getTeam();
    std::string teamColor;
    if (player->getId() < 200)
        teamColor = Team::getAnsiCode(teamIndex);
    else
    {
        teamColor = ColorStrings[CyanColor];    // replay observers
    }

    // authentication status
    std::string statusInfo;
    if (BZDBCache::colorful)
        statusInfo += ColorStrings[CyanColor];
    else
        statusInfo += teamColor;
    if (player->isAdmin())
        statusInfo += '@';
    else if (player->isVerified())
        statusInfo += '+';
    else if (player->isRegistered())
        statusInfo += '-';
    else
    {
        statusInfo = " ";    // space placeholder
    }

    std::string playerInfo;
    // team color
    playerInfo += teamColor;
    // Slot number only for admins (playerList perm check, in case they have
    // hideAdmin)
    LocalPlayer* localPlayer = LocalPlayer::getMyTank();
    if (localPlayer->isAdmin() || localPlayer->hasPlayerList())
    {
        char slot[10];
        sprintf(slot, "%3d",player->getId());
        playerInfo += slot;
        playerInfo += " - ";
    }

    if (roaming && BZDB.isTrue("showVelocities"))
    {
        float vel[3] = {0};
        memcpy(vel,player->getVelocity(),sizeof(float)*3);

        float linSpeed = sqrt(vel[0]*vel[0]+vel[1]*vel[1]);

        float badFactor = 1.5f;
        if (linSpeed > player->getMaxSpeed()*badFactor)
            playerInfo += ColorStrings[RedColor];
        if (linSpeed > player->getMaxSpeed())
            playerInfo += ColorStrings[YellowColor];
        else if (linSpeed < 0.0001f)
            playerInfo += ColorStrings[GreyColor];
        else
            playerInfo += ColorStrings[WhiteColor];
        playerInfo += TextUtils::format("%5.2f ",linSpeed);
        playerInfo += teamColor;
    }

    // callsign
    playerInfo += player->getCallSign();

    // motto in parentheses
    if (player->getMotto()[0] != '\0' && mottoLen>0)
    {
        playerInfo += " (";
        playerInfo += TextUtils::str_trunc_continued (player->getMotto(), mottoLen);
        playerInfo += ")";
    }
    // carried flag
    bool coloredFlag = false;
    FlagType* flagd = player->getFlag();
    if (flagd != Flags::Null)
    {
        // color special flags
        if (BZDBCache::colorful)
        {
            if ((flagd == Flags::ShockWave)
                    ||  (flagd == Flags::Genocide)
                    ||  (flagd == Flags::Laser)
                    ||  (flagd == Flags::GuidedMissile))
                playerInfo += ColorStrings[WhiteColor];
            else if (flagd->flagTeam != NoTeam)
            {
                // use team color for team flags
                playerInfo += Team::getAnsiCode(flagd->flagTeam);
            }
            coloredFlag = true;
        }
        playerInfo += "/";
        playerInfo += (flagd->endurance == FlagNormal ? flagd->flagName : flagd->flagAbbv);
        // back to original color
        if (coloredFlag)
            playerInfo += teamColor;
    }

    // status
    if (player->isPaused())
        playerInfo += "[p]";
    else if (player->isNotResponding())
        playerInfo += "[nr]";
    else if (player->isAutoPilot())
        playerInfo += "[auto]";

#if DEBUG_SHOWRATIOS
    if (player->getTeam() != ObserverTeam)
    {
        if (sortMode == SORT_NORMALIZED)
            stringAppendNormalized (&playerInfo, player->getNormalizedScore());
        else if (sortMode == SORT_MYRATIO && LocalPlayer::getMyTank() != player)
            stringAppendNormalized (&playerInfo, player->getLocalNormalizedScore());
        else if (sortMode == SORT_TKRATIO)
            stringAppendNormalized (&playerInfo, player->getTKRatio());
    }
#endif

    /*if (player == getRoamTargetTank())
    {
        const float w = fm.getStrLength(minorFontFace, minorFontSize, playerInfo);
        const float h = fm.getStrHeight(minorFontFace, minorFontSize, playerInfo);
        drawRoamTarget(x3, y, x3 + w, y + h);
    }*/

    // draw
    if (player->getTeam() != ObserverTeam)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, msgColorToImGuiColor(Team::getTankColor(teamIndex)));
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(score);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(kills);
        ImGui::PopStyleColor();
    }
    ImGui::TableSetColumnIndex(2);
    std::string playerAndStatus = statusInfo + playerInfo;
    ImGui::TextAnsiUnformatted(playerAndStatus.c_str(), playerAndStatus.c_str() + playerAndStatus.size());

    // draw huntEnabled status
    /*if (player->isHunted())
    {
        std::string huntStr = ColorStrings[WhiteColor];
        huntStr += "Hunt->";
        fm.drawString(xs - huntedArrowWidth, y, 0, minorFontFace, minorFontSize,
                      huntStr.c_str());
    }
    else if (huntCursor && !huntAddMode)
    {
        std::string huntStr = ColorStrings[WhiteColor];
        huntStr += ColorStrings[PulsatingColor];
        huntStr += "->";
        fm.drawString(xs - huntArrowWidth, y, 0, minorFontFace, minorFontSize,
                      huntStr.c_str());
    }
    if (huntCursor && huntAddMode)
    {
        std::string huntStr = ColorStrings[WhiteColor];
        huntStr += ColorStrings[PulsatingColor];
        huntStr += "@>";
        fm.drawString(xs - huntPlusesWidth, y, 0, minorFontFace, minorFontSize,
                      huntStr.c_str());
    }*/
}

void BZScoreboard::sortTeamScores()
{
    teamCount = 0;
    memset(sortedTeamsForTeamScore, 0, sizeof(sortedTeamsForTeamScore));

    if (World::getWorld()->allowRabbit())
        return;

    for (int i = RedTeam; i < NumTeams; i++)
    {
        if (!Team::isColorTeam(TeamColor(i))) continue;
        const Team* team = World::getWorld()->getTeams() + i;
        if (team->size == 0) continue;
        sortedTeamsForTeamScore[teamCount++] = i;
    }
    qsort(sortedTeamsForTeamScore, teamCount, sizeof(int), teamScoreCompare);
}

void BZScoreboard::renderTeamScores (int ind)
{
    // print teams sorted by score

    if (World::getWorld()->allowRabbit())
        return;
    
    if (ind >= NumTeams || ind >= teamCount) return;

    char score[44];
    Team& team = World::getWorld()->getTeam(sortedTeamsForTeamScore[ind]);
    sprintf(score, "%3d (%3d-%-3d) %3d", team.getWins() - team.getLosses(), team.getWins(), team.getLosses(), team.size);
    ImGui::TableSetColumnIndex(3);
    ImGui::PushStyleColor(ImGuiCol_Text, msgColorToImGuiColor(Team::getTankColor((TeamColor)sortedTeamsForTeamScore[ind])));
    ImGui::Text(score);
    ImGui::PopStyleColor();
}

void BZScoreboard::draw(const char* title, bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    int numPlayers;
    int mottoLen;
    Player** players;
    Player*  player;
    if ( (players = newSortedList (sortMode, true, &numPlayers)) != NULL) {
        LocalPlayer* myTank = LocalPlayer::getMyTank();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, -1));
        ImGui::PushStyleColor(ImGuiCol_Text, msgColorToImGuiColor(messageColor));
        ImGui::BeginTable("scoreboard", 4);
        ImGui::TableSetupColumn(scoreLabel.c_str());
        ImGui::TableSetupColumn(killLabel.c_str());
        ImGui::TableSetupColumn(playerLabel.c_str());
        ImGui::TableSetupColumn("Team Score");
        // Print hunt *SEL* column label if we're doing it that way
        //if (huntState == HUNT_SELECTING)
        //{ ... }
        // more hunt logic here
        mottoLen = BZDB.getIntClamped ("mottoDispLen", 0, 128);
        huntSelectEvent = false;
        huntPositionEvent = 0;
        numHunted = 0;

        const int maxLines = BZDB.evalInt("maxScoreboardLines");
        int lines = 0;
        int hiddenLines = 0;

        ImGui::TableHeadersRow();

        sortTeamScores();

        int i = 0;
        while ((player = players[i]) != NULL) {
            ImGui::TableNextRow();

            drawPlayerScore(player, mottoLen, false);
            if (World::getWorld()->allowTeams()) {
                renderTeamScores(i);
            }
            ++i;
        }
        ImGui::EndTable();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    ImGui::End();
}
