/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/Move.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Vector.h"

using mozilla::detail::VectorTesting;
using mozilla::MakeUnique;
using mozilla::Move;
using mozilla::UniquePtr;
using mozilla::Vector;

struct mozilla::detail::VectorTesting
{
  static void testReserved();
  static void testConstRange();
  static void testEmplaceBack();
  static void testReverse();
  static void testExtractRawBuffer();
  static void testExtractOrCopyRawBuffer();
  static void testInsert();
  static void testErase();
};

void
mozilla::detail::VectorTesting::testReserved()
{
#ifdef DEBUG
  Vector<bool> bv;
  MOZ_RELEASE_ASSERT(bv.reserved() == 0);

  MOZ_RELEASE_ASSERT(bv.append(true));
  MOZ_RELEASE_ASSERT(bv.reserved() == 1);

  Vector<bool> otherbv;
  MOZ_RELEASE_ASSERT(otherbv.append(false));
  MOZ_RELEASE_ASSERT(otherbv.append(true));
  MOZ_RELEASE_ASSERT(bv.appendAll(otherbv));
  MOZ_RELEASE_ASSERT(bv.reserved() == 3);

  MOZ_RELEASE_ASSERT(bv.reserve(5));
  MOZ_RELEASE_ASSERT(bv.reserved() == 5);

  MOZ_RELEASE_ASSERT(bv.reserve(1));
  MOZ_RELEASE_ASSERT(bv.reserved() == 5);

  Vector<bool> bv2(Move(bv));
  MOZ_RELEASE_ASSERT(bv.reserved() == 0);
  MOZ_RELEASE_ASSERT(bv2.reserved() == 5);

  bv2.clearAndFree();
  MOZ_RELEASE_ASSERT(bv2.reserved() == 0);

  Vector<int, 42> iv;
  MOZ_RELEASE_ASSERT(iv.reserved() == 0);

  MOZ_RELEASE_ASSERT(iv.append(17));
  MOZ_RELEASE_ASSERT(iv.reserved() == 1);

  Vector<int, 42> otheriv;
  MOZ_RELEASE_ASSERT(otheriv.append(42));
  MOZ_RELEASE_ASSERT(otheriv.append(37));
  MOZ_RELEASE_ASSERT(iv.appendAll(otheriv));
  MOZ_RELEASE_ASSERT(iv.reserved() == 3);

  MOZ_RELEASE_ASSERT(iv.reserve(5));
  MOZ_RELEASE_ASSERT(iv.reserved() == 5);

  MOZ_RELEASE_ASSERT(iv.reserve(1));
  MOZ_RELEASE_ASSERT(iv.reserved() == 5);

  MOZ_RELEASE_ASSERT(iv.reserve(55));
  MOZ_RELEASE_ASSERT(iv.reserved() == 55);

  Vector<int, 42> iv2(Move(iv));
  MOZ_RELEASE_ASSERT(iv.reserved() == 0);
  MOZ_RELEASE_ASSERT(iv2.reserved() == 55);

  iv2.clearAndFree();
  MOZ_RELEASE_ASSERT(iv2.reserved() == 0);
#endif
}

void
mozilla::detail::VectorTesting::testConstRange()
{
#ifdef DEBUG
  Vector<int> vec;

  for (int i = 0; i < 10; i++) {
    MOZ_RELEASE_ASSERT(vec.append(i));
  }

  const auto &vecRef = vec;

  Vector<int>::ConstRange range = vecRef.all();
  for (int i = 0; i < 10; i++) {
    MOZ_RELEASE_ASSERT(!range.empty());
    MOZ_RELEASE_ASSERT(range.front() == i);
    range.popFront();
  }
#endif
}

namespace {

struct S
{
  size_t            j;
  UniquePtr<size_t> k;

  static size_t constructCount;
  static size_t moveCount;
  static size_t destructCount;

  static void resetCounts() {
    constructCount = 0;
    moveCount = 0;
    destructCount = 0;
  }

