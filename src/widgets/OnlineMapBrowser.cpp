#include <imgui.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

#include "json.hpp"

#include "OnlineMapBrowser.h"

static std::string jsontest = R"TEST(
    {
        "index": {
            "ahs3": {
                "maps": [
                    {
                        "name": "INCOMING!",
                        "file": "ahs3_INCOMING.bzw",
                        "license": "CC BY 4.0"
                    },
                    {
                        "name": "Ironside Battlefield",
                        "file": "ahs3_Ironside_Battlefield.bzw",
                        "license": "CC BY 4.0"
                    },
                    {
                        "name": "Paradise Valley",
                        "file": "ahs3_Paradise_Valley.bzw",
                        "license": "CC BY 4.0"
                    },
                    {
                        "name": "XUG",
                        "file": "ahs3_XUG_FFA.bzw",
                        "license": "CC BY 4.0"
                    }
                ]
            },
            "Ian": {
                "maps": [
                    {
                        "name": "Fairground",
                        "file": "ian_fairground.bzw",
                        "license": "Open / unspecified"
                    }
                ]
            },
            "DucatiWannabe": {
                "maps": [
                    {
                        "name": "Missile War 2.3",
                        "file": "dw_missilewar2.3.bzw",
                        "license": "Ask author to host"
                    },
                    {
                        "name": "Missile War 3",
                        "file": "dw_missilewar3.bzw",
                        "license": "Ask author to host"
                    }
                ]
            }
        }
    }
)TEST";

using json = nlohmann::json;

/*void from_json(const json& j, MapInfo& mi)
{
    j.at("name").get_to(mi.name);
    j.at("file").get_to(mi.file);
    j.at("license").get_to(mi.license);
}

void from_json(const json& j, AuthorInfo& ai)
{
    j.at("name")
}*/

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapInfo, name, file, license);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapList, maps);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapIndex, index);

OnlineMapBrowser::OnlineMapBrowser() : mapIndex("https://bz-next.github.io/index.html")
{
}

void OnlineMapBrowser::draw(const char* title, bool* p_open)
{
    maybeLoadFetchedData();
    ImGui::Begin(title, p_open);
    ImGui::BeginChild("MapList");
    for (const auto& e: _mi.index) {
        if (ImGui::TreeNode(e.first.c_str())) { // Author
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
            for (const auto& m: e.second.maps) {
                ImVec2 ws = ImGui::GetContentRegionAvail();
                std::string label = m.name + " [License: " + m.license + "]";
                if (ImGui::Button(label.c_str(), {ws.x, 0})) {}
            }
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

void OnlineMapBrowser::maybeLoadFetchedData()
{
    if (!mapIndex.isComplete() && !mapIndex.isInProgress() && !mapIndex.isError()) {
        mapIndex.fetch();
        return;
    }
    if (fetchedData == "" && mapIndex.isComplete() && !mapIndex.isError()) {
        auto& data = mapIndex.getData();
        fetchedData = std::string(data.begin(), data.end());
        mapIndexJSON = json::parse(jsontest);
        _mi = mapIndexJSON.template get<MapIndex>();
    }
}