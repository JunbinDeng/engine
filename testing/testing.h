// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_TESTING_H_
#define TESTING_TESTING_H_

#include <string>

#include "flutter/fml/file.h"
#include "flutter/fml/mapping.h"
#include "flutter/testing/assertions.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

//------------------------------------------------------------------------------
/// @brief      Returns the directory containing the test fixture for the target
///             if this target has fixtures configured. If there are no
///             fixtures, this is a link error. If you see a linker error on
///             this symbol, the unit-test target needs to depend on a
///             `test_fixtures` target.
///
/// @return     The fixtures path.
///
const char* GetFixturesPath();

//------------------------------------------------------------------------------
/// @brief      Opens the fixtures directory for the unit-test harness.
///
/// @return     The file descriptor of the fixtures directory.
///
fml::UniqueFD OpenFixturesDirectory();

//------------------------------------------------------------------------------
/// @brief      Opens a fixture of the given file name.
///
/// @param[in]  fixture_name  The fixture name
///
/// @return     The file descriptor of the given fixture. An invalid file
///             descriptor is returned in case the fixture is not found.
///
fml::UniqueFD OpenFixture(std::string fixture_name);

//------------------------------------------------------------------------------
/// @brief      Opens a fixture of the given file name and returns a mapping to
///             its contents.
///
/// @param[in]  fixture_name  The fixture name
///
/// @return     A mapping to the contents of fixture or null if the fixture does
///             not exist or its contents cannot be mapped in.
///
std::unique_ptr<fml::Mapping> OpenFixtureAsMapping(std::string fixture_name);

//------------------------------------------------------------------------------
/// @brief      Gets the name of the currently running test. This is useful in
///             generating logs or assets based on test name.
///
/// @return     The current test name.
///
std::string GetCurrentTestName();

}  // namespace testing
}  // namespace flutter

#endif  // TESTING_TESTING_H_
