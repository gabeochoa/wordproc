// App-specific E2E Command Handlers for wordproc
// Menu, document, and outline commands specific to this application
#pragma once

#include "../ecs/components.h"
#include "../editor/document_io.h"
#include "../util/file_dialog.h"

#include <afterhours/src/plugins/e2e_testing/e2e_testing.h>

#include <fstream>

namespace e2e_commands {

using namespace afterhours;

// Strip surrounding double-quotes from a string (the default E2E parser
// doesn't recognise menu_open / menu_select as quoted-arg commands, so
// the quotes end up in the argument list).
static inline std::string strip_quotes(const std::string &s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

// Registers document text into the visible text registry every frame.
// Must run in the same TU as HandleExpectTextCommand to ensure they
// share the same VisibleTextRegistry singleton instance.
struct DocumentTextRegistration : System<> {
  ecs::DocumentComponent *doc_comp = nullptr;

  bool should_iterate() const override { return false; }

  void once(float) override {
    if (!doc_comp || !testing::test_input::detail::test_mode)
      return;
    auto &reg = testing::VisibleTextRegistry::instance();
    auto &buffer = doc_comp->buffer;
    int lineCount = static_cast<int>(buffer.lineCount());
    for (int i = 0; i < lineCount; i++) {
      std::string line = buffer.lineString(static_cast<std::size_t>(i));
      if (!line.empty()) {
        reg.register_text(line);
      }
    }
  }
};

// Handle 'menu_open Menu' - opens a menu by name
struct HandleMenuOpenCommand : System<testing::PendingE2ECommand> {
  ecs::MenuComponent *menu_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("menu_open"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("menu_open requires menu name");
      return;
    }
    if (!menu_comp) {
      cmd.fail("menu_comp not set");
      return;
    }

    const std::string menu_name = strip_quotes(cmd.arg(0));
    
    // Close any currently open menus first
    for (auto &menu : menu_comp->menus) {
      menu.open = false;
    }
    
    // Open the requested menu
    for (auto &menu : menu_comp->menus) {
      if (menu.label == menu_name) {
        menu.open = true;
        cmd.consume();
        return;
      }
    }
    
    cmd.fail("Menu not found: " + menu_name);
  }
};

// Handle 'menu_select Item' - selects a menu item
struct HandleMenuSelectCommand : System<testing::PendingE2ECommand> {
  ecs::MenuComponent *menu_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("menu_select"))
      return;
    if (cmd.args.empty()) {
      cmd.fail("menu_select requires item name");
      return;
    }
    if (!menu_comp) {
      cmd.fail("menu_comp not set - was E2EConfig.menu_comp passed?");
      return;
    }

    // Join all arguments to support multi-word menu items, stripping quotes
    std::string item_name = cmd.args[0];
    for (size_t i = 1; i < cmd.args.size(); ++i) {
      item_name += " " + cmd.args[i];
    }
    item_name = strip_quotes(item_name);
    
    // Debug: Check if any menu is open
    bool anyOpen = false;
    for (const auto &menu : menu_comp->menus) {
      if (menu.open) {
        anyOpen = true;
        break;
      }
    }
    if (!anyOpen) {
      cmd.fail("No menu is currently open - use menu_open first");
      return;
    }
    
    for (std::size_t menuIdx = 0; menuIdx < menu_comp->menus.size(); ++menuIdx) {
      auto &menu = menu_comp->menus[menuIdx];
      if (menu.open) {
        // Debug: list all menu items
        std::string available_items;
        for (std::size_t i = 0; i < menu.items.size(); ++i) {
          if (!menu.items[i].separator) {
            if (!available_items.empty()) available_items += ", ";
            available_items += "'" + menu.items[i].label + "'";
          }
          if (menu.items[i].label == item_name) {
            // Set the clicked result for handleMenuActionImpl to process
            menu_comp->lastClickedResult = static_cast<int>(menuIdx * 100 + i);
            menu.open = false;
            cmd.consume();
            return;
          }
        }
        cmd.fail("Menu item '" + item_name + "' not found in '" + menu.label + "'. Available: " + available_items);
        return;
      }
    }
    
