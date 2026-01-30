#pragma once

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <optional>
#include "lsp_types.h"

namespace prodigeetor {
namespace lsp {

// Forward declaration
class JSONValue;

// LSP Client interface
class LSPClient {
public:
  using MessageCallback = std::function<void(const std::string& method, const std::string& params)>;
  using ResponseCallback = std::function<void(const std::string& result)>;
  using ErrorCallback = std::function<void(int code, const std::string& message)>;

  LSPClient();
  virtual ~LSPClient();

  // Lifecycle methods
  bool start(const std::string& command, const std::vector<std::string>& args);
  void shutdown();
  bool is_running() const;

  // Initialize the LSP server
  void initialize(const std::string& rootUri, ResponseCallback onSuccess, ErrorCallback onError);

  // Document lifecycle
  void didOpen(const TextDocumentItem& document);
  void didChange(const std::string& uri, int version, const std::vector<TextDocumentContentChangeEvent>& changes);
  void didClose(const std::string& uri);
  void didSave(const std::string& uri);

  // Language features
  void completion(const std::string& uri, LSPPosition position, ResponseCallback onSuccess, ErrorCallback onError);
  void hover(const std::string& uri, LSPPosition position, ResponseCallback onSuccess, ErrorCallback onError);
  void gotoDefinition(const std::string& uri, LSPPosition position, ResponseCallback onSuccess, ErrorCallback onError);
  void references(const std::string& uri, LSPPosition position, ResponseCallback onSuccess, ErrorCallback onError);
  void documentSymbols(const std::string& uri, ResponseCallback onSuccess, ErrorCallback onError);

  // Notification handlers
  void onNotification(MessageCallback callback);
  void onDiagnostics(std::function<void(const std::string& uri, const std::vector<Diagnostic>&)> callback);

  // Process incoming messages
  void processMessages();

  // Server capabilities
  const ServerCapabilities& capabilities() const { return m_capabilities; }

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;

  int m_nextRequestId = 1;
  ServerCapabilities m_capabilities;
  std::unordered_map<int, ResponseCallback> m_responseCallbacks;
  std::unordered_map<int, ErrorCallback> m_errorCallbacks;
  MessageCallback m_notificationCallback;
  std::function<void(const std::string&, const std::vector<Diagnostic>&)> m_diagnosticsCallback;

  // Internal methods
  void sendRequest(const std::string& method, const std::string& params,
                   ResponseCallback onSuccess, ErrorCallback onError);
  void sendNotification(const std::string& method, const std::string& params);
  void handleMessage(const std::string& message);
  std::string readMessage();
  void writeMessage(const std::string& message);
};

} // namespace lsp
} // namespace prodigeetor
