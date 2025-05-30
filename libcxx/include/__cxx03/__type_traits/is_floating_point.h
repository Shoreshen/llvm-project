//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CXX03___TYPE_TRAITS_IS_FLOATING_POINT_H
#define _LIBCPP___CXX03___TYPE_TRAITS_IS_FLOATING_POINT_H

#include <__cxx03/__config>
#include <__cxx03/__type_traits/integral_constant.h>
#include <__cxx03/__type_traits/remove_cv.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

// clang-format off
template <class _Tp> struct __libcpp_is_floating_point              : public false_type {};
template <>          struct __libcpp_is_floating_point<float>       : public true_type {};
template <>          struct __libcpp_is_floating_point<double>      : public true_type {};
template <>          struct __libcpp_is_floating_point<long double> : public true_type {};
// clang-format on

template <class _Tp>
struct _LIBCPP_TEMPLATE_VIS is_floating_point : public __libcpp_is_floating_point<__remove_cv_t<_Tp> > {};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CXX03___TYPE_TRAITS_IS_FLOATING_POINT_H
