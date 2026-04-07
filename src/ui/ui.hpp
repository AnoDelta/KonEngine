#pragma once

#include "ui_element.hpp"
#include <string>
#include <functional>

// ---------------------------------------------------------------------------
// UI Manager -- screen-space UI system
//
// Usage:
//   1. Create elements: UIAddButton(), UIAddLabel(), UIAddPanel()
//   2. Each frame: call UIUpdate() then UIDrawAll() AFTER scene.Draw()
//   3. Check UIWantsInput() before processing world clicks
// ---------------------------------------------------------------------------

// Create elements (returns pointer for immediate configuration)
UIButton* UIAddButton(const std::string& id, const std::string& text, float x, float y);
UILabel*  UIAddLabel(const std::string& id, const std::string& text, float x, float y,
                     int fontSize = 20, Color color = WHITE);
UIPanel*  UIAddPanel(const std::string& id, float x, float y, float w, float h);

// Add a child element to a panel (child position becomes relative to panel)
void UIPanelAddChild(const std::string& panelId, const std::string& childId);

// Lookup element by ID
UIElement* UIGetElement(const std::string& id);

// Remove element by ID
void UIRemove(const std::string& id);

// Clear all UI elements
void UIClear();

// Per-frame update -- reads mouse state, runs hit tests
// Call BEFORE UIDrawAll()
void UIUpdate();

// Draw all visible elements in screen-space
// Call AFTER scene.Draw() / after EndCamera2D()
void UIDrawAll();

// Returns true if any UI element was hovered this frame
// Check this before processing world clicks
bool UIWantsInput();

// Set onClick callback for a button by ID
void UIOnClick(const std::string& id, std::function<void()> callback);
