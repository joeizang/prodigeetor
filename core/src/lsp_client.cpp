#include "lsp_client.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>

namespace prodigeetor {
namespace lsp {

// Simple JSON builder/parser helpers (minimal implementation)
// In production, use a proper JSON library like nlohmann/json
namespace json {

std::string escape(const std::string& str) {
  std::string result;
  for (char c : str) {
    switch (c) {
      case '"': result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default: result += c;
    }
  }
  return result;
}

std::string position(const LSPPosition& pos) {
  return "{\"line\":" + std::to_string(pos.line) +
         ",\"character\":" + std::to_string(pos.character) + "}";
}

std::string range(const LSPRange& r) {
  return "{\"start\":" + position(r.start) + ",\"end\":" + position(r.end) + "}";
}

std::string textDocumentIdentifier(const std::string& uri) {
  return "{\"uri\":\"" + escape(uri) + "\"}";
}

std::string textDocumentPositionParams(const std::string& uri, const LSPPosition& pos) {
  return "{\"textDocument\":" + textDocumentIdentifier(uri) +
         ",\"position\":" + position(pos) + "}";
}

} // namespace json

struct LSPClient::Impl {
  pid_t pid = -1;
  int stdin_pipe[2] = {-1, -1};
  int stdout_pipe[2] = {-1, -1};
  std::string buffer;
  bool running = false;
};

LSPClient::LSPClient() : m_impl(std::make_unique<Impl>()) {}

LSPClient::~LSPClient() {
  shutdown();
}

bool LSPClient::start(const std::string& command, const std::vector<std::string>& args) {
  if (m_impl->running) {
    return false;
  }

  // Create pipes for stdin and stdout
  if (pipe(m_impl->stdin_pipe) < 0 || pipe(m_impl->stdout_pipe) < 0) {
    std::cerr << "Failed to create pipes: " << strerror(errno) << std::endl;
    return false;
  }

  // Fork the process
  m_impl->pid = fork();
  if (m_impl->pid < 0) {
    std::cerr << "Failed to fork: " << strerror(errno) << std::endl;
    close(m_impl->stdin_pipe[0]);
    close(m_impl->stdin_pipe[1]);
    close(m_impl->stdout_pipe[0]);
    close(m_impl->stdout_pipe[1]);
    return false;
  }

  if (m_impl->pid == 0) {
    // Child process
    // Redirect stdin and stdout
    dup2(m_impl->stdin_pipe[0], STDIN_FILENO);
    dup2(m_impl->stdout_pipe[1], STDOUT_FILENO);

    // Close unused pipe ends
    close(m_impl->stdin_pipe[0]);
    close(m_impl->stdin_pipe[1]);
    close(m_impl->stdout_pipe[0]);
    close(m_impl->stdout_pipe[1]);

    // Prepare arguments
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.c_str()));
    for (const auto& arg : args) {
      argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // Execute the LSP server
    execvp(command.c_str(), argv.data());

    // If exec fails
    std::cerr << "Failed to exec: " << strerror(errno) << std::endl;
    _exit(1);
  }

  // Parent process
  close(m_impl->stdin_pipe[0]);  // Close read end of stdin pipe
  close(m_impl->stdout_pipe[1]); // Close write end of stdout pipe

  // Set stdout pipe to non-blocking
  int flags = fcntl(m_impl->stdout_pipe[0], F_GETFL, 0);
  fcntl(m_impl->stdout_pipe[0], F_SETFL, flags | O_NONBLOCK);