  S(size_t j, size_t k)
    : j(j)
    , k(MakeUnique<size_t>(k))
  {
    constructCount++;
  }

  S(S&& rhs)
    : j(rhs.j)
    , k(Move(rhs.k))
  {
    rhs.j = 0;
    rhs.k.reset(0);
    moveCount++;
  }

  ~S() {
    destructCount++;
  }

  S& operator=(S&& rhs) {
    j = rhs.j;
    rhs.j = 0;
    k = Move(rhs.k);
    rhs.k.reset();
    moveCount++;
    return *this;
  }

  bool operator==(const S& rhs) const { return j == rhs.j && *k == *rhs.k; }

  S(const S&) = delete;
  S& operator=(const S&) = delete;
};

size_t S::constructCount = 0;
size_t S::moveCount = 0;
size_t S::destructCount = 0;

}

void
mozilla::detail::VectorTesting::testEmplaceBack()
{
  S::resetCounts();

  Vector<S> vec;
  MOZ_RELEASE_ASSERT(vec.reserve(20));

  for (size_t i = 0; i < 10; i++) {
    S s(i, i * i);
    MOZ_RELEASE_ASSERT(vec.append(Move(s)));
  }

  MOZ_RELEASE_ASSERT(vec.length() == 10);
  MOZ_RELEASE_ASSERT(S::constructCount == 10);
  MOZ_RELEASE_ASSERT(S::moveCount == 10);

  for (size_t i = 10; i < 20; i++) {
    MOZ_RELEASE_ASSERT(vec.emplaceBack(i, i * i));
  }

  MOZ_RELEASE_ASSERT(vec.length() == 20);
  MOZ_RELEASE_ASSERT(S::constructCount == 20);
  MOZ_RELEASE_ASSERT(S::moveCount == 10);

  for (size_t i = 0; i < 20; i++) {
    MOZ_RELEASE_ASSERT(vec[i].j == i);
    MOZ_RELEASE_ASSERT(*vec[i].k == i * i);
  }
}

void
mozilla::detail::VectorTesting::testReverse()
{
  // Use UniquePtr to make sure that reverse() can handler move-only types.
  Vector<UniquePtr<uint8_t>, 0> vec;

  // Reverse an odd number of elements.

  for (uint8_t i = 0; i < 5; i++) {
    auto p = MakeUnique<uint8_t>(i);
    MOZ_RELEASE_ASSERT(p);
    MOZ_RELEASE_ASSERT(vec.append(mozilla::Move(p)));
  }

  vec.reverse();

  MOZ_RELEASE_ASSERT(*vec[0] == 4);
  MOZ_RELEASE_ASSERT(*vec[1] == 3);
  MOZ_RELEASE_ASSERT(*vec[2] == 2);
  MOZ_RELEASE_ASSERT(*vec[3] == 1);
  MOZ_RELEASE_ASSERT(*vec[4] == 0);

  // Reverse an even number of elements.

  vec.popBack();
  vec.reverse();

  MOZ_RELEASE_ASSERT(*vec[0] == 1);
  MOZ_RELEASE_ASSERT(*vec[1] == 2);
  MOZ_RELEASE_ASSERT(*vec[2] == 3);
  MOZ_RELEASE_ASSERT(*vec[3] == 4);

  // Reverse an empty vector.

  vec.clear();
  MOZ_RELEASE_ASSERT(vec.length() == 0);
  vec.reverse();
  MOZ_RELEASE_ASSERT(vec.length() == 0);

  // Reverse a vector using only inline storage.

  Vector<UniquePtr<uint8_t>, 5> vec2;
  for (uint8_t i = 0; i < 5; i++) {
    auto p = MakeUnique<uint8_t>(i);
    MOZ_RELEASE_ASSERT(p);
    MOZ_RELEASE_ASSERT(vec2.append(mozilla::Move(p)));
  }

  vec2.reverse();

  MOZ_RELEASE_ASSERT(*vec2[0] == 4);
  MOZ_RELEASE_ASSERT(*vec2[1] == 3);
  MOZ_RELEASE_ASSERT(*vec2[2] == 2);
  MOZ_RELEASE_ASSERT(*vec2[3] == 1);
  MOZ_RELEASE_ASSERT(*vec2[4] == 0);
}

