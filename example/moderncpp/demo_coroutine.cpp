#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)

#include <chrono>
#include <coroutine>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace fs = std::filesystem;

// 定义一个可等待对象（awaiter），用于等待文件读取完成
struct FileReadAwaitable {
  std::string* data;
  fs::path path;

  bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    // 在新线程中执行文件读取
    std::thread([this, h] {
      std::ifstream file(path, std::ios::binary);
      if (file) {
        *data = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      }
      h.resume();  // 恢复协程
    }).detach();
  }

  void await_resume() const noexcept {}
};

// 定义一个协程返回类型
struct FileReadTask {
  struct promise_type {
    FileReadTask get_return_object() { return {}; }

    std::suspend_never initial_suspend() { return {}; }

    std::suspend_never final_suspend() noexcept { return {}; }

    void return_void() {}

    void unhandled_exception() { std::terminate(); }
  };
};

// 异步读取文件到字符串的协程
static FileReadTask asyncReadFile(fs::path path, std::string& outData) {
  co_await FileReadAwaitable{.data = &outData, .path = path};
}

int main() {
  fs::path filePath = "D:/test/quick-cmake/example/boost/demo_coro.cpp";  // 文件路径
  std::string fileContent;

  // 启动协程
  auto task = asyncReadFile(filePath, fileContent);

  // 主线程继续做其他工作
  std::cout << "Doing other work...\n";
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // 等待协程完成
  std::cout << "File content: \n" << fileContent << std::endl;

  return 0;
}

#else
#include <iostream>

int main() {
  std::cerr << "Compile Coroutine Demo Using CXX-20 or later\n";
}
#endif
