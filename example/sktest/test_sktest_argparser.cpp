#include <string>
#include <vector>

#include "skutils/argparser.h"
#include "skutils/printer.h"
#include "skutils/test.h"

int main() {  // NOLINT
  namespace ag = sk::utils::arg;

  ag::ArgParser parser;
  parser.add_arg({.name = "--path", .type = ag::ArgType::LIST, .help = "File or Directory"})
    .add_arg({.name = "-o", .type = ag::ArgType::STR, .help = "Out Put Name"})
    .add_arg({.name = "-i", .type = ag::ArgType::INT, .help = "Short Path"})
    .add_arg({.name = "-bool", .type = ag::ArgType::BOOL, .help = "Boolean Value"})
    .add_arg({.name = "error", .type = ag::ArgType::STR, .help = "Skipped Value"});

  int argc = 14;
  const char* argv_raw[] = {"filename", "default_val", "-o",           "false_outname", "-o  =true_outname ",
                            "--path",   " path1  ",    "path space 2", "--path ",       "dir/path3",
                            "-i",       "123",         "-bool",        "unparsed",      "out_range"};
  char* argv[14];
  for (int i = 0; i < argc; ++i) argv[i] = const_cast<char*>(argv_raw[i]);

  parser.parse(argc, argv);

  auto parsed_filename = parser.get_file_name();
  auto parsed_front_value = parser.get_front_args().value_or(std::vector<std::string>{""});
  auto parsed_back_value = parser.get_back_args().value_or(std::vector<std::string>{""});
  auto parsed_out_name = std::get<std::string>(parser.get_value("-o").value_or("FALSE_NAME"));
  auto parsed_path =
    std::get<std::vector<std::string>>(parser.get_value("--path").value_or(std::vector<std::string>{"FALSE_VALUE"}));
  auto parsed_int = std::get<int>(parser.get_value("-i").value_or(-1));
  auto parsed_bool = std::get<bool>(parser.get_value("-bool").value_or(false));

  auto correct_front_default_value = std::vector<std::string>{"default_val"};
#if COLLECT_UNEXPECTED_ARGS
  auto correct_back_default_value = std::vector<std::string>{"unparsed"};
#else
  auto correct_back_default_value = std::vector<std::string>{""};
#endif
  auto correct_path = std::vector<std::string>{"path1", "path space 2", "dir/path3"};

  ASSERT_STR_EQUAL(parsed_filename, "filename");
  ASSERT_STR_EQUAL(parsed_front_value, correct_front_default_value);
  ASSERT_STR_EQUAL(parsed_back_value, correct_back_default_value);
  ASSERT_STR_EQUAL(parsed_out_name, "true_outname");
  ASSERT_STR_EQUAL(parsed_path, correct_path);
  ASSERT_EQUAL(parsed_int, 123);
  ASSERT_TRUE(parsed_bool);

  parser.show_help();

  return ASSERT_ALL_PASSED();
}