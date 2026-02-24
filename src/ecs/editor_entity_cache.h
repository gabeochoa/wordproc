#pragma once

#include <afterhours/ah.h>
#include "components.h"

namespace ecs {

// Caches pointers to editor-entity components so UI systems that query
// UIContext (a different entity) don't need repeated EntityQuery lookups.
// Safe because the editor entity lives for the entire application lifetime.
struct EditorEntityCache {
    DocumentComponent* doc = nullptr;
    LayoutComponent* layout = nullptr;
    MenuComponent* menu = nullptr;
    DialogState* dialogs = nullptr;
    ToolbarComponent* toolbar = nullptr;

    bool resolved() const { return doc != nullptr; }

    void resolve() {
        if (resolved()) return;
        auto entities = afterhours::EntityQuery({.force_merge = true})
                            .whereHasComponent<DocumentComponent>()
                            .gen();
        if (entities.empty()) return;
        auto& e = entities[0].get();
        doc = &e.get<DocumentComponent>();
        layout = &e.get<LayoutComponent>();
        menu = &e.get<MenuComponent>();
        dialogs = &e.get<DialogState>();
        toolbar = &e.get<ToolbarComponent>();
    }
};

}  // namespace ecs
