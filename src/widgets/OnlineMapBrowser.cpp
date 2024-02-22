#include <iostream>

#include <imgui.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

#include "include/OnlineMapBrowser.h"
#include "json.hpp"

#include "OnlineMapBrowser.h"

#include "bzfio.h"

const std::string archiveroot = "https://bz-next.github.io/maparchive/";

OnlineMapBrowser::OnMapDownloadComplete OnlineMapBrowser::_cb = NULL;
CachedResource* OnlineMapBrowser::mapRsc = NULL;

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapInfo, name, file, license);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapList, maps);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapIndex, index);

OnlineMapBrowser::OnlineMapBrowser() : mapIndex(archiveroot + "maps.json")
{
    // Eh whatever. Is this class a singleton or not? Who cares! :)
    // Callbacks are annoying
    mapRsc = NULL;
}

void OnlineMapBrowser::draw(const char* title, bool* p_open)
{
    maybeLoadFetchedData();
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, p_open);
    ImGui::Text("Browse archived maps by author:");
    ImGui::Separator();
    ImGui::BeginChild("MapList");
    for (const auto& e: _mi.index) {
        std::string authlabel = "Author: " + e.first;
        if (ImGui::TreeNode(authlabel.c_str())) { // Author
            
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
            for (const auto& m: e.second.maps) {
                ImGui::Text("Name:    %s", m.name.c_str());
                ImGui::Text("License: %s", m.license.c_str());
                std::string label = "Load " + m.file;
                if (ImGui::Button(label.c_str())) {
                    if (mapRsc) delete mapRsc;
                    mapRsc = new CachedResource(archiveroot + m.file);
                    mapRsc->setOnCompleteCallback([](const CachedResource& rsc) {
                        if (!rsc.isError())
                            if (OnlineMapBrowser::_cb)
                                OnlineMapBrowser::_cb(rsc);
                    });
                    mapRsc->fetch();
                }
                ImGui::Separator();
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
        try {
            mapIndexJSON = json::parse(fetchedData);
        } catch (json::parse_error &e) {
            logDebugMessage(1, "JSON parse failed.");
        }
        try {
            _mi = mapIndexJSON.template get<MapIndex>();
        } catch (json::type_error &e) {
            logDebugMessage(1, "JSON interpretation failed.");
        }
    }
}