    cmd.fail("Menu item not found: " + item_name + " (no menu open)");
  }
};

// Handle 'document_dump path' - dumps document content to file
struct HandleDocumentDumpCommand : System<testing::PendingE2ECommand> {
  ecs::DocumentComponent *doc_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("document_dump"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("document_dump requires file path");
      return;
    }
    if (!doc_comp) {
      cmd.fail("doc_comp not set");
      return;
    }

    std::ofstream file(cmd.arg(0));
    if (file.is_open()) {
      doc_comp->buffer.writeTextTo(file);
      cmd.consume();
    } else {
      cmd.fail("Failed to open file: " + cmd.arg(0));
    }
  }
};

// Handle 'outline_click heading_text' - clicks an outline item
struct HandleOutlineClickCommand : System<testing::PendingE2ECommand> {
  ecs::DocumentComponent *doc_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("outline_click"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("outline_click requires heading text");
      return;
    }
    if (!doc_comp) {
      cmd.fail("doc_comp not set");
      return;
    }

    const auto &heading_text = cmd.arg(0);
    auto outline = doc_comp->buffer.getOutline();
    
    for (const auto &entry : outline) {
      if (entry.text == heading_text || 
          entry.text.find(heading_text) != std::string::npos) {
        if (doc_comp->buffer.goToOutlineEntry(entry.lineNumber)) {
          cmd.consume();
          return;
        }
      }
    }
    
    cmd.fail("Outline entry not found: " + heading_text);
  }
};

// Handle 'open_file path' - opens a file directly into the document
struct HandleOpenFileCommand : System<testing::PendingE2ECommand> {
  ecs::DocumentComponent *doc_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("open_file"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("open_file requires file path");
      return;
    }
    if (!doc_comp) {
      cmd.fail("doc_comp not set");
      return;
    }

    const auto &file_path = cmd.arg(0);
    DocumentResult result = loadDocumentEx(doc_comp->buffer,
                                           doc_comp->docSettings,
                                           file_path);
    if (result.success) {
      doc_comp->filePath = file_path;
      doc_comp->isDirty = false;
      doc_comp->comments.clear();
      doc_comp->revisions.clear();
      cmd.consume();
    } else {
      cmd.fail("Failed to open file: " + file_path +
               (result.error.empty() ? "" : " (" + result.error + ")"));
    }
  }
};

// Handle 'mouse_up' - releases the mouse button (needed between sequential click_text calls
// because simulate_click leaves left_down=true and BeginUIContextManager derives just_pressed
// from the false→true transition)
struct HandleMouseUpCommand : System<testing::PendingE2ECommand> {
  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("mouse_up"))
      return;
    testing::test_input::simulate_mouse_release();
    cmd.consume();
  }
};

// Handle 'load_template name' - directly loads a template, bypassing the dialog
struct HandleLoadTemplateCommand : System<testing::PendingE2ECommand> {
  ecs::DocumentComponent *doc_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("load_template"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("load_template requires template name");
      return;
    }
    if (!doc_comp) {
      cmd.fail("doc_comp not set");
      return;
    }

    std::string name = strip_quotes(cmd.arg(0));
    for (auto &ch : name) ch = static_cast<char>(std::tolower(ch));
    std::filesystem::path templatePath =
        std::filesystem::current_path() / "resources/templates" / (name + ".txt");
    if (std::filesystem::exists(templatePath)) {
      std::ifstream ifs(templatePath);
      std::stringstream buf;
      buf << ifs.rdbuf();
      doc_comp->buffer.setText(buf.str());
      doc_comp->isDirty = true;
      cmd.consume();
    } else {
      cmd.fail("Template not found: " + templatePath.string());
    }
  }
};

