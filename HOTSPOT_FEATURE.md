# Hotspot Display Feature

> **Updated: 2026-03-04** — Configuration moved from Global Options to Game Options (per-game); `HotspotInfo` now carries a `HotspotType` enum; Touche implementation extended with dirty-checking and snapshot mechanism; keymap action documented as `keymap_engine-default_HOTS`.

## Introduction

Before reviewing the technical implementation, I'd like to open a discussion with the ScummVM community about whether this feature aligns with ScummVM's vision and goals. While I believe visual hotspot indicators can improve accessibility and user experience, especially for newer players or those unfamiliar with point-and-click adventure games, I understand this may be a contentious addition that changes the nature of how these games are played.

I'm interested in hearing the community's thoughts on whether such a feature belongs in ScummVM, and if so, what considerations should guide its implementation and availability to users.

## Overview

This Pull Request adds infrastructure for displaying interactive hotspot markers in games. When enabled, users can press a configurable key (default 'h') to show visual markers at clickable locations, with optional text labels showing object names.

The implementation provides:
- Base infrastructure in the Engine class for hotspot support
- A dedicated Graphics::HotspotRenderer module for rendering markers
- Custom engine action event for key remapping
- Per-game GUI configuration options for marker style and display preferences
- Initial implementation for the Touche engine

Touche engine was selected as a first because it is a single game so smaller scope for testing (also I want to play this game). I have already partially implemented Scumm engine support but in that case there is need for few per game fixes and adjustments. Before pushing it I want to first have it polished for single engine.

## Architecture

### Core Components

#### 1. Engine Base Class (`engines/engine.h`, `engines/engine.cpp`)

The Engine class provides three virtual methods that engines can override:

```cpp
virtual void setShowHotspots(bool show);
virtual void getHotspotPositions(Common::Array<Graphics::HotspotInfo> &hotspots);
virtual void drawHotspots();
```

- **`showHotspots(bool show)`**: Called with `true` while the hotspot key is held and `false` when it is released. Base implementation manages overlay visibility.

- **`getHotspotPositions()`**: Engines override this to provide hotspot information (position, name, and type) in game screen coordinates. Default implementation returns empty array.

- **`drawHotspots()`**: Called during the rendering loop. Base implementation handles coordinate conversion, overlay management, and delegates to HotspotRenderer.

#### 2. Graphics::HotspotRenderer (`graphics/hotspot_renderer.h`, `graphics/hotspot_renderer.cpp`)

Dedicated rendering module that draws hotspot markers on the overlay surface with:
- Three marker styles: Point (circular), Square (rectangular outline), Crosshair
- Semi-transparent rendering with glow effects
- Optional text labels with semi-transparent background boxes
- Automatic coordinate scaling from game resolution to overlay resolution

`HotspotInfo` carries a `HotspotType` enum field (`kHotspotDefault`, and engine-specific values) so engines can classify hotspots and renderers can style them differently in the future.

#### 3. Event System (`common/events.h`, `backends/events/default/default-events.cpp`)

A dedicated `EVENT_HOTSPOTS_SHOW` / `EVENT_HOTSPOTS_HIDE` event pair allows the hotspot key to be remapped through ScummVM's keymapper system. The keymap action sends `EVENT_HOTSPOTS_SHOW` directly via `Action::setEvent()`; because the pair is registered in `Keymapper::convertStartToEnd()`, the keymapper emits `SHOW` on key press and `HIDE` on release, giving hold-to-show behavior. The default event handler checks the `enable_hotspots` configuration and shows/hides hotspot display on the active engine.

The standard action `kStandardActionToggleHotspots` ("HOTS") is registered in the default engine keymap so all engines supporting the feature automatically expose a remappable binding.

#### 4. Configuration (`gui/editgamedialog.h`, `gui/editgamedialog.cpp`)

The Game Options dialog provides three per-game settings under the **Misc** tab:
- `enable_hotspots` (bool): Master enable/disable
- `hotspot_marker` (int): Marker style (0=Point, 1=Square, 2=Crosshair)
- `show_hotspot_text` (bool): Show/hide text labels

Settings are stored per-game in the configuration file, allowing different preferences for each title.

## Example: Touche Engine Implementation

### Code Implementation

The Touche engine implementation demonstrates how engines can adopt this feature:

**1. Override `getHotspotPositions()` in `touche.h`:**
```cpp
void getHotspotPositions(Common::Array<Graphics::HotspotInfo> &hotspots) override;
```