void
mozilla::detail::VectorTesting::testExtractRawBuffer()
{
  S::resetCounts();

  Vector<S, 5> vec;
  MOZ_RELEASE_ASSERT(vec.reserve(5));
  for (size_t i = 0; i < 5; i++) {
    vec.infallibleEmplaceBack(i, i * i);
  }
  MOZ_RELEASE_ASSERT(vec.length() == 5);
  MOZ_ASSERT(vec.reserved() == 5);
  MOZ_RELEASE_ASSERT(S::constructCount == 5);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 0);

  S* buf = vec.extractRawBuffer();
  MOZ_RELEASE_ASSERT(!buf);
  MOZ_RELEASE_ASSERT(vec.length() == 5);
  MOZ_ASSERT(vec.reserved() == 5);
  MOZ_RELEASE_ASSERT(S::constructCount == 5);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 0);

  MOZ_RELEASE_ASSERT(vec.reserve(10));
  for (size_t i = 5; i < 10; i++) {
    vec.infallibleEmplaceBack(i, i * i);
  }
  MOZ_RELEASE_ASSERT(vec.length() == 10);
  MOZ_ASSERT(vec.reserved() == 10);
  MOZ_RELEASE_ASSERT(S::constructCount == 10);
  MOZ_RELEASE_ASSERT(S::moveCount == 5);
  MOZ_RELEASE_ASSERT(S::destructCount == 5);

  buf = vec.extractRawBuffer();
  MOZ_RELEASE_ASSERT(buf);
  MOZ_RELEASE_ASSERT(vec.length() == 0);
  MOZ_ASSERT(vec.reserved() == 0);
  MOZ_RELEASE_ASSERT(S::constructCount == 10);
  MOZ_RELEASE_ASSERT(S::moveCount == 5);
  MOZ_RELEASE_ASSERT(S::destructCount == 5);

  for (size_t i = 0; i < 10; i++) {
    MOZ_RELEASE_ASSERT(buf[i].j == i);
    MOZ_RELEASE_ASSERT(*buf[i].k == i * i);
  }

  free(buf);
}

void
mozilla::detail::VectorTesting::testExtractOrCopyRawBuffer()
{
  S::resetCounts();

  Vector<S, 5> vec;
  MOZ_RELEASE_ASSERT(vec.reserve(5));
  for (size_t i = 0; i < 5; i++) {
    vec.infallibleEmplaceBack(i, i * i);
  }
  MOZ_RELEASE_ASSERT(vec.length() == 5);
  MOZ_ASSERT(vec.reserved() == 5);
  MOZ_RELEASE_ASSERT(S::constructCount == 5);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 0);

  S* buf = vec.extractOrCopyRawBuffer();
  MOZ_RELEASE_ASSERT(buf);
  MOZ_RELEASE_ASSERT(vec.length() == 0);
  MOZ_ASSERT(vec.reserved() == 0);
  MOZ_RELEASE_ASSERT(S::constructCount == 5);
  MOZ_RELEASE_ASSERT(S::moveCount == 5);
  MOZ_RELEASE_ASSERT(S::destructCount == 5);

  for (size_t i = 0; i < 5; i++) {
    MOZ_RELEASE_ASSERT(buf[i].j == i);
    MOZ_RELEASE_ASSERT(*buf[i].k == i * i);
  }

  S::resetCounts();

  MOZ_RELEASE_ASSERT(vec.reserve(10));
  for (size_t i = 0; i < 10; i++) {
    vec.infallibleEmplaceBack(i, i * i);
  }
  MOZ_RELEASE_ASSERT(vec.length() == 10);
  MOZ_ASSERT(vec.reserved() == 10);
  MOZ_RELEASE_ASSERT(S::constructCount == 10);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 0);

  buf = vec.extractOrCopyRawBuffer();
  MOZ_RELEASE_ASSERT(buf);
  MOZ_RELEASE_ASSERT(vec.length() == 0);
  MOZ_ASSERT(vec.reserved() == 0);
  MOZ_RELEASE_ASSERT(S::constructCount == 10);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 0);

  for (size_t i = 0; i < 10; i++) {
    MOZ_RELEASE_ASSERT(buf[i].j == i);
    MOZ_RELEASE_ASSERT(*buf[i].k == i * i);
  }

  free(buf);
}