// Handle 'add_comment text' - directly adds a comment to the document
struct HandleAddCommentCommand : System<testing::PendingE2ECommand> {
  ecs::DocumentComponent *doc_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("add_comment"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("add_comment requires comment text");
      return;
    }
    if (!doc_comp) {
      cmd.fail("doc_comp not set");
      return;
    }

    // Join all args and strip quotes
    std::string text = cmd.args[0];
    for (size_t i = 1; i < cmd.args.size(); ++i)
      text += " " + cmd.args[i];
    text = strip_quotes(text);

    Comment comment;
    comment.text = text;
    comment.author = "Test";
    comment.createdAt = std::time(nullptr);
    comment.startOffset = doc_comp->buffer.caret().column;
    comment.endOffset = comment.startOffset;
    if (doc_comp->buffer.hasSelection()) {
      comment.startOffset = doc_comp->buffer.selectionStart().column;
      comment.endOffset = doc_comp->buffer.selectionEnd().column;
    }
    doc_comp->comments.push_back(comment);
    cmd.consume();
  }
};

// Handle 'set_tab_width N' - directly sets tab width
struct HandleSetTabWidthCommand : System<testing::PendingE2ECommand> {
  ecs::DocumentComponent *doc_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("set_tab_width"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("set_tab_width requires a number");
      return;
    }
    if (!doc_comp) {
      cmd.fail("doc_comp not set");
      return;
    }

    int width = std::atoi(cmd.arg(0).c_str());
    if (width >= 1 && width <= 16) {
      doc_comp->docSettings.tabWidth = width;
      cmd.consume();
    } else {
      cmd.fail("Tab width must be 1-16, got: " + cmd.arg(0));
    }
  }
};

// Handle 'replace_text "find" "replace"' - finds and replaces text in the document
struct HandleReplaceTextCommand : System<testing::PendingE2ECommand> {
  ecs::DocumentComponent *doc_comp = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("replace_text"))
      return;
    if (!cmd.has_args(2)) {
      cmd.fail("replace_text requires find and replace arguments");
      return;
    }
    if (!doc_comp) {
      cmd.fail("doc_comp not set");
      return;
    }

    // The E2E parser splits on whitespace, so multi-word quoted strings
    // arrive as separate args. We need to reconstruct them.
    // Format: replace_text FIND_TEXT "replacement text here"
    // args might be: [FIND_TEXT, "replacement, text, here"]
    // We take arg(0) as the find string, and join the rest as the replacement.
    std::string findStr = strip_quotes(cmd.arg(0));
    std::string replaceStr = cmd.args[1];
    for (size_t i = 2; i < cmd.args.size(); ++i)
      replaceStr += " " + cmd.args[i];
    replaceStr = strip_quotes(replaceStr);

    std::string text = doc_comp->buffer.getText();
    auto pos = text.find(findStr);
    if (pos != std::string::npos) {
      text.replace(pos, findStr.length(), replaceStr);
      doc_comp->buffer.setText(text);
      doc_comp->isDirty = true;
      cmd.consume();
    } else {
      cmd.fail("Text not found: " + findStr);
    }
  }
};

// Handle 'set_find_term text' - sets the search term for Find Next/Previous
struct HandleSetFindTermCommand : System<testing::PendingE2ECommand> {
  ecs::DialogState *dialog_state = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("set_find_term"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("set_find_term requires search text");
      return;
    }
    if (!dialog_state) {
      cmd.fail("dialog_state not set");
      return;
    }

    std::string term = cmd.args[0];
    for (size_t i = 1; i < cmd.args.size(); ++i)
      term += " " + cmd.args[i];
    term = strip_quotes(term);

    dialog_state->lastSearchTerm = term;
    dialog_state->findInputStr = term;
    std::strncpy(dialog_state->findInputBuffer, term.c_str(),
                 sizeof(dialog_state->findInputBuffer) - 1);
    cmd.consume();
  }
};

