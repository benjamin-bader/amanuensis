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

#include "TrustyCommon.h"

const std::string ama::kHelperLabel = "com.bendb.amanuensis.Trusty";
const std::string ama::kHostAppRightName = "Amanuensis.app";
const std::string ama::kHelperSocketPath = "/var/run/com.bendb.amanuensis.Trusty.socket";

const char * ama::kSetHostRightName = "com.bendb.amanuensis.trusty.SetHost";
const char * ama::kSetPortRightName = "com.bendb.amanuensis.trusty.SetPort";
const char * ama::kGetHostRightName = "com.bendb.amanuensis.trusty.GetHost";
const char * ama::kGetPortRightName = "com.bendb.amanuensis.trusty.GetPort";
const char * ama::kClearSettingsRightName = "com.bendb.amanuensis.trusty.ClearSettings";
