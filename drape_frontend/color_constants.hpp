#pragma once

#include "drape/color.hpp"

#include "indexer/map_style.hpp"

namespace df
{

enum ColorConstant
{
  GuiText,
  MyPositionAccuracy,
  Selection,
  Route,
  RegionPoly,
  RoutePedestrian,
  Arrow3D,
  TrackHumanSpeed,
  TrackCarSpeed,
  TrackPlaneSpeed,
  TrackUnknownDistance,
  Arrow3DObsolete
};

dp::Color GetColorConstant(MapStyle style, ColorConstant constant);

} // namespace df