  m_impl->running = true;
  return true;
}

void LSPClient::shutdown() {
  if (!m_impl->running) {
    return;
  }

  // Send shutdown request
  sendNotification("shutdown", "null");
  sendNotification("exit", "null");

  // Close pipes
  if (m_impl->stdin_pipe[1] >= 0) {
    close(m_impl->stdin_pipe[1]);
    m_impl->stdin_pipe[1] = -1;
  }
  if (m_impl->stdout_pipe[0] >= 0) {
    close(m_impl->stdout_pipe[0]);
    m_impl->stdout_pipe[0] = -1;
  }

  // Wait for process to exit
  if (m_impl->pid > 0) {
    int status;
    waitpid(m_impl->pid, &status, 0);
    m_impl->pid = -1;
  }

  m_impl->running = false;
}

bool LSPClient::is_running() const {
  return m_impl->running;
}

void LSPClient::initialize(const std::string& rootUri, ResponseCallback onSuccess, ErrorCallback onError) {
  std::string params = "{"
    "\"processId\":" + std::to_string(getpid()) + ","
    "\"rootUri\":\"" + json::escape(rootUri) + "\","
    "\"capabilities\":{"
      "\"textDocument\":{"
        "\"completion\":{\"dynamicRegistration\":false},"
        "\"hover\":{\"dynamicRegistration\":false},"
        "\"definition\":{\"dynamicRegistration\":false},"
        "\"references\":{\"dynamicRegistration\":false},"
        "\"documentSymbol\":{\"dynamicRegistration\":false}"
      "}"
    "}"
  "}";

  sendRequest("initialize", params,
    [this, onSuccess](const std::string& result) {
      // Parse capabilities from result
      // For now, enable all capabilities (simplified)
      m_capabilities.completionProvider = true;
      m_capabilities.hoverProvider = true;
      m_capabilities.definitionProvider = true;
      m_capabilities.referencesProvider = true;
      m_capabilities.documentSymbolProvider = true;
      m_capabilities.textDocumentSync = 2; // Incremental

      // Send initialized notification
      sendNotification("initialized", "{}");

      if (onSuccess) {
        onSuccess(result);
      }
    },
    onError
  );
}

void LSPClient::didOpen(const TextDocumentItem& document) {
  std::string params = "{"
    "\"textDocument\":{"
      "\"uri\":\"" + json::escape(document.uri) + "\","
      "\"languageId\":\"" + json::escape(document.languageId) + "\","
      "\"version\":" + std::to_string(document.version) + ","
      "\"text\":\"" + json::escape(document.text) + "\""
    "}"
  "}";
  sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const std::string& uri, int version,
                          const std::vector<TextDocumentContentChangeEvent>& changes) {
  std::string changesJson = "[";
  for (size_t i = 0; i < changes.size(); ++i) {
    if (i > 0) changesJson += ",";
    changesJson += "{\"text\":\"" + json::escape(changes[i].text) + "\"}";
  }
  changesJson += "]";

  std::string params = "{"
    "\"textDocument\":{"
      "\"uri\":\"" + json::escape(uri) + "\","
      "\"version\":" + std::to_string(version) +
    "},"
    "\"contentChanges\":" + changesJson +
  "}";
  sendNotification("textDocument/didChange", params);
}

void LSPClient::didClose(const std::string& uri) {
  std::string params = "{\"textDocument\":" + json::textDocumentIdentifier(uri) + "}";
  sendNotification("textDocument/didClose", params);
}

void LSPClient::didSave(const std::string& uri) {
  std::string params = "{\"textDocument\":" + json::textDocumentIdentifier(uri) + "}";
  sendNotification("textDocument/didSave", params);
}

void LSPClient::completion(const std::string& uri, LSPPosition position,
                           ResponseCallback onSuccess, ErrorCallback onError) {
  std::string params = json::textDocumentPositionParams(uri, position);
  sendRequest("textDocument/completion", params, onSuccess, onError);
}

void LSPClient::hover(const std::string& uri, LSPPosition position,
                     ResponseCallback onSuccess, ErrorCallback onError) {
  std::string params = json::textDocumentPositionParams(uri, position);
  sendRequest("textDocument/hover", params, onSuccess, onError);
}

void LSPClient::gotoDefinition(const std::string& uri, LSPPosition position,
                               ResponseCallback onSuccess, ErrorCallback onError) {
  std::string params = json::textDocumentPositionParams(uri, position);
  sendRequest("textDocument/definition", params, onSuccess, onError);
}

void LSPClient::references(const std::string& uri, LSPPosition position,
                          ResponseCallback onSuccess, ErrorCallback onError) {
  std::string params = json::textDocumentPositionParams(uri, position) +
                      ",\"context\":{\"includeDeclaration\":true}";
  sendRequest("textDocument/references", params, onSuccess, onError);
}

void LSPClient::documentSymbols(const std::string& uri,
                               ResponseCallback onSuccess, ErrorCallback onError) {
  std::string params = "{\"textDocument\":" + json::textDocumentIdentifier(uri) + "}";
  sendRequest("textDocument/documentSymbol", params, onSuccess, onError);
}

void LSPClient::onNotification(MessageCallback callback) {
  m_notificationCallback = std::move(callback);
}

void LSPClient::onDiagnostics(std::function<void(const std::string&, const std::vector<Diagnostic>&)> callback) {
  m_diagnosticsCallback = std::move(callback);
}

void LSPClient::processMessages() {
  if (!m_impl->running) {
    return;
  }

  // Check if there's data to read
  struct pollfd pfd;
  pfd.fd = m_impl->stdout_pipe[0];
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, 0);
  if (ret <= 0) {
    return;
  }

  // Read available data
  char buffer[4096];
  ssize_t n = read(m_impl->stdout_pipe[0], buffer, sizeof(buffer));
  if (n > 0) {
    m_impl->buffer.append(buffer, n);

    // Process complete messages
    while (true) {
      std::string message = readMessage();
      if (message.empty()) {
        break;
      }
      handleMessage(message);
    }
  }
}

