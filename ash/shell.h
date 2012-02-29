// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_H_
#define ASH_SHELL_H_
#pragma once

#include <utility>
#include <vector>

#include "ash/ash_export.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "ui/gfx/size.h"

class CommandLine;

namespace aura {
class EventFilter;
class RootWindow;
class Window;
}
namespace gfx {
class Point;
class Rect;
}
namespace ui {
class Layer;
}
namespace views {
class NonClientFrameView;
class Widget;
}

namespace ash {

class AcceleratorController;
class AudioController;
class BrightnessController;
class Launcher;
class NestedDispatcherController;
class PowerButtonController;
class ShellDelegate;
class SystemTrayDelegate;
class SystemTray;
class VideoDetector;
class WindowCycleController;

namespace internal {
class ActivationController;
class AcceleratorFilter;
class AppList;
class DragDropController;
class FocusCycler;
class InputMethodEventFilter;
class PartialScreenshotEventFilter;
class RootWindowEventFilter;
class RootWindowLayoutManager;
class ShadowController;
class ShelfLayoutManager;
class StackingController;
class TooltipController;
class VisibilityController;
class WindowModalityController;
class WorkspaceController;
}

// Shell is a singleton object that presents the Shell API and implements the
// RootWindow's delegate interface.
//
// Upon creation, the Shell sets itself as the RootWindow's delegate, which
// takes ownership of the Shell.
class ASH_EXPORT Shell {
 public:
  // In compact window mode we fill the screen with a single maximized window,
  // similar to ChromeOS R17 and earlier.  In overlapping mode we have draggable
  // windows.  In managed mode the workspace arranges windows for the user.
  enum WindowMode {
    MODE_COMPACT,
    MODE_MANAGED,
    MODE_OVERLAPPING,
  };

  enum BackgroundMode {
    BACKGROUND_IMAGE,
    BACKGROUND_SOLID_COLOR
  };

  enum Direction {
    FORWARD,
    BACKWARD
  };

  // Accesses private data from a Shell for testing.
  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(Shell* shell);

    WindowMode ComputeWindowMode(CommandLine* cmd) const;
    internal::RootWindowLayoutManager* root_window_layout();

   private:
    Shell* shell_;  // not owned

    DISALLOW_COPY_AND_ASSIGN(TestApi);
  };

  // A shell must be explicitly created so that it can call |Init()| with the
  // delegate set. |delegate| can be NULL (if not required for initialization).
  static Shell* CreateInstance(ShellDelegate* delegate);

  // Should never be called before |CreateInstance()|.
  static Shell* GetInstance();

  static void DeleteInstance();

  // Get the singleton RootWindow used by the Shell.
  static aura::RootWindow* GetRootWindow();

  const gfx::Size& compact_status_area_offset() const {
    return compact_status_area_offset_;
  }

  BackgroundMode desktop_background_mode() const {
    return desktop_background_mode_;
  }

  aura::Window* GetContainer(int container_id);
  const aura::Window* GetContainer(int container_id) const;

  // Adds or removes |filter| from the RootWindowEventFilter.
  void AddRootWindowEventFilter(aura::EventFilter* filter);
  void RemoveRootWindowEventFilter(aura::EventFilter* filter);
  size_t GetRootWindowEventFilterCount() const;

  // Shows the background menu over |widget|.
  void ShowBackgroundMenu(views::Widget* widget, const gfx::Point& location);

  // Toggles app list.
  void ToggleAppList();

  // Sets the desktop background mode.
  void SetDesktopBackgroundMode(BackgroundMode mode);

  // Returns true if the screen is locked.
  bool IsScreenLocked() const;

  // Returns true if a modal dialog window is currently open.
  bool IsModalWindowOpen() const;

  // See enum WindowMode for details.
  bool IsWindowModeCompact() const { return window_mode_ == MODE_COMPACT; }

  // Sets the offset between the corner of the status area and the corner of the
  // screen when we're using the compact window mode.
  void SetCompactStatusAreaOffset(gfx::Size& offset);

  // Creates a default views::NonClientFrameView for use by windows in the
  // Ash environment.
  views::NonClientFrameView* CreateDefaultNonClientFrameView(
      views::Widget* widget);

  // Rotate focus through containers that can recieve focus.
  void RotateFocus(Direction direction);

#if !defined(OS_MACOSX)
  AcceleratorController* accelerator_controller() {
    return accelerator_controller_.get();
  }
#endif  // !defined(OS_MACOSX)

  internal::RootWindowEventFilter* root_filter() {
    return root_filter_;
  }
  internal::TooltipController* tooltip_controller() {
    return tooltip_controller_.get();
  }
  internal::PartialScreenshotEventFilter* partial_screenshot_filter() {
    return partial_screenshot_filter_.get();
  }
  PowerButtonController* power_button_controller() {
    return power_button_controller_.get();
  }
  VideoDetector* video_detector() {
    return video_detector_.get();
  }
  WindowCycleController* window_cycle_controller() {
    return window_cycle_controller_.get();
  }
  AudioController* audio_controller() const {
    return audio_controller_;
  }
  BrightnessController* brightness_controller() const {
    return brightness_controller_;
  }

