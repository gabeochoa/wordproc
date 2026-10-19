#include "../src/editor/borders_shading.h"
#include "../src/editor/text_buffer.h"

#include "catch2/catch.hpp"

TEST_CASE("Borders and Shading palette", "[borders_shading]") {
    using namespace borders_shading;
    SECTION("None + 8 highlight colors") {
        REQUIRE(kShadingCount == 9);
        REQUIRE(shadingColorForIndex(0) == HighlightColors::None);
        REQUIRE(shadingColorForIndex(0).isNone());
        REQUIRE(shadingColorForIndex(1) == HighlightColors::Yellow);
        REQUIRE(shadingColorForIndex(8) == HighlightColors::Gray);
        REQUIRE(std::string(kShadingNames[0]) == "None");
        REQUIRE(std::string(kShadingNames[1]) == "Yellow");
        REQUIRE(std::string(kShadingNames[8]) == "Gray");
    }
    SECTION("out-of-range index reads as None, clamps into range") {
        REQUIRE(shadingColorForIndex(-1).isNone());
        REQUIRE(shadingColorForIndex(99).isNone());
        REQUIRE(clampShadingIndex(-3) == 0);
        REQUIRE(clampShadingIndex(99) == kShadingCount - 1);
        REQUIRE(clampShadingIndex(4) == 4);
    }
    SECTION("edge bitmask covers all four edges") {
        REQUIRE(kEdgeAll ==
                (kEdgeTop | kEdgeBottom | kEdgeLeft | kEdgeRight));
        REQUIRE((kEdgeAll & kEdgeTop) != 0);
        REQUIRE((kEdgeAll & kEdgeBottom) != 0);
        REQUIRE((kEdgeAll & kEdgeLeft) != 0);
        REQUIRE((kEdgeAll & kEdgeRight) != 0);
    }
}

TEST_CASE("Borders and Shading predicates", "[borders_shading]") {
    using namespace borders_shading;
    SECTION("a border needs Single style and at least one edge") {
        REQUIRE(hasParagraphBorder(kEdgeAll, kBorderStyleSingle));
        REQUIRE(hasParagraphBorder(kEdgeTop, kBorderStyleSingle));
        REQUIRE_FALSE(hasParagraphBorder(0, kBorderStyleSingle));
        REQUIRE_FALSE(hasParagraphBorder(kEdgeAll, kBorderStyleNone));
    }
    SECTION("shading is on for any non-None index") {
        REQUIRE_FALSE(hasParagraphShading(0));
        REQUIRE(hasParagraphShading(1));
        REQUIRE_FALSE(hasParagraphShading(99));
    }
    SECTION("same triple merges; any field difference breaks the merge") {
        REQUIRE(sameBorderShading(kEdgeAll, kBorderStyleSingle, 1, kEdgeAll,
                                  kBorderStyleSingle, 1));
        REQUIRE_FALSE(sameBorderShading(kEdgeTop, kBorderStyleSingle, 1,
                                        kEdgeAll, kBorderStyleSingle, 1));
        REQUIRE_FALSE(sameBorderShading(kEdgeAll, kBorderStyleNone, 1,
                                        kEdgeAll, kBorderStyleSingle, 1));
        REQUIRE_FALSE(sameBorderShading(kEdgeAll, kBorderStyleSingle, 1,
                                        kEdgeAll, kBorderStyleSingle, 2));
    }
    SECTION("merged group draws no internal horizontal edge") {
        // Standalone line: every set edge draws.
        EdgesToDraw alone =
            edgesToDraw(kEdgeAll, kBorderStyleSingle, false, false);
        REQUIRE(alone.top);
        REQUIRE(alone.bottom);
        REQUIRE(alone.left);
        REQUIRE(alone.right);
        // Middle of a group: top/bottom suppressed, sides kept.
        EdgesToDraw middle =
            edgesToDraw(kEdgeAll, kBorderStyleSingle, true, true);
        REQUIRE_FALSE(middle.top);
        REQUIRE_FALSE(middle.bottom);
        REQUIRE(middle.left);
        REQUIRE(middle.right);
        // First of a group: top draws, bottom suppressed.
        EdgesToDraw first =
            edgesToDraw(kEdgeAll, kBorderStyleSingle, false, true);
        REQUIRE(first.top);
        REQUIRE_FALSE(first.bottom);
        // Last of a group: bottom draws, top suppressed.
        EdgesToDraw last =
            edgesToDraw(kEdgeAll, kBorderStyleSingle, true, false);
        REQUIRE_FALSE(last.top);
        REQUIRE(last.bottom);
        // No border: nothing draws even with edges set.
        EdgesToDraw none =
            edgesToDraw(kEdgeAll, kBorderStyleNone, false, false);
        REQUIRE_FALSE(none.top);
        REQUIRE_FALSE(none.bottom);
        REQUIRE_FALSE(none.left);
        REQUIRE_FALSE(none.right);
        // Partial edges: only the set sides draw.
        EdgesToDraw topOnly =
            edgesToDraw(kEdgeTop, kBorderStyleSingle, false, false);
        REQUIRE(topOnly.top);
        REQUIRE_FALSE(topOnly.bottom);
        REQUIRE_FALSE(topOnly.left);
        REQUIRE_FALSE(topOnly.right);
    }
}