**2. Implement the method in `touche.cpp`:**
```cpp
void ToucheEngine::getHotspotPositions(Common::Array<Graphics::HotspotInfo> &hotspots) {
    // Check if interface is hidden
    if (_flagsTable[618] != 0)
        return;

    // Get current viewport scroll
    int scrollX = _flagsTable[614];
    int scrollY = _flagsTable[615];

    // Iterate through program hitboxes
    for (uint i = 0; i < _programHitBoxTable.size(); ++i) {
        const ProgramHitBoxData &hitBox = _programHitBoxTable[i];

        // Skip disabled/invalid hitboxes
        if (hitBox.item & 0x1000) break;
        if (hitBox.item & 0x2000) continue;

        // Calculate center position and convert to screen coordinates
        // ... (engine-specific logic)

        // Add to hotspots array
        hotspots.push_back(::Graphics::HotspotInfo(
            Common::Point(screenX, screenY),
            objectName
        ));
    }
}
```

**3. Call `drawHotspots()` in the main rendering loop:**
```cpp
void ToucheEngine::runCycle() {
    // ... existing rendering code ...
    updateDirtyScreenAreas();
    drawHotspots();  // Add this call
    // ... rest of game loop ...
}
```

### Performance: Dirty-Checking and Snapshot

The Touche implementation avoids rebuilding the hotspot list every frame by maintaining a snapshot of the game state that was used for the last render. Two helper methods manage this:

- **`hotspotDirty()`**: Compares current game state (scroll position, episode, hitbox table, key character positions) against the snapshot. Returns `true` only when something changed, triggering a rebuild.
- **`rebuildHotspotSnapshot()`**: Captures the current state into `_hotspotSnapshot` so subsequent frames can skip the rebuild.

This pattern is recommended for other engines with large hitbox tables to keep the overlay rendering efficient.

## How to Implement in Your Engine

Follow these steps to add hotspot support to an engine:

### Step 1: Include Required Headers

Add to your engine's main cpp file:
```cpp
#include "graphics/hotspot_renderer.h"
```

### Step 2: Override getHotspotPositions()

In your engine header file:
```cpp
void getHotspotPositions(Common::Array<Graphics::HotspotInfo> &hotspots) override;
```

In your engine implementation:
```cpp
void YourEngine::getHotspotPositions(Common::Array<Graphics::HotspotInfo> &hotspots) {
    // Iterate through your engine's interactive objects
    for (each interactive object) {
        // Skip non-interactive or hidden objects
        if (!object.isInteractive())
            continue;

        // Get object position in game screen coordinates
        int screenX = object.x;
        int screenY = object.y;

        // Get object name (can be empty string)
        Common::String name = object.getName();

        // Add to array
        hotspots.push_back(Graphics::HotspotInfo(
            Common::Point(screenX, screenY),
            name
        ));
    }
}
```

### Step 3: Call drawHotspots() in Rendering Loop

Add to your main rendering function:
```cpp
void YourEngine::render() {
    // ... your existing rendering code ...

    drawHotspots();  // Add after main scene rendering

    // ... rest of rendering ...
}
```

### Important Considerations

- **Coordinates**: Provide positions in game screen coordinates (after viewport scrolling). The base implementation handles conversion to overlay coordinates.

- **Performance**: `getHotspotPositions()` is called every frame while hotspots are visible. Keep it efficient. Consider the dirty-checking snapshot pattern used in Touche for engines with many hotspots.

- **Object Names**: Names can be empty strings if unavailable. The renderer will only show text labels for non-empty names when `show_hotspot_text` is enabled.

- **Visibility**: Only include currently visible and interactive objects. Skip objects that are:
  - Off-screen
  - Hidden by game state
  - Not interactive (scenery, etc.)
  - Behind other objects (if your engine tracks occlusion)

## Testing

Unit tests are provided in:
- `test/graphics/hotspot_info.h` - Tests for Graphics::HotspotInfo struct
- `test/common/events.h` - Tests for engine action events

Run tests with:
```bash
make test
```

## Configuration

Users can configure the feature per-game in **Game Options > Misc** tab:
- **Enable hotspot display**: Master toggle
- **Hotspot marker**: Visual style (Point/Square/Crosshair)
- **Show hotspot labels**: Display object names

The hotspot toggle key can be remapped in the Keymaps settings (default: 'h').

## Documentation

User documentation has been added to:
- `doc/docportal/settings/keymaps.rst` — Hotspot toggle action documented as `keymap_engine-default_HOTS` under the Default game keymap section
- `doc/docportal/settings/misc.rst` — Configuration options (`enable_hotspots`, `hotspot_marker`, `show_hotspot_text`)
