#pragma once

template<class T, T default_value>
class Default {
public:
  Default()    : value(default_value) {}
  Default(T v) : value(v) {}

        T& operator*()       { return value; }
  const T& operator*() const { return value; }

  operator T() const { return value; }

private:
  T value;
};

