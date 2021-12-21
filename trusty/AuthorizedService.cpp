// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
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

#include "AuthorizedService.h"

#include "log/Log.h"
#include "trusty/TrustyCommon.h"

namespace ama::trusty {

AuthorizedService::AuthorizedService(const std::shared_ptr<IService>& delegate, const std::vector<uint8_t>& authExternalForm)
    : IService()
    , delegate_(delegate)
{
    if (authExternalForm.size() != sizeof(AuthorizationExternalForm))
    {
        throw std::invalid_argument("Payload isn't the right size");
    }

    OSStatus status = AuthorizationCreateFromExternalForm((AuthorizationExternalForm *) authExternalForm.data(), &auth_);
    if (status != errAuthorizationSuccess || auth_ == nullptr)
    {
        throw std::invalid_argument("payload couldn't be made into an AuthorizationRef");
    }
}

AuthorizedService::~AuthorizedService()
{
    if (auth_ != nullptr)
    {
        AuthorizationFree(auth_, kAuthorizationFlagDefaults);
    }
}

ProxyState AuthorizedService::get_http_proxy_state()
{
    assert_right(ama::kGetProxyStateRightName);
    return delegate_->get_http_proxy_state();
}

void AuthorizedService::set_http_proxy_state(const ProxyState& endpoint)
{
    assert_right(ama::kSetProxyStateRightName);
    delegate_->set_http_proxy_state(endpoint);
}

void AuthorizedService::reset_proxy_settings()
{
    assert_right(ama::kClearSettingsRightName);
    delegate_->reset_proxy_settings();
}

uint32_t AuthorizedService::get_current_version()
{
    assert_right(ama::kGetToolVersionRightName);
    return delegate_->get_current_version();
}

void AuthorizedService::assert_right(const char* right)
{
    log::debug("Asserting a right", log::CStrValue("right", right));
    AuthorizationItem item = { right, 0, NULL, 0 };
    AuthorizationRights rights = { 1, &item };

    OSStatus status = AuthorizationCopyRights(
                auth_,
                &rights,
                kAuthorizationEmptyEnvironment,
                kAuthorizationFlagDefaults | kAuthorizationFlagExtendRights,
                nullptr);

    if (status != errAuthorizationSuccess)
    {
        log::debug("Could not obtain right non-interactively; trying again, with interaction", log::I32Value("status", static_cast<int>(status)));

        status = AuthorizationCopyRights(
                    auth_,
                    &rights,
                    kAuthorizationEmptyEnvironment,
                    kAuthorizationFlagDefaults | kAuthorizationFlagExtendRights | kAuthorizationFlagInteractionAllowed,
                    nullptr);

        if (status != errAuthorizationSuccess)
        {
            log::error("Right not present", log::CStrValue("right", right), log::I32Value("status", static_cast<int>(status)));
            throw std::invalid_argument("Authorization denied");
        }
    }
}

}
