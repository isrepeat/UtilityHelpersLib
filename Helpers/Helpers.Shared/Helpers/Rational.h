#pragma once
#include "common.h"
#include <chrono>
#include <ratio>

template<class T>
class Rational {
public:
    Rational() = default;

    constexpr Rational(const T &num, const T &den)
        : num(num), den(den)
    {}

    template<class RationalT>
    Rational(const RationalT&r)
        : num(static_cast<T>(r.num)), den(static_cast<T>(r.den))
    {}

    bool operator==(const Rational& other) const {
        bool equ = this->den == other.den && this->num == other.num;
        return equ;
    }

    bool operator!=(const Rational& other) const {
        return !(*this == other);
    }

    // converts value with rounding +0.5
    T ConvertValueTo(const T &value, const Rational<T> &other) const {
        T d = this->den * other.num;
        /*
        perform +0.5 rounding to save info about fractional part converting in both directions
        It is what Media Foundation resampler does
        example:
            with rounding
                50(48k) -> 10416.6(hns) -> 10417hns
                130(48k) -> 27083.3(hns) -> 27083hns

                10417hns -> 50.0016(48k) -> 50(48k)
                27083hns -> 129.9984(48k) -> 130(48k)
            without rounding:
                50(48k) -> 10416.6(hns) -> 10416hns
                130(48k) -> 27083.3(hns) -> 27083hns

                10416hns -> 49.9968(48k) -> 49(48k)
                27083hns -> 129.9984(48k) -> 129(48k)
        */
        T res = Rational::Mul3Div(value, other.den, this->num, d / 2, d);
        return res;
    }

    // converts value with trunation of fractional part
    T ConvertTruncValueTo(const T &value, const Rational<T> &other) const {
        T d = this->den * other.num;
        T res = Rational::Mul3Div(value, other.den, this->num, d);
        return res;
    }

    // converts value with ceiling
    T ConvertCeilValueTo(const T &value, const Rational<T> &other) const {
        T d = this->den * other.num;
        T res = Rational::Mul3Div(value, other.den, this->num, d - 1, d);
        return res;
    }

    T num = T(1);
    T den = T(1);

private:
     /*
        Performs (a * b * c) / d minimizing chance for overflow even on big numbers.
        a should be greatest value of a,b,c, b middle, c smallest to have better precision
     */
    template<class T> static T Mul3Div(T a, T b, T c, T d) {
        T q = a / d;
        T r = a % d;

        T q1 = (r * b) / d;
        T r1 = (r * b) % d;

        T x1 = q * b * c;
        T x2 = q1 * c;
        T x3 = (r1 * c) / d;

        T res = x1 + x2 + x3;

        return res;
    }

    /*
        Performs ((a * b * c) + e) / d minimizing chance for overflow even on big numbers.
        a should be greatest value of a,b,c, b middle, c smallest to have better precision
    */
    template<class T> static T Mul3Div(T a, T b, T c, T e, T d) {
        T q = a / d;
        T r = a % d;

        T q1 = (r * b) / d;
        T r1 = (r * b) % d;

        T x1 = q * b * c;
        T x2 = q1 * c;
        T x3 = (r1 * c + e) / d;

        T res = x1 + x2 + x3;

        return res;
    }
};


namespace HELPERS_NS {   
    //
    // Another Rational version
    //
    template <typename _Rep>
    struct Rational {
        Rational()
            : num{ 0 }
            , den{ 1 }
            , valueRep{ 1 }
            , rational{ 0 }
        {}
        Rational(intmax_t num, intmax_t den, _Rep valueRep = 1)
            : num{ num }
            , den{ den }
            , valueRep{ valueRep }
        {
            if (den == 0) {
                throw std::exception("Divide by zero");
            }
            rational = static_cast<double>(num) / den; // Compute once
        }

        intmax_t Numerator() {
            return num;
        }
        intmax_t Denumerator() {
            return den;
        }
        _Rep Value() {
            return valueRep;
        }

        template <typename _ToRep>
        _ToRep To() {
            return static_cast<_ToRep>(rational);
        }

        template <typename _ToRep, typename _OtherRep>
        Rational<_ToRep> CastToRational(Rational<_OtherRep> other) {
            return Rational<_ToRep>(
                other.Numerator(),
                other.Denumerator(),
                static_cast<_ToRep>(this->valueRep * this->To<double>() / other.To<double>()));
        }

        Rational Inversed() {
            return Rational(den, num, valueRep);
        }

    private:
        // TODO: make const members and write custom copy Ctor
        intmax_t num;
        intmax_t den;
        _Rep valueRep; // representation value
        double rational;
    };

    //
    // Helpers class Ratio that allow you to get double rational value, self-inversed instance and get chrono::duration after multiplication
    //
    template <intmax_t _Nx, intmax_t _Dx = 1>
    struct Ratio : std::ratio<_Nx, _Dx> {
        using _MyBase = std::ratio<_Nx, _Dx>;

        static constexpr double value = (double)_MyBase::num / _MyBase::den;
        static constexpr Ratio<_Dx, _Nx> inversed() { return Ratio<_Dx, _Nx>{}; };
    };

    template <typename ValueT, intmax_t _Nx, intmax_t _Dx>
    constexpr std::chrono::duration<ValueT, std::ratio<_Nx, _Dx>> operator*(ValueT value, Ratio<_Nx, _Dx> /*ratio*/) {
        return std::chrono::duration<ValueT, std::ratio<_Nx, _Dx>>(value);
    }

    template <typename ValueT, intmax_t _Nx, intmax_t _Dx>
    constexpr std::chrono::duration<ValueT, std::ratio<_Nx, _Dx>> operator*(Ratio<_Nx, _Dx> /*ratio*/, ValueT value) {
        return std::chrono::duration<ValueT, std::ratio<_Nx, _Dx>>(value);
    }
}