/* Copyright (c) 2014, 2021, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/* See http://code.google.com/p/googletest/wiki/Primer */

#include <gtest/gtest.h>
#include <stddef.h>
#include <stdexcept>

#include "storage/innobase/include/detail/ut0new.h"
#include "storage/innobase/include/univ.i"
#include "storage/innobase/include/ut0new.h"

namespace innodb_ut0new_unittest {

static auto pfs_key = 12345;

static void start() { ut_new_boot_safe(); }

using int_types =
    ::testing::Types<short int, unsigned short int, int, unsigned int, long int,
                     unsigned long int, long long int, unsigned long long int>;

using char_types = ::testing::Types<char, unsigned char, wchar_t>;

using floating_point_types = ::testing::Types<float, double, long double>;

/**
 * This is a typed test template, it's instantiated below for all primitive
 * types. This way we can cover all the supported fundamental alignments and
 * sizes.
 */
template <class T>
class ut0new_t : public ::testing::Test {
 protected:
  ut_allocator<T> allocator;
};

template <class T>
struct wrapper {
 public:
  static constexpr T INIT_VAL = std::numeric_limits<T>::min() + 1;
  wrapper(T data = INIT_VAL) : data(data) {}
  T data;
};

template <class T>
constexpr T wrapper<T>::INIT_VAL;

TYPED_TEST_SUITE_P(ut0new_t);

TYPED_TEST_P(ut0new_t, ut_new_fundamental_types) {
  start();
  const auto MAX = std::numeric_limits<TypeParam>::max();
  auto p = UT_NEW_NOKEY(TypeParam(MAX));
  EXPECT_EQ(*p, MAX);
  UT_DELETE(p);

  p = UT_NEW(TypeParam(MAX - 1), mem_key_buf_buf_pool);
  EXPECT_EQ(*p, MAX - 1);
  UT_DELETE(p);

  const int CNT = 5;
  p = UT_NEW_ARRAY_NOKEY(TypeParam, CNT);
  for (int i = 0; i < CNT; ++i) {
    p[i] = MAX;
    EXPECT_EQ(p[i], MAX);
  }
  UT_DELETE_ARRAY(p);

  p = UT_NEW_ARRAY(TypeParam, CNT, mem_key_buf_buf_pool);
  for (int i = 0; i < CNT; ++i) {
    p[i] = MAX - 1;
    EXPECT_EQ(p[i], MAX - 1);
  }
  UT_DELETE_ARRAY(p);
}

TYPED_TEST_P(ut0new_t, ut_new_structs) {
  start();
  const auto MAX = std::numeric_limits<TypeParam>::max();

  using w = wrapper<TypeParam>;

  auto p = UT_NEW_NOKEY(w(TypeParam(MAX)));
  EXPECT_EQ(p->data, MAX);
  UT_DELETE(p);

  p = UT_NEW(w(TypeParam(MAX - 1)), mem_key_buf_buf_pool);
  EXPECT_EQ(p->data, MAX - 1);
  UT_DELETE(p);

  const int CNT = 5;

  p = UT_NEW_ARRAY_NOKEY(w, CNT);
  for (int i = 0; i < CNT; ++i) {
    EXPECT_EQ(w::INIT_VAL, p[i].data);
  }
  UT_DELETE_ARRAY(p);

  p = UT_NEW_ARRAY(w, CNT, mem_key_buf_buf_pool);
  for (int i = 0; i < CNT; ++i) {
    EXPECT_EQ(w::INIT_VAL, p[i].data);
  }
  UT_DELETE_ARRAY(p);
}

TYPED_TEST_P(ut0new_t, ut_malloc) {
  start();
  TypeParam *p;
  const auto MAX = std::numeric_limits<TypeParam>::max();
  const auto MIN = std::numeric_limits<TypeParam>::min();

  p = static_cast<TypeParam *>(ut_malloc_nokey(sizeof(TypeParam)));
  *p = MIN;
  ut_free(p);

  p = static_cast<TypeParam *>(
      ut_malloc(sizeof(TypeParam), mem_key_buf_buf_pool));
  *p = MAX;
  ut_free(p);

  p = static_cast<TypeParam *>(ut_zalloc_nokey(sizeof(TypeParam)));
  EXPECT_EQ(0, *p);
  *p = MAX;
  ut_free(p);

  p = static_cast<TypeParam *>(
      ut_zalloc(sizeof(TypeParam), mem_key_buf_buf_pool));
  EXPECT_EQ(0, *p);
  *p = MAX;
  ut_free(p);

  p = static_cast<TypeParam *>(ut_malloc_nokey(sizeof(TypeParam)));
  *p = MAX - 1;
  p = static_cast<TypeParam *>(ut_realloc(p, 2 * sizeof(TypeParam)));
  EXPECT_EQ(MAX - 1, p[0]);
  p[1] = MAX;
  ut_free(p);
}
/* test ut_allocator() */
TYPED_TEST_P(ut0new_t, ut_vector) {
  start();

  typedef ut_allocator<TypeParam> vec_allocator_t;
  typedef std::vector<TypeParam, vec_allocator_t> vec_t;
  const auto MAX = std::numeric_limits<TypeParam>::max();
  const auto MIN = std::numeric_limits<TypeParam>::min();

  vec_t v1;
  v1.push_back(MIN);
  v1.push_back(MIN + 1);
  v1.push_back(MAX);
  EXPECT_EQ(MIN, v1[0]);
  EXPECT_EQ(MIN + 1, v1[1]);
  EXPECT_EQ(MAX, v1[2]);

  /* We use "new" instead of "UT_NEW()" for simplicity here. Real InnoDB
  code should use UT_NEW(). */

  /* This could of course be written as:
  std::vector<int, ut_allocator<int> >*	v2
  = new std::vector<int, ut_allocator<int> >(ut_allocator<int>(
  mem_key_buf_buf_pool)); */
  vec_t *v2 = new vec_t(vec_allocator_t(mem_key_buf_buf_pool));
  v2->push_back(MIN);
  v2->push_back(MIN + 1);
  v2->push_back(MAX);
  EXPECT_EQ(MIN, v2->at(0));
  EXPECT_EQ(MIN + 1, v2->at(1));
  EXPECT_EQ(MAX, v2->at(2));
  delete v2;
}

REGISTER_TYPED_TEST_SUITE_P(ut0new_t, ut_new_fundamental_types, ut_new_structs,
                            ut_malloc, ut_vector);

INSTANTIATE_TYPED_TEST_SUITE_P(int_types, ut0new_t, int_types);
INSTANTIATE_TYPED_TEST_SUITE_P(float_types, ut0new_t, floating_point_types);
INSTANTIATE_TYPED_TEST_SUITE_P(char_types, ut0new_t, char_types);
INSTANTIATE_TYPED_TEST_SUITE_P(bool, ut0new_t, bool);

static int n_construct = 0;

class cc_t {
 public:
  cc_t() {
    n_construct++;
    if (n_construct % 4 == 0) {
      throw(1);
    }
  }
};

struct big_t {
  char x[128];
};

/* test edge cases */
TEST(ut0new, edgecases) {
  ut_allocator<byte> alloc1(mem_key_buf_buf_pool);
  void *ret;
  const void *null_ptr = nullptr;

  ret = alloc1.allocate_large(0);
  EXPECT_EQ(null_ptr, ret);

#ifdef UNIV_PFS_MEMORY
  ret = alloc1.allocate(16);
  ASSERT_TRUE(ret != nullptr);

  ret = alloc1.reallocate(ret, 0, UT_NEW_THIS_FILE_PSI_KEY);
  EXPECT_EQ(null_ptr, ret);

  ret = UT_NEW_ARRAY_NOKEY(byte, 0);
  EXPECT_NE(null_ptr, ret);
  UT_DELETE_ARRAY((byte *)ret);
#endif /* UNIV_PFS_MEMORY */

  ut_allocator<big_t> alloc2(mem_key_buf_buf_pool);

  const ut_allocator<big_t>::size_type too_many_elements =
      std::numeric_limits<ut_allocator<big_t>::size_type>::max() /
          sizeof(big_t) +
      1;

#ifdef UNIV_PFS_MEMORY
  ret = alloc2.allocate(16);
  ASSERT_TRUE(ret != nullptr);
  void *ret2 =
      alloc2.reallocate(ret, too_many_elements, UT_NEW_THIS_FILE_PSI_KEY);
  EXPECT_EQ(null_ptr, ret2);
  /* If reallocate fails due to too many elements,
  memory is still allocated. Do explicit deallocate do avoid mem leak. */
  alloc2.deallocate(static_cast<big_t *>(ret));
#endif /* UNIV_PFS_MEMORY */

  bool threw = false;
  try {
    ret = alloc2.allocate(too_many_elements);
  } catch (...) {
    threw = true;
  }
  EXPECT_TRUE(threw);

  threw = false;
  try {
    ret = alloc2.allocate(too_many_elements, nullptr, PSI_NOT_INSTRUMENTED,
                          false);
  } catch (std::bad_array_new_length &e) {
    threw = true;
  }
  EXPECT_TRUE(threw);

  threw = false;
  try {
    cc_t *cc = UT_NEW_ARRAY_NOKEY(cc_t, 16);
    /* Not reached, but silence a compiler warning
    about unused 'cc': */
    ASSERT_TRUE(cc != nullptr);
  } catch (...) {
    threw = true;
  }
  EXPECT_TRUE(threw);
}

struct Pod_type {
  Pod_type(int _x, int _y) : x(_x), y(_y) {}
  int x;
  int y;
};
struct My_fancy_sum {
  My_fancy_sum(int x, int y) : result(x + y) {}
  int result;
};
struct Non_pod_type {
  Non_pod_type(int _x, int _y, std::string _s)
      : x(_x), y(_y), s(_s), sum(std::make_unique<My_fancy_sum>(x, y)) {}
  int x;
  int y;
  std::string s;
  std::unique_ptr<My_fancy_sum> sum;
};
struct Default_constructible_pod {
  Default_constructible_pod() : x(0), y(1) {}
  int x, y;
};
struct Default_constructible_non_pod {
  Default_constructible_non_pod() : x(0), y(1), s("non-pod-string") {}
  int x, y;
  std::string s;
};

template <typename T, bool With_pfs>
struct Ut0new_test_param_wrapper {
  using type = T;
  static constexpr bool with_pfs = With_pfs;
};

using all_fundamental_types = ::testing::Types<
    // with PFS
    Ut0new_test_param_wrapper<char, true>,
    Ut0new_test_param_wrapper<unsigned char, true>,
    Ut0new_test_param_wrapper<wchar_t, true>,
    Ut0new_test_param_wrapper<short int, true>,
    Ut0new_test_param_wrapper<unsigned short int, true>,
    Ut0new_test_param_wrapper<int, true>,
    Ut0new_test_param_wrapper<unsigned int, true>,
    Ut0new_test_param_wrapper<long int, true>,
    Ut0new_test_param_wrapper<unsigned long int, true>,
    Ut0new_test_param_wrapper<long long int, true>,
    Ut0new_test_param_wrapper<unsigned long long int, true>,
    Ut0new_test_param_wrapper<float, true>,
    Ut0new_test_param_wrapper<double, true>,
    Ut0new_test_param_wrapper<long double, true>,
    // no PFS
    Ut0new_test_param_wrapper<char, false>,
    Ut0new_test_param_wrapper<unsigned char, false>,
    Ut0new_test_param_wrapper<wchar_t, false>,
    Ut0new_test_param_wrapper<short int, false>,
    Ut0new_test_param_wrapper<unsigned short int, false>,
    Ut0new_test_param_wrapper<int, false>,
    Ut0new_test_param_wrapper<unsigned int, false>,
    Ut0new_test_param_wrapper<long int, false>,
    Ut0new_test_param_wrapper<unsigned long int, false>,
    Ut0new_test_param_wrapper<long long int, false>,
    Ut0new_test_param_wrapper<unsigned long long int, false>,
    Ut0new_test_param_wrapper<float, false>,
    Ut0new_test_param_wrapper<double, false>,
    Ut0new_test_param_wrapper<long double, false>>;

using all_pod_types = ::testing::Types<
    // with PFS
    Ut0new_test_param_wrapper<Pod_type, true>,
    // no PFS
    Ut0new_test_param_wrapper<Pod_type, false>>;

using all_default_constructible_pod_types = ::testing::Types<
    // with PFS
    Ut0new_test_param_wrapper<Default_constructible_pod, true>,
    // no PFS
    Ut0new_test_param_wrapper<Default_constructible_pod, false>>;

using all_non_pod_types = ::testing::Types<
    // with PFS
    Ut0new_test_param_wrapper<Non_pod_type, true>,
    // no PFS
    Ut0new_test_param_wrapper<Non_pod_type, false>>;

using all_default_constructible_non_pod_types = ::testing::Types<
    // with PFS
    Ut0new_test_param_wrapper<Default_constructible_non_pod, true>,
    // no PFS
    Ut0new_test_param_wrapper<Default_constructible_non_pod, false>>;

// malloc/free - fundamental types
template <typename T>
class ut0new_malloc_free_fundamental_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_malloc_free_fundamental_types);
TYPED_TEST_P(ut0new_malloc_free_fundamental_types, fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr = with_pfs ? static_cast<type *>(ut::malloc_withkey(
                             ut::make_psi_memory_key(pfs_key), sizeof(type)))
                       : static_cast<type *>(ut::malloc(sizeof(type)));
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);
  ut::free(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_malloc_free_fundamental_types,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(FundamentalTypes,
                               ut0new_malloc_free_fundamental_types,
                               all_fundamental_types);

// malloc/free - pod types
template <typename T>
class ut0new_malloc_free_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_malloc_free_pod_types);
TYPED_TEST_P(ut0new_malloc_free_pod_types, pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr = with_pfs ? static_cast<type *>(ut::malloc_withkey(
                             ut::make_psi_memory_key(pfs_key), sizeof(type)))
                       : static_cast<type *>(ut::malloc(sizeof(type)));
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);
  ut::free(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_malloc_free_pod_types, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(PodTypes, ut0new_malloc_free_pod_types,
                               all_pod_types);

// malloc/free - non-pod types
template <typename T>
class ut0new_malloc_free_non_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_malloc_free_non_pod_types);
TYPED_TEST_P(ut0new_malloc_free_non_pod_types, non_pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr = with_pfs ? static_cast<type *>(ut::malloc_withkey(
                             ut::make_psi_memory_key(pfs_key), sizeof(type)))
                       : static_cast<type *>(ut::malloc(sizeof(type)));
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);
  ut::free(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_malloc_free_non_pod_types, non_pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(NonPodTypes, ut0new_malloc_free_non_pod_types,
                               all_non_pod_types);

// zalloc/free - fundamental types
template <typename T>
class ut0new_zalloc_free_fundamental_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_zalloc_free_fundamental_types);
TYPED_TEST_P(ut0new_zalloc_free_fundamental_types, fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr = with_pfs ? static_cast<type *>(ut::zalloc_withkey(
                             ut::make_psi_memory_key(pfs_key), sizeof(type)))
                       : static_cast<type *>(ut::zalloc(sizeof(type)));
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);
  EXPECT_EQ(*ptr, 0);
  ut::free(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_zalloc_free_fundamental_types,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(FundamentalTypes,
                               ut0new_zalloc_free_fundamental_types,
                               all_fundamental_types);

// realloc - fundamental types
template <typename T>
class ut0new_realloc_fundamental_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_realloc_fundamental_types);
TYPED_TEST_P(ut0new_realloc_fundamental_types, fundamental_types) {
  using T = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  {
    // Allocating through realloc and release through free should work fine
    auto p = with_pfs
                 ? static_cast<T *>(ut::realloc_withkey(
                       ut::make_psi_memory_key(pfs_key), nullptr, sizeof(T)))
                 : static_cast<T *>(ut::realloc(nullptr, sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);
    ut::free(p);
  }

  {
    // Allocating through realloc and release through realloc should work fine
    auto p = with_pfs
                 ? static_cast<T *>(ut::realloc_withkey(
                       ut::make_psi_memory_key(pfs_key), nullptr, sizeof(T)))
                 : static_cast<T *>(ut::realloc(nullptr, sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);
    ut::realloc(p, 0);
  }

  {
    // Allocating through malloc and then upsizing the memory region through
    // realloc should work fine
    auto p = with_pfs ? static_cast<T *>(ut::malloc_withkey(
                            ut::make_psi_memory_key(pfs_key), sizeof(T)))
                      : static_cast<T *>(ut::malloc(sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);

    // Let's write something into the memory so we can afterwards check if
    // ut::realloc_* is handling the copying/moving the element(s) properly
    *p = 0xA;

    // Enlarge to 10x through realloc
    p = with_pfs ? static_cast<T *>(ut::realloc_withkey(
                       ut::make_psi_memory_key(pfs_key), p, 10 * sizeof(T)))
                 : static_cast<T *>(ut::realloc(p, 10 * sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);

    // Make sure that the contents of memory are untouched after reallocation
    EXPECT_EQ(p[0], 0xA);

    // Write some more stuff to the memory
    for (size_t i = 1; i < 10; i++) p[i] = 0xB;

    // Enlarge to 100x through realloc
    p = with_pfs ? static_cast<T *>(ut::realloc_withkey(
                       ut::make_psi_memory_key(pfs_key), p, 100 * sizeof(T)))
                 : static_cast<T *>(ut::realloc(p, 100 * sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);

    // Make sure that the contents of memory are untouched after reallocation
    EXPECT_EQ(p[0], 0xA);
    for (size_t i = 1; i < 10; i++) EXPECT_EQ(p[i], 0xB);

    // Write some more stuff to the memory
    for (size_t i = 10; i < 100; i++) p[i] = 0xC;

    // Enlarge to 1000x through realloc
    p = with_pfs ? static_cast<T *>(ut::realloc_withkey(
                       ut::make_psi_memory_key(pfs_key), p, 1000 * sizeof(T)))
                 : static_cast<T *>(ut::realloc(p, 1000 * sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);

    // Make sure that the contents of memory are untouched after reallocation
    EXPECT_EQ(p[0], 0xA);
    for (size_t i = 1; i < 10; i++) EXPECT_EQ(p[i], 0xB);
    for (size_t i = 10; i < 100; i++) EXPECT_EQ(p[i], 0xC);

    ut::free(p);
  }

  {
    // Allocating through malloc and then downsizing the memory region through
    // realloc should also work fine
    auto p = with_pfs ? static_cast<T *>(ut::malloc_withkey(
                            ut::make_psi_memory_key(pfs_key), 10 * sizeof(T)))
                      : static_cast<T *>(ut::malloc(10 * sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);

    // Write some stuff to the memory
    for (size_t i = 0; i < 10; i++) p[i] = 0xA;

    // Downsize the array to only half of the elements
    p = with_pfs ? static_cast<T *>(ut::realloc_withkey(
                       ut::make_psi_memory_key(pfs_key), p, 5 * sizeof(T)))
                 : static_cast<T *>(ut::realloc(p, 5 * sizeof(T)));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(p) % alignof(max_align_t) ==
                0);

    // Make sure that the respective contents of memory (only half of the
    // elements) are untouched after reallocation
    for (size_t i = 0; i < 5; i++) EXPECT_EQ(p[i], 0xA);

    ut::free(p);
  }
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_realloc_fundamental_types,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(FundamentalTypes,
                               ut0new_realloc_fundamental_types,
                               all_fundamental_types);

// new/delete - fundamental types
template <typename T>
class ut0new_new_delete_fundamental_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_fundamental_types);
TYPED_TEST_P(ut0new_new_delete_fundamental_types, fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr = with_pfs
                  ? ut::new_withkey<type>(ut::make_psi_memory_key(pfs_key), 1)
                  : ut::new_<type>(1);
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);
  EXPECT_EQ(*ptr, 1);
  ut::delete_(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_new_delete_fundamental_types,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, ut0new_new_delete_fundamental_types,
                               all_fundamental_types);

// new/delete - pod types
template <typename T>
class ut0new_new_delete_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_pod_types);
TYPED_TEST_P(ut0new_new_delete_pod_types, pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;

  type *ptr =
      with_pfs ? ut::new_withkey<type>(ut::make_psi_memory_key(pfs_key), 2, 5)
               : ut::new_<type>(2, 5);
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);
  EXPECT_EQ(ptr->x, 2);
  EXPECT_EQ(ptr->y, 5);
  ut::delete_(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_new_delete_pod_types, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, ut0new_new_delete_pod_types, all_pod_types);

// new/delete - non-pod types
template <typename T>
class ut0new_new_delete_non_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_non_pod_types);
TYPED_TEST_P(ut0new_new_delete_non_pod_types, non_pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr = with_pfs ? ut::new_withkey<type>(ut::make_psi_memory_key(pfs_key),
                                               2, 5, std::string("non-pod"))
                       : ut::new_<type>(2, 5, std::string("non-pod"));
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);
  EXPECT_EQ(ptr->x, 2);
  EXPECT_EQ(ptr->y, 5);
  EXPECT_EQ(ptr->sum->result, 7);
  EXPECT_EQ(ptr->s, std::string("non-pod"));
  ut::delete_(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_new_delete_non_pod_types, non_pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, ut0new_new_delete_non_pod_types,
                               all_non_pod_types);

// new/delete - array specialization for fundamental types
template <typename T>
class ut0new_new_delete_fundamental_types_arr : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_fundamental_types_arr);
TYPED_TEST_P(ut0new_new_delete_fundamental_types_arr, fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr =
      with_pfs
          ? ut::new_arr_withkey<type>(
                ut::make_psi_memory_key(pfs_key),
                std::forward_as_tuple((type)0), std::forward_as_tuple((type)1),
                std::forward_as_tuple((type)2), std::forward_as_tuple((type)3),
                std::forward_as_tuple((type)4), std::forward_as_tuple((type)5),
                std::forward_as_tuple((type)6), std::forward_as_tuple((type)7),
                std::forward_as_tuple((type)8), std::forward_as_tuple((type)9))
          : ut::new_arr<type>(
                std::forward_as_tuple((type)0), std::forward_as_tuple((type)1),
                std::forward_as_tuple((type)2), std::forward_as_tuple((type)3),
                std::forward_as_tuple((type)4), std::forward_as_tuple((type)5),
                std::forward_as_tuple((type)6), std::forward_as_tuple((type)7),
                std::forward_as_tuple((type)8), std::forward_as_tuple((type)9));

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);

  for (size_t elem = 0; elem < 10; elem++) {
    EXPECT_EQ(ptr[elem], elem);
  }
  ut::delete_arr(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_new_delete_fundamental_types_arr,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, ut0new_new_delete_fundamental_types_arr,
                               all_fundamental_types);

// new/delete - array specialization for pod types
template <typename T>
class ut0new_new_delete_pod_types_arr : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_pod_types_arr);
TYPED_TEST_P(ut0new_new_delete_pod_types_arr, pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr =
      with_pfs
          ? ut::new_arr_withkey<type>(
                ut::make_psi_memory_key(pfs_key), std::forward_as_tuple(0, 1),
                std::forward_as_tuple(2, 3), std::forward_as_tuple(4, 5),
                std::forward_as_tuple(6, 7), std::forward_as_tuple(8, 9))
          : ut::new_arr<type>(
                std::forward_as_tuple(0, 1), std::forward_as_tuple(2, 3),
                std::forward_as_tuple(4, 5), std::forward_as_tuple(6, 7),
                std::forward_as_tuple(8, 9));

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);

  for (size_t elem = 0; elem < 5; elem++) {
    EXPECT_EQ(ptr[elem].x, 2 * elem);
    EXPECT_EQ(ptr[elem].y, 2 * elem + 1);
  }
  ut::delete_arr(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_new_delete_pod_types_arr, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, ut0new_new_delete_pod_types_arr,
                               all_pod_types);

// new/delete - array specialization for non-pod types
template <typename T>
class ut0new_new_delete_non_pod_types_arr : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_non_pod_types_arr);
TYPED_TEST_P(ut0new_new_delete_non_pod_types_arr, non_pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  type *ptr = with_pfs ? ut::new_arr_withkey<type>(
                             ut::make_psi_memory_key(pfs_key),
                             std::forward_as_tuple(1, 2, std::string("a")),
                             std::forward_as_tuple(3, 4, std::string("b")),
                             std::forward_as_tuple(5, 6, std::string("c")),
                             std::forward_as_tuple(7, 8, std::string("d")),
                             std::forward_as_tuple(9, 10, std::string("e")))
                       : ut::new_arr<type>(
                             std::forward_as_tuple(1, 2, std::string("a")),
                             std::forward_as_tuple(3, 4, std::string("b")),
                             std::forward_as_tuple(5, 6, std::string("c")),
                             std::forward_as_tuple(7, 8, std::string("d")),
                             std::forward_as_tuple(9, 10, std::string("e")));

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);

  EXPECT_EQ(ptr[0].x, 1);
  EXPECT_EQ(ptr[0].y, 2);
  EXPECT_TRUE(ptr[0].s == std::string("a"));
  EXPECT_EQ(ptr[0].sum->result, 3);

  EXPECT_EQ(ptr[1].x, 3);
  EXPECT_EQ(ptr[1].y, 4);
  EXPECT_TRUE(ptr[1].s == std::string("b"));
  EXPECT_EQ(ptr[1].sum->result, 7);

  EXPECT_EQ(ptr[2].x, 5);
  EXPECT_EQ(ptr[2].y, 6);
  EXPECT_TRUE(ptr[2].s == std::string("c"));
  EXPECT_EQ(ptr[2].sum->result, 11);

  EXPECT_EQ(ptr[3].x, 7);
  EXPECT_EQ(ptr[3].y, 8);
  EXPECT_TRUE(ptr[3].s == std::string("d"));
  EXPECT_EQ(ptr[3].sum->result, 15);

  EXPECT_EQ(ptr[4].x, 9);
  EXPECT_EQ(ptr[4].y, 10);
  EXPECT_TRUE(ptr[4].s == std::string("e"));
  EXPECT_EQ(ptr[4].sum->result, 19);

  ut::delete_arr(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(ut0new_new_delete_non_pod_types_arr, non_pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, ut0new_new_delete_non_pod_types_arr,
                               all_non_pod_types);

// new/delete - array specialization for default constructible
// fundamental types
template <typename T>
class ut0new_new_delete_default_constructible_fundamental_types_arr
    : public ::testing::Test {};
TYPED_TEST_SUITE_P(
    ut0new_new_delete_default_constructible_fundamental_types_arr);
TYPED_TEST_P(ut0new_new_delete_default_constructible_fundamental_types_arr,
             fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  type *ptr = with_pfs
                  ? ut::new_arr_withkey<type>(ut::make_psi_memory_key(pfs_key),
                                              ut::Count{n_elements})
                  : ut::new_arr<type>(ut::Count{n_elements});

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);

  for (size_t elem = 0; elem < n_elements; elem++) {
    EXPECT_EQ(ptr[elem], type{});
  }
  ut::delete_arr(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(
    ut0new_new_delete_default_constructible_fundamental_types_arr,
    fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(
    My, ut0new_new_delete_default_constructible_fundamental_types_arr,
    all_fundamental_types);

// new/delete - array specialization for default constructible pod types
template <typename T>
class ut0new_new_delete_default_constructible_pod_types_arr
    : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_default_constructible_pod_types_arr);
TYPED_TEST_P(ut0new_new_delete_default_constructible_pod_types_arr, pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  type *ptr = with_pfs
                  ? ut::new_arr_withkey<type>(ut::make_psi_memory_key(pfs_key),
                                              ut::Count{n_elements})
                  : ut::new_arr<type>(ut::Count{n_elements});

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);

  for (size_t elem = 0; elem < n_elements; elem++) {
    EXPECT_EQ(ptr[elem].x, 0);
    EXPECT_EQ(ptr[elem].y, 1);
  }
  ut::delete_arr(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(
    ut0new_new_delete_default_constructible_pod_types_arr, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(
    My, ut0new_new_delete_default_constructible_pod_types_arr,
    all_default_constructible_pod_types);

// new/delete - array specialization for default constructible non-pod
// types
template <typename T>
class ut0new_new_delete_default_constructible_non_pod_types_arr
    : public ::testing::Test {};
TYPED_TEST_SUITE_P(ut0new_new_delete_default_constructible_non_pod_types_arr);
TYPED_TEST_P(ut0new_new_delete_default_constructible_non_pod_types_arr,
             pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  type *ptr = with_pfs
                  ? ut::new_arr_withkey<type>(ut::make_psi_memory_key(pfs_key),
                                              ut::Count{n_elements})
                  : ut::new_arr<type>(ut::Count{n_elements});

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignof(max_align_t) ==
              0);

  for (size_t elem = 0; elem < n_elements; elem++) {
    EXPECT_EQ(ptr[elem].x, 0);
    EXPECT_EQ(ptr[elem].y, 1);
    EXPECT_TRUE(ptr[elem].s == std::string("non-pod-string"));
  }
  ut::delete_arr(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(
    ut0new_new_delete_default_constructible_non_pod_types_arr, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(
    My, ut0new_new_delete_default_constructible_non_pod_types_arr,
    all_default_constructible_non_pod_types);

TEST(ut0new_new_delete, unique_ptr_demo) {
  struct Int_deleter {
    void operator()(int *p) {
      std::cout << "Hello from custom deleter!\n";
      ut::delete_(p);
    }
  };
  std::unique_ptr<int, Int_deleter> ptr(ut::new_<int>(1), Int_deleter{});
}

TEST(ut0new_new_delete_arr, unique_ptr_demo) {
  struct Int_arr_deleter {
    void operator()(int *p) {
      std::cout << "Hello from custom deleter!\n";
      ut::delete_arr(p);
    }
  };
  std::unique_ptr<int, Int_arr_deleter> ptr(
      ut::new_arr<int>(std::forward_as_tuple(1), std::forward_as_tuple(2),
                       std::forward_as_tuple(3), std::forward_as_tuple(4),
                       std::forward_as_tuple(5)),
      Int_arr_deleter{});
}

TEST(ut0new_new_delete_arr, demo_with_non_default_constructible_types) {
  auto ptr = ut::new_arr_withkey<Pod_type>(
      ut::make_psi_memory_key(pfs_key), std::forward_as_tuple(1, 2),
      std::forward_as_tuple(3, 4), std::forward_as_tuple(5, 6),
      std::forward_as_tuple(7, 8), std::forward_as_tuple(9, 10));

  EXPECT_EQ(ptr[0].x, 1);
  EXPECT_EQ(ptr[0].y, 2);
  EXPECT_EQ(ptr[1].x, 3);
  EXPECT_EQ(ptr[1].y, 4);
  EXPECT_EQ(ptr[2].x, 5);
  EXPECT_EQ(ptr[2].y, 6);
  EXPECT_EQ(ptr[3].x, 7);
  EXPECT_EQ(ptr[3].y, 8);
  EXPECT_EQ(ptr[4].x, 9);
  EXPECT_EQ(ptr[4].y, 10);

  ut::delete_arr(ptr);
}

TEST(ut0new_new_delete_arr,
     demo_with_explicit_N_default_constructible_instances) {
  constexpr auto n_elements = 5;
  auto ptr = ut::new_arr_withkey<Default_constructible_pod>(
      ut::make_psi_memory_key(pfs_key), std::forward_as_tuple(),
      std::forward_as_tuple(), std::forward_as_tuple(), std::forward_as_tuple(),
      std::forward_as_tuple());

  for (size_t elem = 0; elem < n_elements; elem++) {
    EXPECT_EQ(ptr[elem].x, 0);
    EXPECT_EQ(ptr[elem].y, 1);
  }
  ut::delete_arr(ptr);
}

TEST(ut0new_new_delete_arr,
     demo_with_N_default_constructible_instances_through_ut_count) {
  constexpr auto n_elements = 5;
  auto ptr = ut::new_arr_withkey<Default_constructible_pod>(
      ut::make_psi_memory_key(pfs_key), ut::Count(n_elements));

  for (size_t elem = 0; elem < n_elements; elem++) {
    EXPECT_EQ(ptr[elem].x, 0);
    EXPECT_EQ(ptr[elem].y, 1);
  }
  ut::delete_arr(ptr);
}

TEST(
    ut0new_new_delete_arr,
    demo_with_type_that_is_both_default_constructible_and_constructible_through_user_provided_ctr) {
  struct Type {
    Type() : x(10), y(15) {}
    Type(int _x, int _y) : x(_x), y(_y) {}
    int x, y;
  };

  auto ptr = ut::new_arr_withkey<Type>(
      ut::make_psi_memory_key(pfs_key), std::forward_as_tuple(1, 2),
      std::forward_as_tuple(), std::forward_as_tuple(3, 4),
      std::forward_as_tuple(5, 6), std::forward_as_tuple());

  EXPECT_EQ(ptr[0].x, 1);
  EXPECT_EQ(ptr[0].y, 2);
  EXPECT_EQ(ptr[1].x, 10);
  EXPECT_EQ(ptr[1].y, 15);
  EXPECT_EQ(ptr[2].x, 3);
  EXPECT_EQ(ptr[2].y, 4);
  EXPECT_EQ(ptr[3].x, 5);
  EXPECT_EQ(ptr[3].y, 6);
  EXPECT_EQ(ptr[4].x, 10);
  EXPECT_EQ(ptr[4].y, 15);

  ut::delete_arr(ptr);
}

TEST(
    ut0new_new_delete_arr,
    destructors_of_successfully_instantiated_trivially_constructible_elements_are_invoked_when_one_of_the_element_constructors_throws) {
  static int n_constructors = 0;
  static int n_destructors = 0;

  struct Type_that_may_throw {
    Type_that_may_throw() {
      n_constructors++;
      if (n_constructors % 4 == 0) {
        throw std::runtime_error("cannot construct");
      }
    }
    ~Type_that_may_throw() { ++n_destructors; }
  };

  bool exception_thrown_and_caught = false;
  try {
    auto ptr = ut::new_arr_withkey<Type_that_may_throw>(
        ut::make_psi_memory_key(pfs_key), ut::Count(7));
    ASSERT_FALSE(ptr);
  } catch (std::runtime_error &e) {
    exception_thrown_and_caught = true;
  }
  EXPECT_TRUE(exception_thrown_and_caught);
  EXPECT_EQ(n_constructors, 4);
  EXPECT_EQ(n_destructors, 3);
}

TEST(
    ut0new_new_delete_arr,
    destructors_of_successfully_instantiated_non_trivially_constructible_elements_are_invoked_when_one_of_the_element_constructors_throws) {
  static int n_constructors = 0;
  static int n_destructors = 0;

  struct Type_that_may_throw {
    Type_that_may_throw(int x, int y) : _x(x), _y(y) {
      n_constructors++;
      if (n_constructors % 4 == 0) {
        throw std::runtime_error("cannot construct");
      }
    }
    ~Type_that_may_throw() { ++n_destructors; }
    int _x, _y;
  };

  bool exception_thrown_and_caught = false;
  try {
    auto ptr = ut::new_arr_withkey<Type_that_may_throw>(
        ut::make_psi_memory_key(pfs_key), std::forward_as_tuple(0, 1),
        std::forward_as_tuple(2, 3), std::forward_as_tuple(4, 5),
        std::forward_as_tuple(6, 7), std::forward_as_tuple(8, 9));
    ASSERT_FALSE(ptr);
  } catch (std::runtime_error &e) {
    exception_thrown_and_caught = true;
  }
  EXPECT_TRUE(exception_thrown_and_caught);
  EXPECT_EQ(n_constructors, 4);
  EXPECT_EQ(n_destructors, 3);
}

TEST(
    ut0new_new_delete_arr,
    no_destructors_are_invoked_when_first_trivially_constructible_element_constructor_throws) {
  static int n_constructors = 0;
  static int n_destructors = 0;

  struct Type_that_always_throws {
    Type_that_always_throws() {
      n_constructors++;
      throw std::runtime_error("cannot construct");
    }
    ~Type_that_always_throws() { ++n_destructors; }
  };

  bool exception_thrown_and_caught = false;
  try {
    auto ptr = ut::new_arr_withkey<Type_that_always_throws>(
        ut::make_psi_memory_key(pfs_key), ut::Count(7));
    ASSERT_FALSE(ptr);
  } catch (std::runtime_error &e) {
    exception_thrown_and_caught = true;
  }
  EXPECT_TRUE(exception_thrown_and_caught);
  EXPECT_EQ(n_constructors, 1);
  EXPECT_EQ(n_destructors, 0);
}

TEST(
    ut0new_new_delete_arr,
    no_destructors_are_invoked_when_first_non_trivially_constructible_element_constructor_throws) {
  static int n_constructors = 0;
  static int n_destructors = 0;

  struct Type_that_always_throws {
    Type_that_always_throws(int x, int y) : _x(x), _y(y) {
      n_constructors++;
      throw std::runtime_error("cannot construct");
    }
    ~Type_that_always_throws() { ++n_destructors; }
    int _x, _y;
  };

  bool exception_thrown_and_caught = false;
  try {
    auto ptr = ut::new_arr_withkey<Type_that_always_throws>(
        ut::make_psi_memory_key(pfs_key), std::forward_as_tuple(0, 1),
        std::forward_as_tuple(2, 3), std::forward_as_tuple(4, 5),
        std::forward_as_tuple(6, 7), std::forward_as_tuple(8, 9));
    ASSERT_FALSE(ptr);
  } catch (std::runtime_error &e) {
    exception_thrown_and_caught = true;
  }
  EXPECT_TRUE(exception_thrown_and_caught);
  EXPECT_EQ(n_constructors, 1);
  EXPECT_EQ(n_destructors, 0);
}

TEST(ut0new_new_delete,
     no_destructor_is_invoked_when_no_object_is_successfully_constructed) {
  static int n_constructors = 0;
  static int n_destructors = 0;

  struct Type_that_always_throws {
    Type_that_always_throws() {
      n_constructors++;
      throw std::runtime_error("cannot construct");
    }
    ~Type_that_always_throws() { ++n_destructors; }
  };

  bool exception_thrown_and_caught = false;
  try {
    auto ptr = ut::new_withkey<Type_that_always_throws>(
        ut::make_psi_memory_key(pfs_key));
    ASSERT_FALSE(ptr);
  } catch (std::runtime_error &e) {
    exception_thrown_and_caught = true;
  }
  EXPECT_TRUE(exception_thrown_and_caught);
  EXPECT_EQ(n_constructors, 1);
  EXPECT_EQ(n_destructors, 0);
}

TEST(ut0new_new_delete, zero_sized_allocation_returns_valid_ptr) {
  auto ptr = ut::new_<byte>(0);
  EXPECT_NE(ptr, nullptr);
  ut::delete_(ptr);
}

TEST(ut0new_new_delete_arr, zero_sized_allocation_returns_valid_ptr) {
  auto ptr = ut::new_arr<byte>(ut::Count{0});
  EXPECT_NE(ptr, nullptr);
  ut::delete_arr(ptr);
}

TEST(ut0new_new_delete_arr,
     reference_types_are_automagically_handled_through_forward_as_tuple) {
  struct My_ref {
    My_ref(int &ref) : _ref(ref) {}
    int &_ref;
  };

  int y = 10;
  int x = 20;
  auto ptr = ut::new_arr_withkey<My_ref>(ut::make_psi_memory_key(pfs_key),
                                         std::forward_as_tuple(x),
                                         std::forward_as_tuple(y));
  EXPECT_EQ(ptr[0]._ref, x);
  EXPECT_EQ(ptr[1]._ref, y);
  x = 30;
  y = 40;
  EXPECT_EQ(ptr[0]._ref, x);
  EXPECT_EQ(ptr[1]._ref, y);

  ut::delete_arr(ptr);
}

TEST(ut0new_new_delete_arr,
     reference_types_are_automagically_handled_through_make_tuple_and_ref) {
  struct My_ref {
    My_ref(int &ref) : _ref(ref) {}
    int &_ref;
  };

  int y = 10;
  int x = 20;
  auto ptr = ut::new_arr_withkey<My_ref>(ut::make_psi_memory_key(pfs_key),
                                         std::make_tuple(std::ref(x)),
                                         std::make_tuple(std::ref(y)));
  EXPECT_EQ(ptr[0]._ref, x);
  EXPECT_EQ(ptr[1]._ref, y);
  x = 30;
  y = 40;
  EXPECT_EQ(ptr[0]._ref, x);
  EXPECT_EQ(ptr[1]._ref, y);

  ut::delete_arr(ptr);
}

TEST(ut0new_new_delete_arr,
     reference_types_are_not_automagically_handled_through_make_tuple) {
  struct My_ref {
    My_ref(int &ref) : _ref(ref) {}
    int &_ref;
  };

  int y = 10;
  int x = 20;
  auto ptr = ut::new_arr_withkey<My_ref>(
      ut::make_psi_memory_key(pfs_key), std::make_tuple(x), std::make_tuple(y));
  EXPECT_EQ(ptr[0]._ref, x);
  EXPECT_EQ(ptr[1]._ref, y);
  x = 30;
  y = 40;
  EXPECT_NE(ptr[0]._ref, x);  // mind this diff as opposed to prev two examples
  EXPECT_NE(ptr[1]._ref, y);  // ditto

  ut::delete_arr(ptr);
}

TEST(ut0new_new_delete_arr, proper_overload_resolution_is_selected) {
  auto ptr = ut::new_arr_withkey<Pod_type>(ut::make_psi_memory_key(pfs_key),
                                           std::forward_as_tuple(1, 2));
  EXPECT_EQ(ptr[0].x, 1);
  EXPECT_EQ(ptr[0].y, 2);
  ut::delete_arr(ptr);
}

// aligned alloc/free - fundamental types
template <typename T>
class aligned_alloc_free_fundamental_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_alloc_free_fundamental_types);
TYPED_TEST_P(aligned_alloc_free_fundamental_types, fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr =
        with_pfs
            ? static_cast<type *>(ut::aligned_alloc_withkey(
                  ut::make_psi_memory_key(pfs_key), sizeof(type), alignment))
            : static_cast<type *>(ut::aligned_alloc(sizeof(type), alignment));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);
    ut::aligned_free(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(aligned_alloc_free_fundamental_types,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(FundamentalTypes,
                               aligned_alloc_free_fundamental_types,
                               all_fundamental_types);

// aligned alloc/free - pod types
template <typename T>
class aligned_alloc_free_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_alloc_free_pod_types);
TYPED_TEST_P(aligned_alloc_free_pod_types, pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  auto alignment = 4 * 1024;
  type *ptr =
      with_pfs
          ? static_cast<type *>(ut::aligned_alloc_withkey(
                ut::make_psi_memory_key(pfs_key), sizeof(type), alignment))
          : static_cast<type *>(ut::aligned_alloc(sizeof(type), alignment));
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);
  ut::aligned_free(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(aligned_alloc_free_pod_types, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(PodTypes, aligned_alloc_free_pod_types,
                               all_pod_types);

// aligned alloc/free - non-pod types
template <typename T>
class aligned_alloc_free_non_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_alloc_free_non_pod_types);
TYPED_TEST_P(aligned_alloc_free_non_pod_types, non_pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  auto alignment = 4 * 1024;
  type *ptr =
      with_pfs
          ? static_cast<type *>(ut::aligned_alloc_withkey(
                ut::make_psi_memory_key(pfs_key), sizeof(type), alignment))
          : static_cast<type *>(ut::aligned_alloc(sizeof(type), alignment));
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);
  // Referencing non-pod type members through returned pointer is UB.
  // Solely releasing it is ok.
  //
  // Using it otherwise is UB because aligned_alloc_* functions are raw
  // memory management functions which do not invoke constructors neither
  // they know which type they are operating with. That is why we would be end
  // up accessing memory of not yet instantiated object (UB).
  ut::aligned_free(ptr);
}
REGISTER_TYPED_TEST_SUITE_P(aligned_alloc_free_non_pod_types, non_pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(NonPodTypes, aligned_alloc_free_non_pod_types,
                               all_non_pod_types);

// aligned new/delete - fundamental types
template <typename T>
class aligned_new_delete_fundamental_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_fundamental_types);
TYPED_TEST_P(aligned_new_delete_fundamental_types, fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr = with_pfs ? ut::aligned_new_withkey<type>(
                               ut::make_psi_memory_key(pfs_key), alignment, 1)
                         : ut::aligned_new<type>(alignment, 1);
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);
    EXPECT_EQ(*ptr, 1);
    ut::aligned_delete(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(aligned_new_delete_fundamental_types,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, aligned_new_delete_fundamental_types,
                               all_fundamental_types);

// aligned new/delete - pod types
template <typename T>
class aligned_new_delete_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_pod_types);
TYPED_TEST_P(aligned_new_delete_pod_types, pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;

  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr = with_pfs
                    ? ut::aligned_new_withkey<type>(
                          ut::make_psi_memory_key(pfs_key), alignment, 2, 5)
                    : ut::aligned_new<type>(alignment, 2, 5);
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);
    EXPECT_EQ(ptr->x, 2);
    EXPECT_EQ(ptr->y, 5);
    ut::aligned_delete(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(aligned_new_delete_pod_types, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, aligned_new_delete_pod_types, all_pod_types);

// aligned new/delete - non-pod types
template <typename T>
class aligned_new_delete_non_pod_types : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_non_pod_types);
TYPED_TEST_P(aligned_new_delete_non_pod_types, non_pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr =
        with_pfs
            ? ut::aligned_new_withkey<type>(ut::make_psi_memory_key(pfs_key),
                                            alignment, 2, 5,
                                            std::string("non-pod"))
            : ut::aligned_new<type>(alignment, 2, 5, std::string("non-pod"));
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);
    EXPECT_EQ(ptr->x, 2);
    EXPECT_EQ(ptr->y, 5);
    EXPECT_EQ(ptr->sum->result, 7);
    EXPECT_EQ(ptr->s, std::string("non-pod"));
    ut::aligned_delete(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(aligned_new_delete_non_pod_types, non_pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, aligned_new_delete_non_pod_types,
                               all_non_pod_types);

// aligned new/delete - array specialization for fundamental types
template <typename T>
class aligned_new_delete_fundamental_types_arr : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_fundamental_types_arr);
TYPED_TEST_P(aligned_new_delete_fundamental_types_arr, fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 10;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr = with_pfs ? ut::aligned_new_arr_withkey<type, n_elements>(
                               ut::make_psi_memory_key(pfs_key), alignment, 0,
                               1, 2, 3, 4, 5, 6, 7, 8, 9)
                         : ut::aligned_new_arr<type, n_elements>(
                               alignment, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    for (size_t elem = 0; elem < n_elements; elem++) {
      EXPECT_EQ(ptr[elem], elem);
    }
    ut::aligned_delete_arr(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(aligned_new_delete_fundamental_types_arr,
                            fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, aligned_new_delete_fundamental_types_arr,
                               all_fundamental_types);

// aligned new/delete - array specialization for pod types
template <typename T>
class aligned_new_delete_pod_types_arr : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_pod_types_arr);
TYPED_TEST_P(aligned_new_delete_pod_types_arr, pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr = with_pfs ? ut::aligned_new_arr_withkey<type, n_elements>(
                               ut::make_psi_memory_key(pfs_key), alignment, 0,
                               1, 2, 3, 4, 5, 6, 7, 8, 9)
                         : ut::aligned_new_arr<type, n_elements>(
                               alignment, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    for (size_t elem = 0; elem < n_elements; elem++) {
      EXPECT_EQ(ptr[elem].x, 2 * elem);
      EXPECT_EQ(ptr[elem].y, 2 * elem + 1);
    }
    ut::aligned_delete_arr(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(aligned_new_delete_pod_types_arr, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, aligned_new_delete_pod_types_arr,
                               all_pod_types);

// aligned new/delete - array specialization for non-pod types
template <typename T>
class aligned_new_delete_non_pod_types_arr : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_non_pod_types_arr);
TYPED_TEST_P(aligned_new_delete_non_pod_types_arr, non_pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr = with_pfs ? ut::aligned_new_arr_withkey<type, n_elements>(
                               ut::make_psi_memory_key(pfs_key), alignment, 1,
                               2, std::string("a"), 3, 4, std::string("b"), 5,
                               6, std::string("c"), 7, 8, std::string("d"), 9,
                               10, std::string("e"))
                         : ut::aligned_new_arr<type, n_elements>(
                               alignment, 1, 2, std::string("a"), 3, 4,
                               std::string("b"), 5, 6, std::string("c"), 7, 8,
                               std::string("d"), 9, 10, std::string("e"));

    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    EXPECT_EQ(ptr[0].x, 1);
    EXPECT_EQ(ptr[0].y, 2);
    EXPECT_TRUE(ptr[0].s == std::string("a"));
    EXPECT_EQ(ptr[0].sum->result, 3);

    EXPECT_EQ(ptr[1].x, 3);
    EXPECT_EQ(ptr[1].y, 4);
    EXPECT_TRUE(ptr[1].s == std::string("b"));
    EXPECT_EQ(ptr[1].sum->result, 7);

    EXPECT_EQ(ptr[2].x, 5);
    EXPECT_EQ(ptr[2].y, 6);
    EXPECT_TRUE(ptr[2].s == std::string("c"));
    EXPECT_EQ(ptr[2].sum->result, 11);

    EXPECT_EQ(ptr[3].x, 7);
    EXPECT_EQ(ptr[3].y, 8);
    EXPECT_TRUE(ptr[3].s == std::string("d"));
    EXPECT_EQ(ptr[3].sum->result, 15);

    EXPECT_EQ(ptr[4].x, 9);
    EXPECT_EQ(ptr[4].y, 10);
    EXPECT_TRUE(ptr[4].s == std::string("e"));
    EXPECT_EQ(ptr[4].sum->result, 19);

    ut::aligned_delete_arr(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(aligned_new_delete_non_pod_types_arr,
                            non_pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(My, aligned_new_delete_non_pod_types_arr,
                               all_non_pod_types);

// aligned new/delete - array specialization for default constructible
// fundamental types
template <typename T>
class aligned_new_delete_default_constructible_fundamental_types_arr
    : public ::testing::Test {};
TYPED_TEST_SUITE_P(
    aligned_new_delete_default_constructible_fundamental_types_arr);
TYPED_TEST_P(aligned_new_delete_default_constructible_fundamental_types_arr,
             fundamental_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr =
        with_pfs ? ut::aligned_new_arr_withkey<type>(
                       ut::make_psi_memory_key(pfs_key), alignment, n_elements)
                 : ut::aligned_new_arr<type>(alignment, n_elements);

    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    for (size_t elem = 0; elem < n_elements; elem++) {
      EXPECT_EQ(ptr[elem], type{});
    }
    ut::aligned_delete_arr(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(
    aligned_new_delete_default_constructible_fundamental_types_arr,
    fundamental_types);
INSTANTIATE_TYPED_TEST_SUITE_P(
    My, aligned_new_delete_default_constructible_fundamental_types_arr,
    all_fundamental_types);

// aligned new/delete - array specialization for default constructible pod types
template <typename T>
class aligned_new_delete_default_constructible_pod_types_arr
    : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_default_constructible_pod_types_arr);
TYPED_TEST_P(aligned_new_delete_default_constructible_pod_types_arr,
             pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr =
        with_pfs ? ut::aligned_new_arr_withkey<type>(
                       ut::make_psi_memory_key(pfs_key), alignment, n_elements)
                 : ut::aligned_new_arr<type>(alignment, n_elements);

    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    for (size_t elem = 0; elem < n_elements; elem++) {
      EXPECT_EQ(ptr[elem].x, 0);
      EXPECT_EQ(ptr[elem].y, 1);
    }
    ut::aligned_delete_arr(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(
    aligned_new_delete_default_constructible_pod_types_arr, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(
    My, aligned_new_delete_default_constructible_pod_types_arr,
    all_default_constructible_pod_types);

// aligned new/delete - array specialization for default constructible non-pod
// types
template <typename T>
class aligned_new_delete_default_constructible_non_pod_types_arr
    : public ::testing::Test {};
TYPED_TEST_SUITE_P(aligned_new_delete_default_constructible_non_pod_types_arr);
TYPED_TEST_P(aligned_new_delete_default_constructible_non_pod_types_arr,
             pod_types) {
  using type = typename TypeParam::type;
  auto with_pfs = TypeParam::with_pfs;
  constexpr size_t n_elements = 5;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr =
        with_pfs ? ut::aligned_new_arr_withkey<type>(
                       ut::make_psi_memory_key(pfs_key), alignment, n_elements)
                 : ut::aligned_new_arr<type>(alignment, n_elements);

    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    for (size_t elem = 0; elem < n_elements; elem++) {
      EXPECT_EQ(ptr[elem].x, 0);
      EXPECT_EQ(ptr[elem].y, 1);
      EXPECT_TRUE(ptr[elem].s == std::string("non-pod-string"));
    }
    ut::aligned_delete_arr(ptr);
  }
}
REGISTER_TYPED_TEST_SUITE_P(
    aligned_new_delete_default_constructible_non_pod_types_arr, pod_types);
INSTANTIATE_TYPED_TEST_SUITE_P(
    My, aligned_new_delete_default_constructible_non_pod_types_arr,
    all_default_constructible_non_pod_types);

TEST(aligned_new_delete, unique_ptr_demo) {
  constexpr auto alignment = 4 * 1024;
  struct Aligned_int_deleter {
    void operator()(int *p) {
      std::cout << "Hello from custom deleter!\n";
      ut::aligned_delete(p);
    }
  };
  std::unique_ptr<int, Aligned_int_deleter> ptr(
      ut::aligned_new<int>(alignment, 1), Aligned_int_deleter{});
}

TEST(aligned_new_delete_arr, unique_ptr_demo) {
  constexpr auto n_elements = 5;
  constexpr auto alignment = 4 * 1024;
  struct Aligned_int_arr_deleter {
    void operator()(int *p) {
      std::cout << "Hello from custom deleter!\n";
      ut::aligned_delete_arr(p);
    }
  };
  std::unique_ptr<int, Aligned_int_arr_deleter> ptr(
      ut::aligned_new_arr<int, n_elements>(alignment, 1, 2, 3, 4, 5),
      Aligned_int_arr_deleter{});
}

TEST(aligned_new_delete_arr, distance_between_elements_in_arr) {
  using type = Default_constructible_pod;
  constexpr size_t n_elements = 5;
  for (auto alignment = 2 * alignof(std::max_align_t);
       alignment < 1024 * 1024 + 1; alignment *= 2) {
    type *ptr = ut::aligned_new_arr<type>(alignment, n_elements);

    EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    for (size_t elem = 1; elem < n_elements; elem++) {
      auto addr_curr = reinterpret_cast<std::uintptr_t>(&ptr[elem]);
      auto addr_prev = reinterpret_cast<std::uintptr_t>(&ptr[elem - 1]);
      auto distance = addr_curr - addr_prev;
      EXPECT_EQ(distance, sizeof(type));
    }
    ut::aligned_delete_arr(ptr);
  }
}

TEST(aligned_pointer, access_data_through_implicit_conversion_operator) {
  constexpr auto alignment = 4 * 1024;
  ut::aligned_pointer<int, alignment> ptr;
  ptr.alloc();

  int *data = ptr;
  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(data) % alignment == 0);
  EXPECT_EQ(*data, int{});

  ptr.dealloc();
}

TEST(aligned_array_pointer, access_data_through_subscript_operator) {
  constexpr auto n_elements = 5;
  constexpr auto alignment = 4 * 1024;
  ut::aligned_array_pointer<Default_constructible_pod, alignment> ptr;
  ptr.alloc(n_elements);

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(&ptr[0]) % alignment == 0);
  for (size_t elem = 0; elem < n_elements; elem++) {
    EXPECT_EQ(ptr[elem].x, 0);
    EXPECT_EQ(ptr[elem].y, 1);
  }

  ptr.dealloc();
}

TEST(aligned_array_pointer, initialize_an_array_of_non_pod_types) {
  constexpr auto n_elements = 5;
  constexpr auto alignment = 4 * 1024;
  ut::aligned_array_pointer<Non_pod_type, alignment> ptr;
  ptr.alloc<n_elements>(1, 2, std::string("a"), 3, 4, std::string("b"), 5, 6,
                        std::string("c"), 7, 8, std::string("d"), 9, 10,
                        std::string("e"));

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(&ptr[0]) % alignment == 0);

  EXPECT_EQ(ptr[0].x, 1);
  EXPECT_EQ(ptr[0].y, 2);
  EXPECT_TRUE(ptr[0].s == std::string("a"));

  EXPECT_EQ(ptr[1].x, 3);
  EXPECT_EQ(ptr[1].y, 4);
  EXPECT_TRUE(ptr[1].s == std::string("b"));

  EXPECT_EQ(ptr[2].x, 5);
  EXPECT_EQ(ptr[2].y, 6);
  EXPECT_TRUE(ptr[2].s == std::string("c"));

  EXPECT_EQ(ptr[3].x, 7);
  EXPECT_EQ(ptr[3].y, 8);
  EXPECT_TRUE(ptr[3].s == std::string("d"));

  EXPECT_EQ(ptr[4].x, 9);
  EXPECT_EQ(ptr[4].y, 10);
  EXPECT_TRUE(ptr[4].s == std::string("e"));

  ptr.dealloc();
}

TEST(aligned_array_pointer, distance_between_elements_in_arr) {
  constexpr auto n_elements = 5;
  constexpr auto alignment = 4 * 1024;
  ut::aligned_array_pointer<Default_constructible_pod, alignment> ptr;
  ptr.alloc(n_elements);

  EXPECT_TRUE(reinterpret_cast<std::uintptr_t>(&ptr[0]) % alignment == 0);

  for (size_t elem = 1; elem < n_elements; elem++) {
    auto addr_curr = reinterpret_cast<std::uintptr_t>(&ptr[elem]);
    auto addr_prev = reinterpret_cast<std::uintptr_t>(&ptr[elem - 1]);
    auto distance = addr_curr - addr_prev;
    EXPECT_EQ(distance, sizeof(Default_constructible_pod));
  }

  ptr.dealloc();
}

}  // namespace innodb_ut0new_unittest
