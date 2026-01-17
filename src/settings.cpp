#include "settings.h"

#include <afterhours/src/plugins/files.h>

#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>

#include "rl.h"

using namespace afterhours;

struct S_Data {
    afterhours::window_manager::Resolution resolution = {
        .width = 1280,
        .height = 720,
    };

    bool fullscreen_enabled = false;
    std::vector<std::string> recent_files;

    std::filesystem::path loaded_from;
};

void to_json(nlohmann::json &j,
             const afterhours::window_manager::Resolution &resolution) {
    j = nlohmann::json{
        {"width", resolution.width},
        {"height", resolution.height},
    };
}

void from_json(const nlohmann::json &j,
               afterhours::window_manager::Resolution &resolution) {
    j.at("width").get_to(resolution.width);
    j.at("height").get_to(resolution.height);
}

void to_json(nlohmann::json &j, const S_Data &data) {
    nlohmann::json rez_j;
    to_json(rez_j, data.resolution);
    j["resolution"] = rez_j;
    j["fullscreen_enabled"] = data.fullscreen_enabled;
    j["recent_files"] = data.recent_files;
}

void from_json(const nlohmann::json &j, S_Data &data) {
    from_json(j.at("resolution"), data.resolution);
    if (j.contains("fullscreen_enabled")) {
        data.fullscreen_enabled = j.at("fullscreen_enabled");
    }
    if (j.contains("recent_files")) {
        data.recent_files = j.at("recent_files").get<std::vector<std::string>>();
    }
}

Settings::Settings() { data = new S_Data(); }
Settings::~Settings() { delete data; }

void Settings::reset() {
    delete data;
    data = new S_Data();
    refresh_settings();
}

int Settings::get_screen_width() const { return data->resolution.width; }
int Settings::get_screen_height() const { return data->resolution.height; }

void Settings::update_resolution(afterhours::window_manager::Resolution rez) {
    data->resolution = rez;
    save_if_auto();
}

void match_fullscreen_to_setting(bool fs_enabled) {
    if (raylib::IsWindowFullscreen() && fs_enabled) return;
    if (!raylib::IsWindowFullscreen() && !fs_enabled) return;
    raylib::ToggleFullscreen();
}

void Settings::refresh_settings() {
    match_fullscreen_to_setting(data->fullscreen_enabled);
}

void Settings::toggle_fullscreen() {
    data->fullscreen_enabled = !data->fullscreen_enabled;
    raylib::ToggleFullscreen();
    save_if_auto();
}

void Settings::save_if_auto() {
    if (auto_save_enabled) {
        write_save_file();
    }
}

bool &Settings::get_fullscreen_enabled() { return data->fullscreen_enabled; }

const std::vector<std::string>& Settings::get_recent_files() const {
    return data->recent_files;
}

void Settings::add_recent_file(const std::string& path) {
    if (path.empty()) return;
    // Remove duplicates
    data->recent_files.erase(
        std::remove(data->recent_files.begin(), data->recent_files.end(), path),
        data->recent_files.end());
    // Insert at front
    data->recent_files.insert(data->recent_files.begin(), path);
    // Keep last 10
    if (data->recent_files.size() > 10) {
        data->recent_files.resize(10);
    }
    save_if_auto();
}

void Settings::clear_recent_files() {
    data->recent_files.clear();
    save_if_auto();
}

bool Settings::load_save_file(int width, int height) {
    this->data->resolution.width = width;
    this->data->resolution.height = height;

    std::vector<std::filesystem::path> settings_places = {
        std::filesystem::current_path() / "settings.json"};
    if (files::get_provider() != nullptr) {
        settings_places.push_back(files::get_save_path() / "settings.json");
    }

    size_t file_loc = 0;
    std::ifstream ifs;
    while (true) {
        if (file_loc >= settings_places.size()) {
            std::stringstream buffer;
            buffer << "Failed to find settings file (Read): \n";
            for (auto place : settings_places) buffer << place << ", \n";
            log_warn("{}", buffer.str());
            return false;
        }

        ifs = std::ifstream(settings_places[file_loc]);
        if (ifs.is_open()) {
            log_info("opened file {}", settings_places[file_loc]);
            break;
        }
        file_loc++;
    }
    data->loaded_from = settings_places[file_loc];

    try {
        const auto settingsJSON =
            nlohmann::json::parse(ifs, nullptr, true, true);

        (*this->data) = settingsJSON;
        this->data->loaded_from = settings_places[file_loc];
        refresh_settings();
        return true;

    } catch (const std::exception &e) {
        log_error("Settings::load_save_file: {} formatted improperly. {}",
                  data->loaded_from, e.what());
        return false;
    }
}

void Settings::write_save_file() {
    // If no settings file was loaded, use default path
    std::string save_path = data->loaded_from;
    if (save_path.empty()) {
        save_path = "settings.json";
    }

    std::ofstream ofs(save_path);
    if (!ofs.good()) {
        std::cerr << "write_json_config_file error: Couldn't open file "
                     "for writing: "
                  << save_path << std::endl;
        return;
    }
    data->loaded_from = save_path;

    log_info("Saving to {}", data->loaded_from);

    nlohmann::json settingsJSON = *data;

    ofs << settingsJSON.dump(4);
    ofs.close();
}