  ShellDelegate* delegate() { return delegate_.get(); }
  SystemTrayDelegate* tray_delegate() { return tray_delegate_.get(); }

  Launcher* launcher() { return launcher_.get(); }

  internal::ShelfLayoutManager* shelf() const { return shelf_; }

  SystemTray* tray() const { return tray_.get(); }

  // Made available for tests.
  internal::ShadowController* shadow_controller() {
    return shadow_controller_.get();
  }

  static void set_compact_window_mode_for_test(bool compact) {
    compact_window_mode_for_test_ = compact;
  }

 private:
  typedef std::pair<aura::Window*, gfx::Rect> WindowAndBoundsPair;

  explicit Shell(ShellDelegate* delegate);
  virtual ~Shell();

  void Init();

  // Returns the appropriate window mode to use based on the |command_line|
  // and |compact_window_mode_for_test_|.
  WindowMode ComputeWindowMode(CommandLine* command_line) const;

  // Initializes or re-initializes the layout managers and event filters needed
  // to support a given window mode and cleans up the unneeded ones.
  void SetupCompactWindowMode();
  void SetupNonCompactWindowMode();

  // Sets the LayoutManager of the container with the specified id to NULL. This
  // has the effect of deleting the current LayoutManager.
  void ResetLayoutManager(int container_id);

  static Shell* instance_;

  // Window mode is computed at shell initialization time, so allow it to be
  // overridden without modifying the global command line.
  static bool compact_window_mode_for_test_;

  internal::RootWindowEventFilter* root_filter_;  // not owned

  std::vector<WindowAndBoundsPair> to_restore_;

#if !defined(OS_MACOSX)
  scoped_ptr<NestedDispatcherController> nested_dispatcher_controller_;

  scoped_ptr<AcceleratorController> accelerator_controller_;
#endif  // !defined(OS_MACOSX)

  scoped_ptr<ShellDelegate> delegate_;
  scoped_ptr<SystemTrayDelegate> tray_delegate_;

  scoped_ptr<Launcher> launcher_;

  scoped_ptr<internal::AppList> app_list_;

  scoped_ptr<internal::StackingController> stacking_controller_;
  scoped_ptr<internal::ActivationController> activation_controller_;
  scoped_ptr<internal::WindowModalityController> window_modality_controller_;
  scoped_ptr<internal::DragDropController> drag_drop_controller_;
  scoped_ptr<internal::WorkspaceController> workspace_controller_;
  scoped_ptr<internal::ShadowController> shadow_controller_;
  scoped_ptr<internal::TooltipController> tooltip_controller_;
  scoped_ptr<internal::VisibilityController> visibility_controller_;
  scoped_ptr<PowerButtonController> power_button_controller_;
  scoped_ptr<VideoDetector> video_detector_;
  scoped_ptr<WindowCycleController> window_cycle_controller_;
  scoped_ptr<internal::FocusCycler> focus_cycler_;

  // The audio and brightness controllers are not owner by the shell.
  AudioController* audio_controller_;
  BrightnessController* brightness_controller_;

  // An event filter that pre-handles all key events to send them to an IME.
  scoped_ptr<internal::InputMethodEventFilter> input_method_filter_;

  // An event filter that pre-handles key events while the partial
  // screenshot UI is active.
  scoped_ptr<internal::PartialScreenshotEventFilter> partial_screenshot_filter_;

#if !defined(OS_MACOSX)
  // An event filter that pre-handles global accelerators.
  scoped_ptr<internal::AcceleratorFilter> accelerator_filter_;
#endif

  // The shelf for managing the launcher and the status widget in non-compact
  // mode. Shell does not own the shelf. Instead, it is owned by container of
  // the status area.
  internal::ShelfLayoutManager* shelf_;

  // Does not change after Init().
  WindowMode window_mode_;

  // Can change at runtime.
  BackgroundMode desktop_background_mode_;

  // Owned by aura::RootWindow, cached here for type safety.
  internal::RootWindowLayoutManager* root_window_layout_;

  // Status area with clock, Wi-Fi signal, etc.
  views::Widget* status_widget_;

  // System tray with clock, Wi-Fi signal, etc. (a replacement in progress for
  // |status_widget_|).
  scoped_ptr<SystemTray> tray_;

  // Offset between the corner of the status area and the corner of the screen
  // when in the compact window mode.
  gfx::Size compact_status_area_offset_;

  DISALLOW_COPY_AND_ASSIGN(Shell);
};

}  // namespace ash

#endif  // ASH_SHELL_H_
