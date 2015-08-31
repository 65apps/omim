#include "testing/testing.hpp"

#include "routing/turns_sound.hpp"
#include "routing/turns_sound_settings.hpp"

#include "platform/location.hpp"

namespace
{
using namespace location;
using namespace routing::turns;
using namespace routing::turns::sound;

// A error to compare two double after conversion feet to meters.
double const kEps = 1.;
// A error to compare two doubles which are almost equal.
double const kSmallEps = .001;

UNIT_TEST(TurnNotificationSettingsMetersTest)
{
  Settings const settings(20 /* notificationTimeSeconds */,
                          200 /* minNotificationDistanceUnits */,
                          700 /* maxNotificationDistanceUnits */,
                          {100, 200, 300, 400, 500, 600, 700} /* soundedDistancesUnits */,
                          LengthUnits::Meters /* lengthUnits */);

  TEST(settings.IsValid(), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(20.), 20., kEps), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(0.), 0., kEps), ());
  TEST(my::AlmostEqualAbs(settings.ConvertUnitsToMeters(300. /* distanceInUnits */), 300., kEps), ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(300 /* distanceInUnits */), 300, ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(0 /* distanceInUnits */), 100, ());

  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(0. /* distanceInUnits */), 200., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(10. /* distanceInUnits */), 200., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(20. /* distanceInUnits */), 400., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(35. /* distanceInUnits */), 700., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(200. /* distanceInUnits */), 700., kSmallEps), ());
}

UNIT_TEST(TurnNotificationSettingsFeetTest)
{
  Settings const settings(20 /* notificationTimeSeconds */,
                          500 /* minNotificationDistanceUnits */,
                          2000 /* maxNotificationDistanceUnits */,
                          {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
                          LengthUnits::Feet /* lengthUnits */);

  TEST(settings.IsValid(), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(20.), 65., kEps), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(0.), 0., kEps), ());
  TEST(my::AlmostEqualAbs(settings.ConvertUnitsToMeters(300. /* distanceInUnits */), 91., kEps), ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(500 /* distanceInUnits */), 600, ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(0 /* distanceInUnits */), 200, ());

  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(0. /* distanceInUnits */), 500., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(10. /* distanceInUnits */), 500., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(30. /* distanceInUnits */), 600., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(40. /* distanceInUnits */), 800., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(200. /* distanceInUnits */), 2000., kSmallEps), ());
}