void
mozilla::detail::VectorTesting::testInsert()
{
  S::resetCounts();

  Vector<S, 8> vec;
  MOZ_RELEASE_ASSERT(vec.reserve(8));
  for (size_t i = 0; i < 7; i++) {
    vec.infallibleEmplaceBack(i, i * i);
  }

  MOZ_RELEASE_ASSERT(vec.length() == 7);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 7);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 0);

  S s(42, 43);
  MOZ_RELEASE_ASSERT(vec.insert(vec.begin() + 4, Move(s)));

  for (size_t i = 0; i < vec.length(); i++) {
    const S& s = vec[i];
    MOZ_RELEASE_ASSERT(s.k);
    if (i < 4) {
      MOZ_RELEASE_ASSERT(s.j == i && *s.k == i * i);
    } else if (i == 4) {
      MOZ_RELEASE_ASSERT(s.j == 42 && *s.k == 43);
    } else {
      MOZ_RELEASE_ASSERT(s.j == i - 1 && *s.k == (i - 1) * (i - 1));
    }
  }

  MOZ_RELEASE_ASSERT(vec.length() == 8);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 8);
  MOZ_RELEASE_ASSERT(S::moveCount == 1 /* move in insert() call */ +
                                     1 /* move the back() element */ +
                                     3 /* elements to shift */);
  MOZ_RELEASE_ASSERT(S::destructCount == 1);
}

