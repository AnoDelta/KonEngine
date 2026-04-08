#include "ui.hpp"
#include "../input/input.hpp"
#include "../window/window.hpp"
#include <vector>
#include <memory>
#include <algorithm>

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static struct UIState {
    std::vector<std::unique_ptr<UIElement>> elements;
    bool inputConsumed = false;
} g_ui;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static UIElement* FindElement(const std::string& id) {
    for (auto& e : g_ui.elements)
        if (e->id == id) return e.get();
    return nullptr;
}

// ---------------------------------------------------------------------------
// Create elements
// ---------------------------------------------------------------------------
UIButton* UIAddButton(const std::string& id, const std::string& text, float x, float y) {
    auto btn = std::make_unique<UIButton>(id);
    btn->text = text;
    btn->x = x;
    btn->y = y;
    UIButton* ptr = btn.get();
    g_ui.elements.push_back(std::move(btn));
    return ptr;
}

UILabel* UIAddLabel(const std::string& id, const std::string& text, float x, float y,
                    int fontSize, Color color) {
    auto lbl = std::make_unique<UILabel>(id);
    lbl->text = text;
    lbl->x = x;
    lbl->y = y;
    lbl->fontSize = fontSize;
    lbl->color = color;
    UILabel* ptr = lbl.get();
    g_ui.elements.push_back(std::move(lbl));
    return ptr;
}

UIPanel* UIAddPanel(const std::string& id, float x, float y, float w, float h) {
    auto panel = std::make_unique<UIPanel>(id);
    panel->x = x;
    panel->y = y;
    panel->width = w;
    panel->height = h;
    UIPanel* ptr = panel.get();
    g_ui.elements.push_back(std::move(panel));
    return ptr;
}

UIImage* UIAddImage(const std::string& id, Texture tex, float x, float y,
                    float w, float h) {
    auto img = std::make_unique<UIImage>(id);
    img->x = x;
    img->y = y;
    img->SetTexture(tex);
    if (w > 0) img->width = w;
    if (h > 0) img->height = h;
    UIImage* ptr = img.get();
    g_ui.elements.push_back(std::move(img));
    return ptr;
}

UITextBox* UIAddTextBox(const std::string& id, const std::string& text,
                        float x, float y, float w, float h,
                        bool typewriter, float charsPerSec) {
    auto tb = std::make_unique<UITextBox>(id);
    tb->text = text;
    tb->x = x; tb->y = y;
    tb->width = w; tb->height = h;
    tb->typewriter = typewriter;
    tb->charsPerSec = charsPerSec;
    if (!typewriter) tb->visibleChars = (int)text.size();
    UITextBox* ptr = tb.get();
    g_ui.elements.push_back(std::move(tb));
    return ptr;
}

void UIPanelAddChild(const std::string& panelId, const std::string& childId) {
    UIElement* el = FindElement(panelId);
    if (!el) return;
    UIPanel* panel = dynamic_cast<UIPanel*>(el);
    if (panel) panel->AddChild(childId);
}

UIElement* UIGetElement(const std::string& id) {
    return FindElement(id);
}

void UIRemove(const std::string& id) {
    g_ui.elements.erase(
        std::remove_if(g_ui.elements.begin(), g_ui.elements.end(),
            [&](const std::unique_ptr<UIElement>& e) { return e->id == id; }),
        g_ui.elements.end());
}

void UIClear() {
    g_ui.elements.clear();
    g_ui.inputConsumed = false;
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------
void UIUpdate() {
    g_ui.inputConsumed = false;

    float mouseX = GetGameMouseX();
    float mouseY = GetGameMouseY();
    bool mouseDown    = IsMouseButtonDown(Mouse::Left);
    bool mousePressed = IsMouseButtonPressed(Mouse::Left);

    // Sort by zOrder descending (highest first = topmost gets input first)
    std::vector<UIElement*> sorted;
    sorted.reserve(g_ui.elements.size());
    for (auto& e : g_ui.elements) sorted.push_back(e.get());
    std::sort(sorted.begin(), sorted.end(),
        [](UIElement* a, UIElement* b) { return a->zOrder > b->zOrder; });

    bool clickConsumed = false;

    for (UIElement* el : sorted) {
        if (!el->visible) continue;

        // Check if this is a panel -- apply offset to children
        UIPanel* panel = dynamic_cast<UIPanel*>(el);
        if (panel) {
            for (auto& childId : panel->childIds) {
                UIElement* child = FindElement(childId);
                if (!child || !child->visible) continue;

                // Temporarily offset child position by panel
                float origX = child->x, origY = child->y;
                child->x += panel->x;
                child->y += panel->y;

                child->Update(mouseX, mouseY, mouseDown, clickConsumed ? false : mousePressed);

                if (child->Contains(mouseX, mouseY)) {
                    g_ui.inputConsumed = true;
                    clickConsumed = true;
                }

                // Restore local position
                child->x = origX;
                child->y = origY;
            }
        }

        el->Update(mouseX, mouseY, mouseDown, clickConsumed ? false : mousePressed);

        if (el->Contains(mouseX, mouseY)) {
            g_ui.inputConsumed = true;
            clickConsumed = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Draw all elements
// ---------------------------------------------------------------------------
void UIDrawAll() {
    // Sort by zOrder ascending (lowest first = drawn first = behind)
    std::vector<UIElement*> sorted;
    sorted.reserve(g_ui.elements.size());

    // Collect which elements are panel children (skip standalone draw)
    std::vector<std::string> panelChildIds;
    for (auto& e : g_ui.elements) {
        UIPanel* panel = dynamic_cast<UIPanel*>(e.get());
        if (panel) {
            for (auto& cid : panel->childIds)
                panelChildIds.push_back(cid);
        }
        sorted.push_back(e.get());
    }

    std::sort(sorted.begin(), sorted.end(),
        [](UIElement* a, UIElement* b) { return a->zOrder < b->zOrder; });

    for (UIElement* el : sorted) {
        if (!el->visible) continue;

        // Check if this element is a panel child (drawn by panel, not standalone)
        bool isChild = false;
        for (auto& cid : panelChildIds) {
            if (el->id == cid) { isChild = true; break; }
        }
        if (isChild) continue;

        el->Draw();

        // If it's a panel, draw children with offset
        UIPanel* panel = dynamic_cast<UIPanel*>(el);
        if (panel) {
            for (auto& childId : panel->childIds) {
                UIElement* child = FindElement(childId);
                if (!child || !child->visible) continue;

                float origX = child->x, origY = child->y;
                child->x += panel->x;
                child->y += panel->y;
                child->Draw();
                child->x = origX;
                child->y = origY;
            }
        }
    }
}

bool UIWantsInput() {
    return g_ui.inputConsumed;
}

void UIOnClick(const std::string& id, std::function<void()> callback) {
    UIElement* el = FindElement(id);
    if (!el) return;
    UIButton* btn = dynamic_cast<UIButton*>(el);
    if (btn) btn->onClick = callback;
}

void UIConnect(const std::string& id, const std::string& signal,
               std::function<void()> callback) {
    UIElement* el = FindElement(id);
    if (el) el->Connect(signal, callback);
}
