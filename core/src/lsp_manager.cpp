#include "lsp_manager.h"
#include <algorithm>
#include <iostream>

namespace prodigeetor {
namespace lsp {

LSPManager::LSPManager() {}

LSPManager::~LSPManager() {
  shutdown();
}

void LSPManager::registerLanguageServer(const std::string& name, const LanguageServerConfig& config) {
  ServerInfo info;
  info.client = std::make_unique<LSPClient>();
  info.config = config;
  info.initialized = false;
  m_servers[name] = std::move(info);
}

void LSPManager::initializeServers(const std::string& rootUri) {
  m_rootUri = rootUri;

  for (auto& [name, info] : m_servers) {
    if (info.client->start(info.config.command, info.config.args)) {
      info.client->initialize(
        rootUri,
        [&info, name](const std::string& result) {
          info.initialized = true;
          std::cout << "LSP server '" << name << "' initialized successfully" << std::endl;
        },
        [name](int code, const std::string& message) {
          std::cerr << "Failed to initialize LSP server '" << name << "': " << message << std::endl;
        }
      );

      // Set up diagnostics callback
      if (m_diagnosticsCallback) {
        info.client->onDiagnostics(m_diagnosticsCallback);
      }
    } else {
      std::cerr << "Failed to start LSP server: " << name << std::endl;
    }
  }
}

void LSPManager::didOpen(const std::string& uri, const std::string& languageId, const std::string& text) {
  LSPClient* client = getClientForLanguage(languageId);
  if (!client) {
    return;
  }

  TextDocumentItem doc;
  doc.uri = uri;
  doc.languageId = languageId;
  doc.version = 1;
  doc.text = text;

  client->didOpen(doc);

  // Track which server handles this document
  std::string serverName = getServerNameForLanguage(languageId);
  if (!serverName.empty()) {
    m_documentToServer[uri] = serverName;
  }
}

void LSPManager::didChange(const std::string& uri, const std::string& text) {
  LSPClient* client = getClientForUri(uri);
  if (!client) {
    return;
  }

  TextDocumentContentChangeEvent change;
  change.text = text;

  std::vector<TextDocumentContentChangeEvent> changes = {change};
  client->didChange(uri, 1, changes);
}

void LSPManager::didClose(const std::string& uri) {
  LSPClient* client = getClientForUri(uri);
  if (!client) {
    return;
  }

  client->didClose(uri);
  m_documentToServer.erase(uri);
}

void LSPManager::didSave(const std::string& uri) {
  LSPClient* client = getClientForUri(uri);
  if (!client) {
    return;
  }

  client->didSave(uri);
}

void LSPManager::completion(const std::string& uri, int line, int character,
                           std::function<void(const std::vector<CompletionItem>&)> callback) {
  LSPClient* client = getClientForUri(uri);
  if (!client) {
    callback({});
    return;
  }

  LSPPosition pos{line, character};
  client->completion(
    uri, pos,
    [callback](const std::string& result) {
      // Parse completion result (simplified)
      // In production, properly parse JSON
      std::vector<CompletionItem> items;
      callback(items);
    },
    [callback](int code, const std::string& message) {
      callback({});
    }
  );
}

void LSPManager::hover(const std::string& uri, int line, int character,
                      std::function<void(const std::optional<Hover>&)> callback) {
  LSPClient* client = getClientForUri(uri);
  if (!client) {
    callback(std::nullopt);
    return;
  }

  LSPPosition pos{line, character};
  client->hover(
    uri, pos,
    [callback](const std::string& result) {
      // Parse hover result (simplified)
      std::optional<Hover> hover;
      callback(hover);
    },
    [callback](int code, const std::string& message) {
      callback(std::nullopt);
    }
  );
}

void LSPManager::gotoDefinition(const std::string& uri, int line, int character,
                               std::function<void(const std::vector<LSPLocation>&)> callback) {
  LSPClient* client = getClientForUri(uri);
  if (!client) {
    callback({});
    return;
  }

  LSPPosition pos{line, character};
  client->gotoDefinition(
    uri, pos,
    [callback](const std::string& result) {
      // Parse definition result (simplified)
      std::vector<LSPLocation> locations;
      callback(locations);
    },
    [callback](int code, const std::string& message) {
      callback({});
    }
  );
}

void LSPManager::documentSymbols(const std::string& uri,
                                std::function<void(const std::vector<DocumentSymbol>&)> callback) {
  LSPClient* client = getClientForUri(uri);
  if (!client) {
    callback({});
    return;
  }

  client->documentSymbols(
    uri,
    [callback](const std::string& result) {
      // Parse symbols result (simplified)
      std::vector<DocumentSymbol> symbols;
      callback(symbols);
    },
    [callback](int code, const std::string& message) {
      callback({});
    }
  );
}

void LSPManager::onDiagnostics(std::function<void(const std::string&, const std::vector<Diagnostic>&)> callback) {
  m_diagnosticsCallback = std::move(callback);

  // Register with all active clients
  for (auto& [name, info] : m_servers) {
    if (info.client) {
      info.client->onDiagnostics(m_diagnosticsCallback);
    }
  }
}

void LSPManager::processMessages() {
  for (auto& [name, info] : m_servers) {
    if (info.client && info.client->is_running()) {
      info.client->processMessages();
    }
  }
}

void LSPManager::shutdown() {
  for (auto& [name, info] : m_servers) {
    if (info.client) {
      info.client->shutdown();
    }
  }
  m_servers.clear();
  m_documentToServer.clear();
}

LSPClient* LSPManager::getClientForUri(const std::string& uri) {
  auto it = m_documentToServer.find(uri);
  if (it == m_documentToServer.end()) {
    return nullptr;
  }

  auto serverIt = m_servers.find(it->second);
  if (serverIt == m_servers.end() || !serverIt->second.initialized) {
    return nullptr;
  }

  return serverIt->second.client.get();
}

LSPClient* LSPManager::getClientForLanguage(const std::string& languageId) {
  std::string serverName = getServerNameForLanguage(languageId);
  if (serverName.empty()) {
    return nullptr;
  }

  auto it = m_servers.find(serverName);
  if (it == m_servers.end() || !it->second.initialized) {
    return nullptr;
  }

  return it->second.client.get();
}

std::string LSPManager::getLanguageIdFromUri(const std::string& uri) {
  // Extract file extension
  size_t dotPos = uri.find_last_of('.');
  if (dotPos == std::string::npos) {
    return "";
  }

  std::string ext = uri.substr(dotPos);

  // Map extension to language ID
  for (const auto& [name, info] : m_servers) {
    for (const auto& configExt : info.config.extensions) {
      if (ext == configExt) {
        return info.config.languageId;
      }
    }
  }

  return "";
}

std::string LSPManager::getServerNameForLanguage(const std::string& languageId) {
  for (const auto& [name, info] : m_servers) {
    if (info.config.languageId == languageId) {
      return name;
    }
  }
  return "";
}

} // namespace lsp
} // namespace prodigeetor