void mozilla::detail::VectorTesting::testErase() {
  S::resetCounts();

  Vector<S, 8> vec;
  MOZ_RELEASE_ASSERT(vec.reserve(8));
  for (size_t i = 0; i < 7; i++) {
    vec.infallibleEmplaceBack(i, i * i);
  }

  // vec: [0, 1, 2, 3, 4, 5, 6]
  MOZ_RELEASE_ASSERT(vec.length() == 7);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 7);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 0);
  S::resetCounts();

  vec.erase(&vec[4]);
  // vec: [0, 1, 2, 3, 5, 6]
  MOZ_RELEASE_ASSERT(vec.length() == 6);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 0);
  // 5 and 6 should have been moved into 4 and 5.
  MOZ_RELEASE_ASSERT(S::moveCount == 2);
  MOZ_RELEASE_ASSERT(S::destructCount == 1);
  MOZ_RELEASE_ASSERT(vec[4] == S(5, 5 * 5));
  MOZ_RELEASE_ASSERT(vec[5] == S(6, 6 * 6));
  S::resetCounts();

  vec.erase(&vec[3], &vec[5]);
  // vec: [0, 1, 2, 6]
  MOZ_RELEASE_ASSERT(vec.length() == 4);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 0);
  // 6 should have been moved into 3.
  MOZ_RELEASE_ASSERT(S::moveCount == 1);
  MOZ_RELEASE_ASSERT(S::destructCount == 2);
  MOZ_RELEASE_ASSERT(vec[3] == S(6, 6 * 6));

  S s2(2, 2 * 2);
  S::resetCounts();

  vec.eraseIfEqual(s2);
  // vec: [0, 1, 6]
  MOZ_RELEASE_ASSERT(vec.length() == 3);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 0);
  // 6 should have been moved into 2.
  MOZ_RELEASE_ASSERT(S::moveCount == 1);
  MOZ_RELEASE_ASSERT(S::destructCount == 1);
  MOZ_RELEASE_ASSERT(vec[2] == S(6, 6 * 6));
  S::resetCounts();

  // Predicate to find one element.
  vec.eraseIf([](const S& s) { return s.j == 1; });
  // vec: [0, 6]
  MOZ_RELEASE_ASSERT(vec.length() == 2);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 0);
  // 6 should have been moved into 1.
  MOZ_RELEASE_ASSERT(S::moveCount == 1);
  MOZ_RELEASE_ASSERT(S::destructCount == 1);
  MOZ_RELEASE_ASSERT(vec[1] == S(6, 6 * 6));
  S::resetCounts();

  // Generic predicate that flags everything.
  vec.eraseIf([](auto&&) { return true; });
  // vec: []
  MOZ_RELEASE_ASSERT(vec.length() == 0);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 0);
  MOZ_RELEASE_ASSERT(S::moveCount == 0);
  MOZ_RELEASE_ASSERT(S::destructCount == 2);

  for (size_t i = 0; i < 7; i++) {
    vec.infallibleEmplaceBack(i, i * i);
  }
  // vec: [0, 1, 2, 3, 4, 5, 6]
  MOZ_RELEASE_ASSERT(vec.length() == 7);
  S::resetCounts();

  // Predicate that flags all even numbers.
  vec.eraseIf([](const S& s) { return s.j % 2 == 0; });
  // vec: [1 (was 0), 3 (was 1), 5 (was 2)]
  MOZ_RELEASE_ASSERT(vec.length() == 3);
  MOZ_ASSERT(vec.reserved() == 8);
  MOZ_RELEASE_ASSERT(S::constructCount == 0);
  MOZ_RELEASE_ASSERT(S::moveCount == 3);
  MOZ_RELEASE_ASSERT(S::destructCount == 4);
}

// Declare but leave (permanently) incomplete.
struct Incomplete;

// We could even *construct* a Vector<Incomplete, 0> if we wanted.  But we can't
// destruct it, so it's not worth the trouble.
static_assert(sizeof(Vector<Incomplete, 0>) > 0,
              "Vector of an incomplete type will compile");

// Vector with no inline storage should occupy the absolute minimum space in
// non-debug builds.  (Debug adds a laundry list of other constraints, none
// directly relevant to shipping builds, that aren't worth precisely modeling.)
#ifndef DEBUG

template<typename T>
struct NoInlineStorageLayout
{
  T* mBegin;
  size_t mLength;
  struct CRAndStorage {
    size_t mCapacity;
  } mTail;
};

// Only one of these should be necessary, but test a few of them for good
// measure.
static_assert(sizeof(Vector<int, 0>) == sizeof(NoInlineStorageLayout<int>),
              "Vector of int without inline storage shouldn't occupy dead "
              "space for that absence of storage");

static_assert(sizeof(Vector<bool, 0>) == sizeof(NoInlineStorageLayout<bool>),
              "Vector of bool without inline storage shouldn't occupy dead "
              "space for that absence of storage");

static_assert(sizeof(Vector<S, 0>) == sizeof(NoInlineStorageLayout<S>),
              "Vector of S without inline storage shouldn't occupy dead "
              "space for that absence of storage");

static_assert(sizeof(Vector<Incomplete, 0>) == sizeof(NoInlineStorageLayout<Incomplete>),
              "Vector of an incomplete class without inline storage shouldn't "
              "occupy dead space for that absence of storage");

#endif // DEBUG

int
main()
{
  VectorTesting::testReserved();
  VectorTesting::testConstRange();
  VectorTesting::testEmplaceBack();
  VectorTesting::testReverse();
  VectorTesting::testExtractRawBuffer();
  VectorTesting::testExtractOrCopyRawBuffer();
  VectorTesting::testInsert();
  VectorTesting::testErase();
}