// Handle 'right_click x y' - opens context menu at coordinates
// Workaround: bypasses input simulation (afterhours doesn't support right-click
// in test mode yet) and directly sets context menu state.
// TODO: Upstream right-click support to afterhours (see afterhours-feature-requests.md #14)
struct HandleRightClickCommand : System<testing::PendingE2ECommand> {
  ecs::DialogState *dialog_state = nullptr;

  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("right_click"))
      return;
    if (!cmd.has_args(2)) {
      cmd.fail("right_click requires x y arguments");
      return;
    }
    if (!dialog_state) {
      cmd.fail("dialog_state not set");
      return;
    }

    auto [sw, sh] = testing::e2e_screen_size();
    float x = cmd.coord_arg(0, sw);
    float y = cmd.coord_arg(1, sh);

    dialog_state->showContextMenu = true;
    dialog_state->contextMenuX = x;
    dialog_state->contextMenuY = y;
    cmd.consume();
  }
};

// Handle 'file_dialog_set_path path' - queues a path for the next native file dialog call
// This allows E2E tests to control what open_file/save_file return in test mode.
struct HandleFileDialogSetPathCommand : System<testing::PendingE2ECommand> {
  virtual void for_each_with(Entity &, testing::PendingE2ECommand &cmd,
                             float) override {
    if (cmd.is_consumed() || !cmd.is("file_dialog_set_path"))
      return;
    if (!cmd.has_args(1)) {
      cmd.fail("file_dialog_set_path requires a file path");
      return;
    }

    file_dialog::set_test_path(cmd.arg(0));
    cmd.consume();
  }
};

// Register all app-specific commands
inline void register_app_commands(
    SystemManager &sm,
    ecs::DocumentComponent *doc_comp,
    ecs::MenuComponent *menu_comp,
    ecs::DialogState *dialog_state
) {
  // Menu commands
  auto menu_open = std::make_unique<HandleMenuOpenCommand>();
  menu_open->menu_comp = menu_comp;
  sm.register_update_system(std::move(menu_open));
  
  auto menu_select = std::make_unique<HandleMenuSelectCommand>();
  menu_select->menu_comp = menu_comp;
  sm.register_update_system(std::move(menu_select));
  
  // Document commands
  auto doc_dump = std::make_unique<HandleDocumentDumpCommand>();
  doc_dump->doc_comp = doc_comp;
  sm.register_update_system(std::move(doc_dump));
  
  auto outline_click = std::make_unique<HandleOutlineClickCommand>();
  outline_click->doc_comp = doc_comp;
  sm.register_update_system(std::move(outline_click));

  auto open_file = std::make_unique<HandleOpenFileCommand>();
  open_file->doc_comp = doc_comp;
  sm.register_update_system(std::move(open_file));

  auto mouse_up = std::make_unique<HandleMouseUpCommand>();
  sm.register_update_system(std::move(mouse_up));

  auto load_tmpl = std::make_unique<HandleLoadTemplateCommand>();
  load_tmpl->doc_comp = doc_comp;
  sm.register_update_system(std::move(load_tmpl));

  auto set_tw = std::make_unique<HandleSetTabWidthCommand>();
  set_tw->doc_comp = doc_comp;
  sm.register_update_system(std::move(set_tw));

  auto add_cmt = std::make_unique<HandleAddCommentCommand>();
  add_cmt->doc_comp = doc_comp;
  sm.register_update_system(std::move(add_cmt));

  auto replace_txt = std::make_unique<HandleReplaceTextCommand>();
  replace_txt->doc_comp = doc_comp;
  sm.register_update_system(std::move(replace_txt));

  auto set_find = std::make_unique<HandleSetFindTermCommand>();
  set_find->dialog_state = dialog_state;
  sm.register_update_system(std::move(set_find));

  auto file_dialog_set = std::make_unique<HandleFileDialogSetPathCommand>();
  sm.register_update_system(std::move(file_dialog_set));

  auto right_click = std::make_unique<HandleRightClickCommand>();
  right_click->dialog_state = dialog_state;
  sm.register_update_system(std::move(right_click));
}

} // namespace e2e_commands

