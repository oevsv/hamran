# Get base url

> Deprecated: This library was created due to me being unaware of the existence of the [WHATWG URL API](https://nodejs.org/dist/latest-v8.x/docs/api/url.html#url_the_whatwg_url_api). As it is widely supported in both browsers and node and probably more accurate, I see no need for this anymore.

Get the host of an arbitrary url - relative or absulute or static. Suited for comparison with `location.host` to inquire whether an url is in local domain scope.

## Usage

```js
import getBaseUrl from "get-base-url"
const { getBaseUrl } = require("get-base-url")

getBaseUrl("https://maps.google.com/sub/")   // maps.google.com
getBaseUrl("maps.google.com/sub")            // maps.google.com
getBaseUrl("google.com")                     // google.com
getBaseUrl()                                 // The current host if in a browser or localhost of in node
getBaseUrl("relative")                       // The current host if in a browser or localhost of in node
getBaseUrl("./relative")                     // The current host if in a browser or localhost of in node
getBaseUrl("/absolute")                      // The current host if in a browser or localhost of in node
```

### Compare whether arbitrary url is in local domain scope

```js
import getBaseUrl from "get-base-url"
const { getBaseUrl } = require("get-base-url")

function isLocalUrl(url) {
  return getBaseUrl(url) === getBaseUrl()
}

isLocalUrl("sub")           // true
isLocalUrl("google.com")    // false
isLocalUrl("/google.com")   // true
```