UNIT_TEST(TurnNotificationSettingsNotValidTest)
{
  Settings settings1(20 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */,
                     2000 /* maxNotificationDistanceUnits */,
                     {200, 400, 800, 600, 1000, 1500, 2000} /* soundedDistancesUnits */,
                     LengthUnits::Feet /* lengthUnits */);
  TEST(!settings1.IsValid(), ());

  Settings settings2(20 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */,
                     2000 /* maxNotificationDistanceUnits */,
                     {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
                     LengthUnits::Undefined /* lengthUnits */);
  TEST(!settings2.IsValid(), ());

  Settings settings3(20 /* notificationTimeSeconds */, 5000 /* minNotificationDistanceUnits */,
                     2000 /* maxNotificationDistanceUnits */,
                     {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
                     LengthUnits::Meters /* lengthUnits */);
  TEST(!settings3.IsValid(), ());
}

UNIT_TEST(TurnsSoundMetersTest)
{
  TurnsSound turnSound;
  turnSound.Enable(true);
  turnSound.SetLengthUnits(routing::turns::sound::LengthUnits::Meters);
  string const engShortJson =
      "\
      {\
      \"in_600_meters\":\"In 600 meters.\",\
      \"make_a_right_turn\":\"Make a right turn.\"\
      }";
  turnSound.SetLocaleWithJson(engShortJson);

  turnSound.Reset();
  turnSound.SetSpeedMetersPerSecond(30.);

  TurnItem turnItem(5 /* idx */, TurnDirection::TurnRight);
  vector<string> turnNotifications;

  ASSERT(turnNotifications.empty(), ());

  // Starting nearing the turnItem.
  // 1000 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 1000. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 700 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 700. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 699 meters till the turn. It's time to pronounce the first voice notification.
  // Why? The current speed is 30 meters per seconds. According to correctSettingsMeters
  // we need to play the first voice notification 20 seconds before the turn.
  // Besides that we need 5 seconds (but 100 meters maximum) for playing the notification.
  // So we start playing the first notification when the distance till the turn is less
  // then 20 seconds * 30 meters per seconds + 100 meters = 700 meters.
  turnSound.GenerateTurnSound(turnItem, 699. /* distanceToTurnMeters */, turnNotifications);
  vector<string> const expectedNotification1 = {{"In 600 meters. Make a right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  // 650 meters till the turn. No sound notifications is required.
  turnNotifications.clear();
  turnSound.GenerateTurnSound(turnItem, 650. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  turnSound.SetSpeedMetersPerSecond(32.);

  // 150 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 150. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 100 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 100. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turnSound.GenerateTurnSound(turnItem, 99. /* distanceToTurnMeters */, turnNotifications);
  vector<string> const expectedNotification2 = {{"Make a right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification2, ());

  // 99 meters till the turn again. No sound notifications is required.
  turnNotifications.clear();
  turnSound.GenerateTurnSound(turnItem, 99. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 50 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 50. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 0 meters till the turn. No sound notifications is required.
  turnNotifications.clear();
  turnSound.GenerateTurnSound(turnItem, 0. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  TEST(turnSound.IsEnabled(), ());
}

// Test case:
// - Two turns;
// - They are close to each other;
// So the first notification of the second turn shall be skipped.
UNIT_TEST(TurnsSoundMetersTwoTurnsTest)
{
  TurnsSound turnSound;
  turnSound.Enable(true);
  turnSound.SetLengthUnits(routing::turns::sound::LengthUnits::Meters);
  string const engShortJson =
      "\
      {\
      \"in_700_meters\":\"In 700 meters.\",\
      \"make_a_sharp_right_turn\":\"Make a sharp right turn.\",\
      \"enter_the_roundabout\":\"Enter the roundabout.\"\
      }";
  turnSound.SetLocaleWithJson(engShortJson);

  turnSound.Reset();
  turnSound.SetSpeedMetersPerSecond(35.);

  TurnItem turnItem1(5 /* idx */, TurnDirection::TurnSharpRight);
  vector<string> turnNotifications;

  ASSERT(turnNotifications.empty(), ());

  // Starting nearing the first turn.
  // 800 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem1, 800. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 700 meters till the turn. It's time to pronounce the first voice notification.
  turnSound.GenerateTurnSound(turnItem1, 700. /* distanceToTurnMeters */, turnNotifications);
  vector<string> const expectedNotification1 = {{"In 700 meters. Make a sharp right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  turnSound.SetSpeedMetersPerSecond(32.);

  // 150 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem1, 150. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turnSound.GenerateTurnSound(turnItem1, 99. /* distanceToTurnMeters */, turnNotifications);
  vector<string> const expectedNotification2 = {{"Make a sharp right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification2, ());

  turnSound.SetSpeedMetersPerSecond(10.);

  // 0 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem1, 0. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  TurnItem turnItem2(11 /* idx */, TurnDirection::EnterRoundAbout, 2 /* exitNum */);

  // Starting nearing the second turn.
  turnSound.GenerateTurnSound(turnItem2, 60. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 40 meters till the second turn. It's time to pronounce the second voice notification
  // without the first one.
  turnSound.GenerateTurnSound(turnItem2, 40. /* distanceToTurnMeters */, turnNotifications);
  vector<string> const expectedNotification3 = {{"Enter the roundabout."}};
  TEST_EQUAL(turnNotifications, expectedNotification3, ());

  TEST(turnSound.IsEnabled(), ());
}

UNIT_TEST(TurnsSoundFeetTest)
{
  TurnsSound turnSound;
  turnSound.Enable(true);
  turnSound.SetLengthUnits(routing::turns::sound::LengthUnits::Feet);
  string const engShortJson =
      "\
      {\
      \"in_2000_feet\":\"In 2000 feet.\",\
      \"enter_the_roundabout\":\"Enter the roundabout.\"\
      }";
  turnSound.SetLocaleWithJson(engShortJson);

  turnSound.Reset();
  turnSound.SetSpeedMetersPerSecond(30.);

  TurnItem turnItem(7 /* idx */, TurnDirection::EnterRoundAbout, 3 /* exitNum */);
  vector<string> turnNotifications;

  ASSERT(turnNotifications.empty(), ());

  // Starting nearing the turnItem.
  // 1000 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 1000. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 700 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 700. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 699 meters till the turn. It's time to pronounce the first voice notification.
  // Why? The current speed is 30 meters per seconds. According to correctSettingsMeters
  // we need to play the first voice notification 20 seconds before the turn.
  // Besides that we need 5 seconds (but 100 meters maximum) for playing the notification.
  // So we start playing the first notification when the distance till the turn is less
  // then 20 seconds * 30 meters per seconds + 100 meters = 700 meters.
  turnSound.GenerateTurnSound(turnItem, 699. /* distanceToTurnMeters */, turnNotifications);
  vector<string> const expectedNotification1 = {{"In 2000 feet. Enter the roundabout."}};
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  // 650 meters till the turn. No sound notifications is required.
  turnNotifications.clear();
  turnSound.GenerateTurnSound(turnItem, 650. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 150 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 150. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 100 meters till the turn. No sound notifications is required.
  turnSound.GenerateTurnSound(turnItem, 100. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turnSound.GenerateTurnSound(turnItem, 99. /* distanceToTurnMeters */, turnNotifications);
  vector<string> const expectedNotification2 = {{"Enter the roundabout."}};
  TEST_EQUAL(turnNotifications, expectedNotification2, ());

  // 99 meters till the turn again. No sound notifications is required.
  turnNotifications.clear();
  turnSound.GenerateTurnSound(turnItem, 99. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 50 meters till the turn. No sound notifications is required.
  turnNotifications.clear();
  turnSound.GenerateTurnSound(turnItem, 50. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 0 meters till the turn. No sound notifications is required.
  turnNotifications.clear();
  turnSound.GenerateTurnSound(turnItem, 0. /* distanceToTurnMeters */, turnNotifications);
  TEST(turnNotifications.empty(), ());

  TEST(turnSound.IsEnabled(), ());
}
}  // namespace