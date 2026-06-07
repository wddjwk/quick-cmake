#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <system_error>

#include "skutils/argparser.h"
#include "skutils/printer.h"
#include "skutils/string_utils.h"
#include "skutils/time_utils.h"

namespace fs = std::filesystem;

const static std::set<std::string> skipped_dirs{".git", "build"};  // NOLINT

void collect_files(std::vector<fs::path>& files, const fs::path& path, const std::string& ext, bool no_skip) {
  if (!fs::exists(path)) {
    return;
  }
  if (fs::is_directory(path)) {
    if (!no_skip && skipped_dirs.find(path.filename().generic_string()) != skipped_dirs.end()) {
      return;
    }
    for (const auto& entry : fs::directory_iterator(path)) {
      collect_files(files, entry, ext, no_skip);
    }
  } else {
    if (path.extension() == ext) {
      files.emplace_back(path);
    }
  }
}

void combine_files(const std::vector<fs::path>& paths, const std::string& output, const std::string& ext,
                   bool show_detail_files, bool no_skip) {
  std::vector<fs::path> files;
  for (const auto& p : paths) {
    collect_files(files, sk::utils::str::expandUser(p.generic_string()), ext, no_skip);
  }
  std::ofstream out(output, std::ios::out);
  if (!out.is_open()) {
    SK_ERROR("Cannot create file {}, combine failed.", output);
    return;
  }

  for (const auto& f : files) {
    std::ifstream in(f);
    if (!in.is_open()) {
      SK_ERROR("Cannon Open file {}, skipped.", f.filename());
      continue;
    }
    out << sk::utils::format("<<<<<<<<<< [FILE-BEGIN] '{}' <<<<<<<<<<\n\n", f.string());
    std::string line;
    while (std::getline(in, line)) {
      out << line << "\n";
    }
    in.close();
    out << sk::utils::format("\n>>>>>>>>>> [FILE-END] '{}' >>>>>>>>>>\n\n\n", f.string());
  }
  out.close();

  if (show_detail_files) {
    std::cout << sk::utils::colorful_format("Combine: \n{}\n=> {}\n", files, output);
  } else {
    std::cout << sk::utils::colorful_format("Combined files {} from {}\n", output, paths);
  }
}

int main(int argc, char** argv) {
  namespace args = sk::utils::arg;

  args::ArgParser parser;

  parser.add_arg({.name = "-p", .type = args::ArgType::LIST, .help = "Specify files or directories."})
    .add_arg({.name = "-o", .type = args::ArgType::STR, .help = "Specify combined file name."})
    .add_arg(
      {.name = "-e", .type = args::ArgType::STR, .help = "File type to combine, like txt, md, cpp etc, default is txt"})
    .add_arg({.name = "-v", .type = args::ArgType::BOOL, .help = "Show All Combined Files."})
    .add_arg(
      {.name = "-uu", .type = args::ArgType::BOOL, .help = "Dont Skip Ingored Directories, like build adn .git"});

  parser.parse(argc, argv);

  if (parser.need_help()) {
    parser.show_help();
    return 0;
  }

  std::vector<fs::path> paths;

  if (parser.get_front_args().has_value() && parser.get_back_args().has_value()) {
    SK_WARN("You have default value at both front and back, which is not recommended.");
  }

  for (auto& p : parser.get_front_args().value_or(std::vector<std::string>{})) {
    paths.emplace_back(std::move(p));
  }
  for (auto& p : parser.get_back_args().value_or(std::vector<std::string>{})) {
    paths.emplace_back(std::move(p));
  }

  auto args_p = std::get<std::vector<std::string>>(parser.get_value("-p").value_or(std::vector<std::string>{}));
  for (auto& p : args_p) {
    paths.emplace_back(std::move(p));
  }

  if (paths.empty()) {
    paths.emplace_back(fs::current_path());
  }

  auto ext = std::get<std::string>(parser.get_value("-e").value_or(".txt"));
  if (ext[0] != '.') {
    ext = '.' + ext;
  }

  std::string default_name("combined_files" + sk::utils::time::current("%Y%m%d_%H%M%S") + ext);
  auto output_name = std::get<std::string>(parser.get_value("-o").value_or(default_name));
  if (output_name.find('.') == std::string::npos) {
    output_name += ext;
  }

  bool show_detail_files = std::get<bool>(parser.get_value("-v").value_or(false));
  bool no_skip = std::get<bool>(parser.get_value("-uu").value_or(false));
  combine_files(paths, output_name, ext, show_detail_files, no_skip);
}