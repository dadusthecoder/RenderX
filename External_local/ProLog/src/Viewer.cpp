// ============================================================================
// ProLog_ImGui.cpp  (DROP-IN FILE)
// Real-time profiling UI for your existing ProLogProfiler
// ============================================================================

#include "ProLog/ProLog.h"
#include <imgui.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <unordered_set>

namespace ProLog {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static double MicroToMs(long long us) {
    return double(us) / 1000.0;
}

// ---------------------------------------------------------------------------
// Internal state for ImGui panel
// ---------------------------------------------------------------------------

static bool g_Paused = false;
static bool g_ShowTimeline = true;
static bool g_ShowTopFunctions = true;
static bool g_ShowEventsTable = false;
static bool g_FilterByThread = false;

static char g_SearchBuffer[128] = "";
static std::thread::id g_SelectedThread;

static float g_TimelineScale = 1.0f;   // zoom
static float g_TimelineOffset = 0.0f;  // pan

// ---------------------------------------------------------------------------
// Collect recent events (thread-safe)
// ---------------------------------------------------------------------------

static void CollectRecentEvents(
    const ProfilerSession& session,
    std::vector<ProfileResult>& out,
    size_t maxCount = 3000)
{
    std::lock_guard<std::mutex> lock(session.m_Mutex);

    size_t take = std::min(maxCount, session.m_Buffer.size());
    out.reserve(take);

    for (size_t i = session.m_Buffer.size() - take; i < session.m_Buffer.size(); ++i)
        out.push_back(session.m_Buffer[i]);
}

// ---------------------------------------------------------------------------
// Collect statistics sorted by total duration
// ---------------------------------------------------------------------------

static void CollectSortedStatistics(
    const ProfilerSession& session,
    std::vector<ProfileStatistics>& out)
{
    std::lock_guard<std::mutex> lock(session.m_Mutex);

    out.reserve(session.m_Statistics.size());
    for (auto& kv : session.m_Statistics)
        out.push_back(kv.second);

    std::sort(out.begin(), out.end(),
        [](const ProfileStatistics& a, const ProfileStatistics& b)
        {
            return a.totalDuration > b.totalDuration;
        });
}

// ---------------------------------------------------------------------------
// Draw Frame-Time Graph
// ---------------------------------------------------------------------------

static void DrawFrameGraph(const ProfilerSession& session)
{
    std::vector<float> graph;

    {
        std::lock_guard<std::mutex> lock(session.m_Mutex);
        graph.reserve(session.m_Buffer.size());

        for (auto it = session.m_Buffer.rbegin(); it != session.m_Buffer.rend(); ++it)
        {
            if (it->depth == 0)
                graph.push_back((float)MicroToMs(it->duration));

            if (graph.size() >= 400) break;
        }
    }

    std::reverse(graph.begin(), graph.end());

    if (!graph.empty()) {
        float last = graph.back();
        ImGui::Text("Frame Time: %.3f ms", last);
        ImGui::PlotLines("##frame_plot", graph.data(), (int)graph.size(), 0, 0, 0.0f, 40.0f, ImVec2(0, 100));
    }
    else {
        ImGui::TextDisabled("No frame events yet.");
    }
}

// ---------------------------------------------------------------------------
// Draw Top Functions Table
// ---------------------------------------------------------------------------

static void DrawTopFunctions(const ProfilerSession& session)
{
    if (!g_ShowTopFunctions)
        return;

    std::vector<ProfileStatistics> stats;
    CollectSortedStatistics(session, stats);

    ImGui::SeparatorText("Top Functions");

    ImGui::BeginChild("topfuncs", ImVec2(0, 200), true);
    ImGui::Columns(5, "statscols");
    ImGui::Text("Function"); ImGui::NextColumn();
    ImGui::Text("Calls"); ImGui::NextColumn();
    ImGui::Text("Avg (ms)"); ImGui::NextColumn();
    ImGui::Text("Total (ms)"); ImGui::NextColumn();
    ImGui::Text("Min/Max"); ImGui::NextColumn();
    ImGui::Separator();

    int shown = 0;
    for (auto& s : stats)
    {
        if (shown++ >= 30) break; // limit rows

        if (strlen(g_SearchBuffer) > 0 &&
            s.functionName.find(g_SearchBuffer) == std::string::npos)
            continue;

        ImGui::Text("%s", s.functionName.c_str()); ImGui::NextColumn();
        ImGui::Text("%zu", s.callCount); ImGui::NextColumn();
        ImGui::Text("%.3f", MicroToMs(s.avgDuration * 1000)); ImGui::NextColumn();
        ImGui::Text("%.3f", MicroToMs(s.totalDuration)); ImGui::NextColumn();
        ImGui::Text("%.3f / %.3f",
            MicroToMs(s.minDuration),
            MicroToMs(s.maxDuration));
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
// Draw Timeline (Zoomable / Pannable)
// ---------------------------------------------------------------------------

static void DrawTimeline(const ProfilerSession& session)
{
    if (!g_ShowTimeline)
        return;

    ImGui::SeparatorText("Timeline (Interactive)");

    std::vector<ProfileResult> events;
    CollectRecentEvents(session, events, 2000);

    if (events.empty()) {
        ImGui::TextDisabled("No events yet.");
        return;
    }

    // get time range
    long long minT = events.front().start;
    long long maxT = events.front().start + events.front().duration;

    for (auto& e : events)
    {
        minT = std::min(minT, e.start);
        maxT = std::max(maxT, (long long)(e.start + e.duration));
    }

    double span = (maxT - minT) * g_TimelineScale;
    if (span <= 0.0) span = 1.0;

    // canvas
    ImVec2 size(ImGui::GetContentRegionAvail().x, 250);
    ImGui::BeginChild("timeline", size, true);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    // background
    dl->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
        IM_COL32(30, 30, 30, 255));

    // gather thread lanes
    std::unordered_map<std::thread::id, float> lanes;
    float nextLane = 5.0f;

    // draw events
    for (auto& e : events)
    {
        if (strlen(g_SearchBuffer) > 0 &&
            e.name.find(g_SearchBuffer) == std::string::npos)
            continue;

        if (g_FilterByThread && e.threadID != g_SelectedThread)
            continue;

        if (!lanes.count(e.threadID)) {
            lanes[e.threadID] = nextLane;
            nextLane += 18.0f;
        }

        float laneY = lanes[e.threadID];

        double normStart = (e.start - minT) / span;
        double normEnd   = (e.start + e.duration - minT) / span;

        float x0 = origin.x + float(normStart * size.x + g_TimelineOffset);
        float x1 = origin.x + float(normEnd   * size.x + g_TimelineOffset);

        if (x1 < origin.x || x0 > origin.x + size.x) continue;

        ImU32 col = IM_COL32(80 + e.depth * 10, 150 - e.depth * 10, 220 - e.depth * 5, 200);

        dl->AddRectFilled(ImVec2(x0, origin.y + laneY),
                          ImVec2(x1, origin.y + laneY + 12),
                          col);

        if (x1 - x0 > 40.0f)
            dl->AddText(ImVec2(x0 + 3, origin.y + laneY + 2),
                        IM_COL32(255,255,255,255),
                        e.name.c_str());
    }

    // zoom / pan controls
    if (ImGui::IsWindowHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
            g_TimelineScale = std::max(0.1f, g_TimelineScale + wheel * 0.1f);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            g_TimelineOffset += ImGui::GetIO().MouseDelta.x;
    }

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
// Draw Events Table
// ---------------------------------------------------------------------------

static void DrawEventsTable(std::vector<ProfileResult>& events)
{
    if (!g_ShowEventsTable)
        return;

    ImGui::SeparatorText("Events (Recent)");

    ImGui::BeginChild("eventstable", ImVec2(0, 250), true);

    ImGui::Columns(6);
    ImGui::Text("Name"); ImGui::NextColumn();
    ImGui::Text("Cat"); ImGui::NextColumn();
    ImGui::Text("Start"); ImGui::NextColumn();
    ImGui::Text("Dur (ms)"); ImGui::NextColumn();
    ImGui::Text("Thread"); ImGui::NextColumn();
    ImGui::Text("Depth"); ImGui::NextColumn();
    ImGui::Separator();

    for (auto& e : events)
    {
        if (strlen(g_SearchBuffer) > 0 &&
            e.name.find(g_SearchBuffer) == std::string::npos)
            continue;

        ImGui::Text("%s", e.name.c_str()); ImGui::NextColumn();
        ImGui::Text("%s", e.category.c_str()); ImGui::NextColumn();
        ImGui::Text("%lld", e.start); ImGui::NextColumn();
        ImGui::Text("%.3f", MicroToMs(e.duration)); ImGui::NextColumn();
        ImGui::Text("%zu", *(size_t*)&e.threadID); ImGui::NextColumn();
        ImGui::Text("%zu", e.depth); ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
// Public ImGui Panel Entry
// ---------------------------------------------------------------------------

void ProfilerSession::RenderImGui()
{
    ImGui::Begin("ProLog Profiler");

    // Controls
    if (ImGui::Button(g_Paused ? "Resume Capture" : "Pause Capture")) {
        g_Paused = !g_Paused;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save JSON")) {
        FlushBuffer();
        WriteFooter();
        WriteHeader(); // reopen
    }

    ImGui::InputText("Search", g_SearchBuffer, sizeof(g_SearchBuffer));

    ImGui::Checkbox("Show Timeline", &g_ShowTimeline);
    ImGui::Checkbox("Show Top Functions", &g_ShowTopFunctions);
    ImGui::Checkbox("Show Events Table", &g_ShowEventsTable);

    // Frame time graph
    DrawFrameGraph(*this);

    // Timeline
    DrawTimeline(*this);

    // Top functions
    DrawTopFunctions(*this);

    // Events table
    std::vector<ProfileResult> events;
    CollectRecentEvents(*this, events, 800);
    DrawEventsTable(events);

    ImGui::End();
}

} // namespace ProLog