void LSPClient::sendRequest(const std::string& method, const std::string& params,
                            ResponseCallback onSuccess, ErrorCallback onError) {
  int id = m_nextRequestId++;

  std::string message = "{"
    "\"jsonrpc\":\"2.0\","
    "\"id\":" + std::to_string(id) + ","
    "\"method\":\"" + method + "\","
    "\"params\":" + params +
  "}";

  if (onSuccess) {
    m_responseCallbacks[id] = onSuccess;
  }
  if (onError) {
    m_errorCallbacks[id] = onError;
  }

  writeMessage(message);
}

void LSPClient::sendNotification(const std::string& method, const std::string& params) {
  std::string message = "{"
    "\"jsonrpc\":\"2.0\","
    "\"method\":\"" + method + "\","
    "\"params\":" + params +
  "}";
  writeMessage(message);
}

void LSPClient::handleMessage(const std::string& message) {
  // Simple JSON parsing (in production, use a proper JSON library)
  // Check if it's a response or notification

  // Look for "id" field
  size_t idPos = message.find("\"id\":");
  if (idPos != std::string::npos) {
    // It's a response
    size_t idStart = idPos + 5;
    while (idStart < message.size() && (message[idStart] == ' ' || message[idStart] == '\t')) {
      idStart++;
    }
    size_t idEnd = idStart;
    while (idEnd < message.size() && message[idEnd] >= '0' && message[idEnd] <= '9') {
      idEnd++;
    }

    if (idEnd > idStart) {
      int id = std::stoi(message.substr(idStart, idEnd - idStart));

      // Check if it's an error response
      size_t errorPos = message.find("\"error\":");
      if (errorPos != std::string::npos) {
        auto it = m_errorCallbacks.find(id);
        if (it != m_errorCallbacks.end()) {
          it->second(0, "LSP Error");
          m_errorCallbacks.erase(it);
        }
        m_responseCallbacks.erase(id);
      } else {
        auto it = m_responseCallbacks.find(id);
        if (it != m_responseCallbacks.end()) {
          it->second(message);
          m_responseCallbacks.erase(it);
        }
        m_errorCallbacks.erase(id);
      }
    }
  } else {
    // It's a notification
    size_t methodPos = message.find("\"method\":");
    if (methodPos != std::string::npos) {
      size_t methodStart = message.find("\"", methodPos + 9);
      if (methodStart != std::string::npos) {
        size_t methodEnd = message.find("\"", methodStart + 1);
        if (methodEnd != std::string::npos) {
          std::string method = message.substr(methodStart + 1, methodEnd - methodStart - 1);

          // Handle diagnostics specially
          if (method == "textDocument/publishDiagnostics" && m_diagnosticsCallback) {
            // Extract URI and diagnostics (simplified)
            // In production, use proper JSON parsing
            m_diagnosticsCallback("", {});
          }

          if (m_notificationCallback) {
            m_notificationCallback(method, message);
          }
        }
      }
    }
  }
}

std::string LSPClient::readMessage() {
  // LSP messages use Content-Length header
  size_t headerEnd = m_impl->buffer.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    return "";
  }

  // Parse Content-Length
  size_t lengthPos = m_impl->buffer.find("Content-Length: ");
  if (lengthPos == std::string::npos || lengthPos > headerEnd) {
    m_impl->buffer.clear();
    return "";
  }

  size_t lengthStart = lengthPos + 16;
  size_t lengthEnd = m_impl->buffer.find("\r\n", lengthStart);
  if (lengthEnd == std::string::npos) {
    return "";
  }

  int contentLength = std::stoi(m_impl->buffer.substr(lengthStart, lengthEnd - lengthStart));
  size_t messageStart = headerEnd + 4;

  if (m_impl->buffer.size() < messageStart + contentLength) {
    return ""; // Not enough data yet
  }

  std::string message = m_impl->buffer.substr(messageStart, contentLength);
  m_impl->buffer.erase(0, messageStart + contentLength);

  return message;
}

void LSPClient::writeMessage(const std::string& message) {
  if (!m_impl->running || m_impl->stdin_pipe[1] < 0) {
    return;
  }

  std::string header = "Content-Length: " + std::to_string(message.size()) + "\r\n\r\n";
  std::string fullMessage = header + message;

  ssize_t written = write(m_impl->stdin_pipe[1], fullMessage.c_str(), fullMessage.size());
  if (written < 0) {
    std::cerr << "Failed to write to LSP server: " << strerror(errno) << std::endl;
  }
}

} // namespace lsp
} // namespace prodigeetor
