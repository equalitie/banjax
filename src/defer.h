#pragma once

template<class F>
class Defer {
public:
  Defer(F f) : _f(std::move(f)) {}
  ~Defer() { _f(); }
private:
  F _f;
};

template<class F>
Defer<F> defer(F&& f) { return { std::forward<F>(f) }; }