TEST_CASE("ParagraphFormat borders and shading round-trip + undo",
          "[borders_shading][text_buffer]") {
    TextBuffer buffer;
    buffer.setText("Boxed\nPlain");

    auto applyTo = [&](std::size_t first, std::size_t last,
                       ParagraphFormat next) {
        std::vector<ParagraphFormatChange> changes;
        for (std::size_t row = first; row <= last; ++row) {
            ParagraphFormat old = buffer.lineParagraphFormat(row);
            buffer.setLineParagraphFormat(row, next);
            changes.push_back({row, old, next});
        }
        buffer.commitParagraphFormat(std::move(changes));
    };

    SECTION("boxed + shaded paragraph undoes in one step, keeps indents") {
        // The paragraph already has an indent; the dialog OK must
        // preserve it (it only touches the border/shading fields).
        ParagraphFormat indented;
        indented.indents = {36, 0, 0};
        buffer.setLineParagraphFormat(0, indented);
        {
            ParagraphFormat old = buffer.lineParagraphFormat(0);
            ParagraphFormat merged = old;
            merged.borderEdges = borders_shading::kEdgeAll;
            merged.borderStyle = borders_shading::kBorderStyleSingle;
            merged.shadingColorIndex = 1;  // Yellow
            buffer.setLineParagraphFormat(0, merged);
            buffer.commitParagraphFormat({{0, old, merged}});
        }

        ParagraphFormat got = buffer.lineParagraphFormat(0);
        REQUIRE(got.borderEdges == borders_shading::kEdgeAll);
        REQUIRE(got.borderStyle == borders_shading::kBorderStyleSingle);
        REQUIRE(got.shadingColorIndex == 1);
        REQUIRE(got.indents.left == 36);
        REQUIRE(buffer.lineParagraphFormat(1) == ParagraphFormat{});

        buffer.undo();
        ParagraphFormat undone = buffer.lineParagraphFormat(0);
        REQUIRE(undone.borderEdges == 0);
        REQUIRE(undone.borderStyle == borders_shading::kBorderStyleNone);
        REQUIRE(undone.shadingColorIndex == 0);
        REQUIRE(undone.indents.left == 36);
        buffer.redo();
        REQUIRE(buffer.lineParagraphFormat(0).borderEdges ==
                borders_shading::kEdgeAll);
        REQUIRE(buffer.lineParagraphFormat(0).shadingColorIndex == 1);
    }

    SECTION("one OK across two paragraphs undoes in one step") {
        ParagraphFormat next;
        next.borderEdges = borders_shading::kEdgeTop;
        next.borderStyle = borders_shading::kBorderStyleSingle;
        next.shadingColorIndex = 3;  // Cyan
        applyTo(0, 1, next);
        REQUIRE(buffer.lineParagraphFormat(0) == next);
        REQUIRE(buffer.lineParagraphFormat(1) == next);
        buffer.undo();
        REQUIRE(buffer.lineParagraphFormat(0) == ParagraphFormat{});
        REQUIRE(buffer.lineParagraphFormat(1) == ParagraphFormat{});
    }

    SECTION("apply clamps edges, style, and shading index") {
        ParagraphFormat next;
        next.borderEdges = 0xFF;
        next.borderStyle = 7;
        next.shadingColorIndex = 99;
        buffer.setLineParagraphFormat(0, next);
        ParagraphFormat got = buffer.lineParagraphFormat(0);
        REQUIRE(got.borderEdges == borders_shading::kEdgeAll);
        REQUIRE(got.borderStyle == borders_shading::kBorderStyleNone);
        REQUIRE(got.shadingColorIndex ==
                borders_shading::kShadingCount - 1);
    }

    SECTION("border + shading + right indent survive a line-index rebuild") {
        ParagraphFormat next;
        next.indents = {0, 0, 36};
        next.borderEdges = borders_shading::kEdgeAll;
        next.borderStyle = borders_shading::kBorderStyleSingle;
        next.shadingColorIndex = 1;
        buffer.setLineParagraphFormat(0, next);
        // Deleting a selection rebuilds the line index.
        buffer.setCaret({1, 0});
        buffer.setSelectionAnchor({1, 0});
        buffer.setCaret({1, 1});
        buffer.updateSelectionToCaret();
        REQUIRE(buffer.deleteSelection());
        ParagraphFormat got = buffer.lineParagraphFormat(0);
        REQUIRE(got.borderEdges == borders_shading::kEdgeAll);
        REQUIRE(got.borderStyle == borders_shading::kBorderStyleSingle);
        REQUIRE(got.shadingColorIndex == 1);
        REQUIRE(got.indents.right == 36);
    }

    SECTION("a new paragraph from Enter starts unbordered, unshaded") {
        ParagraphFormat next;
        next.borderEdges = borders_shading::kEdgeAll;
        next.borderStyle = borders_shading::kBorderStyleSingle;
        next.shadingColorIndex = 1;
        buffer.setLineParagraphFormat(0, next);
        buffer.setCaret({0, 5});
        buffer.insertChar('\n');
        REQUIRE(buffer.lineCount() == 3);
        REQUIRE(buffer.lineParagraphFormat(1) == ParagraphFormat{});
        // The original paragraph keeps its border + shading.
        REQUIRE(buffer.lineParagraphFormat(0).borderEdges ==
                borders_shading::kEdgeAll);
    }
}
