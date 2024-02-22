#include <imgui.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

#include "OnlineMapBrowser.h"

OnlineMapBrowser::OnlineMapBrowser() : mapIndex("https://bz-next.github.io/index.html")
{
}

void OnlineMapBrowser::draw(const char* title, bool* p_open)
{
    maybeLoadFetchedData();
    ImGui::Begin(title, p_open);
    ImGui::TextWrapped(fetchedData.c_str());
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
    }
}