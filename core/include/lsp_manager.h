#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "lsp_client.h"
#include "lsp_types.h"

namespace prodigeetor {
namespace lsp {

// Language server configuration
struct LanguageServerConfig {
  std::string command;
  std::vector<std::string> args;
  std::vector<std::string> extensions; // e.g., {".ts", ".tsx", ".js", ".jsx"}
  std::string languageId; // e.g., "typescript", "javascript"
};

// LSP Manager - handles multiple language servers
class LSPManager {
public:
  LSPManager();
  ~LSPManager();

  // Configuration
  void registerLanguageServer(const std::string& name, const LanguageServerConfig& config);

  // Initialize all registered servers
  void initializeServers(const std::string& rootUri);

  // Document lifecycle
  void didOpen(const std::string& uri, const std::string& languageId, const std::string& text);
  void didChange(const std::string& uri, const std::string& text);
  void didClose(const std::string& uri);
  void didSave(const std::string& uri);

  // Language features
  void completion(const std::string& uri, int line, int character,
                 std::function<void(const std::vector<CompletionItem>&)> callback);
  void hover(const std::string& uri, int line, int character,
            std::function<void(const std::optional<Hover>&)> callback);
  void gotoDefinition(const std::string& uri, int line, int character,
                     std::function<void(const std::vector<LSPLocation>&)> callback);
  void documentSymbols(const std::string& uri,
                      std::function<void(const std::vector<DocumentSymbol>&)> callback);

  // Diagnostics callback
  void onDiagnostics(std::function<void(const std::string& uri, const std::vector<Diagnostic>&)> callback);

  // Process incoming messages from all servers
  void processMessages();

  // Shutdown all servers
  void shutdown();

private:
  struct ServerInfo {
    std::unique_ptr<LSPClient> client;
    LanguageServerConfig config;
    bool initialized = false;
  };

  std::unordered_map<std::string, ServerInfo> m_servers;
  std::unordered_map<std::string, std::string> m_documentToServer; // uri -> server name
  std::function<void(const std::string&, const std::vector<Diagnostic>&)> m_diagnosticsCallback;
  std::string m_rootUri;

  // Helper methods
  LSPClient* getClientForUri(const std::string& uri);
  LSPClient* getClientForLanguage(const std::string& languageId);
  std::string getLanguageIdFromUri(const std::string& uri);
  std::string getServerNameForLanguage(const std::string& languageId);
};

} // namespace lsp
} // namespace prodigeetor
