// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2017 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "trusty/TrustyCommon.h"

const uint32_t ama::kToolVersion = 1;

const std::string ama::kHelperLabel = "com.bendb.amanuensis.Trusty";
const std::string ama::kHostAppRightName = "Amanuensis.app";
const std::string ama::kHelperSocketPath = "/var/run/com.bendb.amanuensis.Trusty.socket";

const char * ama::kGetProxyStateRightName = "com.bendb.amanuensis.trusty.GetProxyState";
const char * ama::kSetProxyStateRightName = "com.bendb.amanuensis.trusty.SetProxyState";
const char * ama::kClearSettingsRightName = "com.bendb.amanuensis.trusty.ClearSettings";
const char * ama::kGetToolVersionRightName = "com.bendb.amanuensis.trusty.GetToolVersion";

const char * ama::kPlistLaunchdSocketName = "com.bendb.amanuensis.Trusty";
