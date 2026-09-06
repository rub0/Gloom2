#pragma once
#include <gloom/gameplay/match_discovery.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace gloom::desktop {
struct Session {
    std::shared_ptr<gameplay::SliceMatchDirectory> directory;
    std::function<std::expected<std::string,std::string>()> identity;
    std::string notice;
    bool quit{false};
    // Bounded acceptance driver. Empty in normal play; uses the same UI input seam.
    std::filesystem::path flow_output;
    std::string flow_name;
    std::size_t flow_selection{3};
};
std::vector<std::string> menu(Session& session,const std::filesystem::path& review={},unsigned width=1280,unsigned height=720);
}
