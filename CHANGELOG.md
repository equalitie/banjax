## Not yet released

* Show error to client without delay when `ERR_CONNECT_FAIL` (non standard HTTP error) is received from origin.
* Difficulty of inverse SHA is no longer limited to multiples of 4 bits.
* Cleanup of unused files.
* Replace Gtest test framework in favor of Boost.Test (we already require boost).
* Replace autotools in favor of CMake.
* Add CircleCI automated builds.
* `magic_words` now uses partial regex.
* `magic_words_exceptions` now uses full regex.
* Change integration tests to work with TS 7.0.0 (as well as 6.0.0).
* Disable caching for pages displayed to white listed IPs ([6053](https://redmine.equalit.ie/issues/6053)).
* Enable white listing of IPs per host in the challenger filter ([6053](https://redmine.equalit.ie/issues/6053).
