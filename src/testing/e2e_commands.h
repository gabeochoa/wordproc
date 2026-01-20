// App-specific E2E Command Handlers for wordproc
// Menu, document, and outline commands specific to this application
#pragma once

#include "../ecs/components.h"

#include <afterhours/src/plugins/e2e_testing/e2e_testing.h>

#include <fstream>

namespace e2e_commands {

using namespace afterhours;

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

    const auto &menu_name = cmd.arg(0);
    
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
    if (!cmd.has_args(1)) {
      cmd.fail("menu_select requires item name");
      return;
    }
    if (!menu_comp) {
      cmd.fail("menu_comp not set - was E2EConfig.menu_comp passed?");
      return;
    }

    const auto &item_name = cmd.arg(0);
    
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
      file << doc_comp->buffer.getText();
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

// Register all app-specific commands
inline void register_app_commands(
    SystemManager &sm,
    ecs::DocumentComponent *doc_comp,
    ecs::MenuComponent *menu_comp
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
}

} // namespace e2e_commands

