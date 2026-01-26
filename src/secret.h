/* Copyright 2026 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#if !__has_include("private/secret.h")
/////////////////////////////////////////////////////////////////////////////
// Modify the followings with your credentials
/////////////////////////////////////////////////////////////////////////////

// replace "<MobileNumber>" and "<ApiKey>" with your whatsapp phone number and API key from callmebot
#define MOBILE_NUMBER "<MobileNumber>"
#define APIKEY "<ApiKey>"

/////////////////////////////////////////////////////////////////////////////

#else
#include "private/secret.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////
#define CALLMEBOT_HOST "api.callmebot.com"
#define CALLMEBOT_PORT 80
#define CALLMEBOT_PATH "/whatsapp.php?phone=" MOBILE_NUMBER "&apikey=" APIKEY "&text="
