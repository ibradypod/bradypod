#ifndef CONSTS_H
#define CONSTS_H

#define BRADYPOD_VERSION_MAJOR     1
#define BRADYPOD_VERSION_MINOR     0
#define BRADYPOD_VERSION_PATCH     0
#define BRADYPOD_VERSION_STRING    "1.0.0-dev"

#define HTTP_HEADER_CONTENT_LENGTH      "content-length"
#define HTTP_HEADER_CONTENT_TYPE        "content-type"

#define JAVASCRIPT_SOURCE_PLATFORM_URL  "bradypodjs://platform/%1"
#define JAVASCRIPT_SOURCE_CODE_URL      "bradypodjs://code/%1"

#define JS_ELEMENT_CLICK "(function (el) { " \
    "var ev = document.createEvent('MouseEvents');" \
    "ev.initEvent(\"click\", true, true);" \
    "el.dispatchEvent(ev);" \
    "})(this);"

#define JS_APPEND_SCRIPT_ELEMENT "var el = document.createElement('script');" \
    "el.onload = function() { alert('%1'); };" \
    "el.src = '%1';" \
    "document.body.appendChild(el);"

#define PAGE_SETTINGS_LOAD_IMAGES           "loadImages"
#define PAGE_SETTINGS_JS_ENABLED            "javascriptEnabled"
#define PAGE_SETTINGS_XSS_AUDITING          "XSSAuditingEnabled"
#define PAGE_SETTINGS_USER_AGENT            "userAgent"
#define PAGE_SETTINGS_PROXY                 "proxy"
#define PAGE_SETTINGS_LOCAL_ACCESS_REMOTE   "localToRemoteUrlAccessEnabled"
#define PAGE_SETTINGS_USERNAME              "userName"
#define PAGE_SETTINGS_PASSWORD              "password"
#define PAGE_SETTINGS_MAX_AUTH_ATTEMPTS     "maxAuthAttempts"
#define PAGE_SETTINGS_RESOURCE_TIMEOUT      "resourceTimeout"
#define PAGE_SETTINGS_WEB_SECURITY_ENABLED  "webSecurityEnabled"
#define PAGE_SETTINGS_JS_CAN_OPEN_WINDOWS   "javascriptCanOpenWindows"
#define PAGE_SETTINGS_JS_CAN_CLOSE_WINDOWS  "javascriptCanCloseWindows"
#define PAGE_SETTINGS_DPI                   "dpi"

#endif // CONSTS_H
