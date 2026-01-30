// Example extension entry point
export function activate(api: any) {
  api.commands.registerCommand("helloWorld", () => {
    const editor = api.editor.getActiveEditor();
    if (!editor) {
      api.window.showMessage("info", "No active editor");
      return;
    }
    editor.insertText("Hello from Prodigeetor!\n");
  });
}

export function deactivate() {
  // Cleanup if needed
}
