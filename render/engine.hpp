#pragma once

#include "render/information_display.hpp"
#include "render/scales_processor.hpp"
#include "render/navigator.hpp"
#include "render/render_policy.hpp"
#include "render/events.hpp"
#include "render/animator.hpp"
#include "render/active_maps_bridge.hpp"
#include "render/route_renderer.hpp"

#include "geometry/polyline2d.hpp"

#include "platform/location.hpp"

#include "base/deferred_task.hpp"

#include "std/unique_ptr.hpp"
#include "std/function.hpp"

class StringsBundle;
namespace gui { class Controller; }
namespace anim { class Controller; }

class FeatureType;

namespace rg
{

class RenderPolicy;

class Engine
{
public:
  using TParseFn = function<bool (FeatureType const &)>;
  using TDrawFn = function<void (TParseFn const &, m2::RectD const &, int, bool)>;
  using TScreenChangedFn = function<void (ScreenBase const &)>;

  struct Params
  {
    RenderPolicy::Params m_rpParams;
    TDrawFn m_drawFn;
    bool m_initGui = true;
    unique_ptr<ActiveMapsBridge> m_mapsBrigde;
  };

  Engine(Params && params);
  ~Engine();

  void Scale(double factor, m2::PointD const & pxPoint, bool isAnim);

  void InitGui(StringsBundle const & bundle);
  void Resize(int w, int h);
  void DrawFrame();

  void ShowRect(m2::AnyRectD const & rect);
  int AddModelViewListener(TScreenChangedFn const & listener);
  void RemoveModelViewListener(int slotID);

  void SetMyPositionModeListener(location::TMyPositionModeChanged const & listener);

  using TPositionListener = function<void (m2::PointD const &)>;
  void SetMyPositionListener(TPositionListener const & fn);
  void SwitchMyPositionNextMode();
  void InvalidateMyPosition();
  void OnLocationError();
  void OnLocationUpdate(location::GpsInfo const & info, bool isNavigable,
                        location::RouteMatchingInfo const & routeInfo);
  void OnCompassUpdate(location::CompassInfo const & info);

  int GetDrawScale();

  enum ETouchMask
  {
    MASK_EMPTY,
    MASK_FIRST,
    MASK_SECOND,
    MASK_BOTH
  };

  enum ETouchAction
  {
    ACTION_DOWN,
    ACTION_MOVE,
    ACTION_UP,
    ACTION_CANCEL
  };

  void Touch(ETouchAction action, ETouchMask mask, m2::PointD const & pt1, m2::PointD const & pt2);

  bool GetMyPosition(m2::PointD & myPosition);
  void AddRoute(m2::PolylineD const & routePolyline, vector<double> const & turns, graphics::Color const & color);
  void RemoveRoute(bool deactivateFollowing);

private:
  void StartDrag(DragEvent const & e);
  void DoDrag(DragEvent const & e);
  void StopDrag(DragEvent const & e);

  void StartScale(ScaleEvent const & e);
  void DoScale(ScaleEvent const & e);
  void StopScale(ScaleEvent const & e);

  void CalcScalePoints(ScaleEvent const & e, m2::PointD & pt1, m2::PointD & pt2) const;

private:
  void BeginPaint(shared_ptr<PaintEvent> const & e);
  void DoPaint(shared_ptr<PaintEvent> const & e);
  void EndPaint(shared_ptr<PaintEvent> const & e);

  void DrawAdditionalInfo(shared_ptr<PaintEvent> const & e);

  void DrawModel(shared_ptr<PaintEvent> const & e, ScreenBase const & screen,
                 m2::RectD const & renderRect, int baseScale);

  void Invalidate(bool forceUpdate = true);
  void Invalidate(m2::RectD const & rect, bool forceUpdate = true);

  int GetWidth() const;
  int GetHeight() const;

  void SetFullScreenMode(bool enable) { m_isFullScreenMode = enable; }

private:
  unique_ptr<gui::Controller> m_guiController; // can be nullptr if gui subsystem not initialized
  unique_ptr<RenderPolicy> m_renderPolicy;
  unique_ptr<InformationDisplay> m_informationDisplay;
  bool m_isFullScreenMode = false;
  unique_ptr<ActiveMapsBridge> m_mapsBridge;
  unique_ptr<Animator> m_animator;
  unique_ptr<RouteRenderer> m_routeRenderer;

  ScalesProcessor m_scales;
  Navigator m_navigator;
  TDrawFn m_drawFn;

  m2::PointD m_touch1;
  m2::PointD m_touch2;
  ETouchMask m_mask;

  my::HighResTimer m_doubleClickTimer;
  bool m_isCleanSingleClick;
  m2::PointD m_lastTouch;

  void StartTouchTask(m2::PointD const & pt, unsigned ms);
  void KillTouchTask();
  void OnProcessTouchTask(m2::PointD const & pt, unsigned ms);

  unique_ptr<DeferredTask> m_deferredTask;
  bool m_wasLongClick;
};

} // namespace rg