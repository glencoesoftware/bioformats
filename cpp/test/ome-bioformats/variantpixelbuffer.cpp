/*
 * #%L
 * OME-BIOFORMATS C++ library for image IO.
 * %%
 * Copyright © 2006 - 2014 Open Microscopy Environment:
 *   - Massachusetts Institute of Technology
 *   - National Institutes of Health
 *   - University of Dundee
 *   - Board of Regents of the University of Wisconsin-Madison
 *   - Glencoe Software, Inc.
 * %%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of any organization.
 * #L%
 */

#include <sstream>
#include <stdexcept>
#include <iostream>

#include <ome/bioformats/VariantPixelBuffer.h>

#include <gtest/gtest.h>
#include <gtest/gtest-death-test.h>

#include "pixel.h"

using ome::bioformats::EndianType;
using ome::bioformats::PixelBuffer;
using ome::bioformats::VariantPixelBuffer;
typedef ome::xml::model::enums::PixelType PT;

/*
 * NOTE: Update equivalent tests in pixelbuffer.cpp when making
 * changes.
 */

class VariantPixelBufferTestParameters
{
public:
  PT         type;
  EndianType endian;

  VariantPixelBufferTestParameters(PT         type,
                                   EndianType endian):
    type(type),
    endian(endian)
  {}
};

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const VariantPixelBufferTestParameters& params)
{
  return os << PT(params.type) << '/'<< params.endian;
}

class VariantPixelBufferTest : public ::testing::TestWithParam<VariantPixelBufferTestParameters>
{
};

TEST_P(VariantPixelBufferTest, DefaultConstruct)
{
  VariantPixelBuffer buf;

  ASSERT_EQ(buf.num_elements(), 1U);
  ASSERT_TRUE(buf.data());
}

TEST_P(VariantPixelBufferTest, ConstructSize)
{
  const VariantPixelBufferTestParameters& params = GetParam();

  VariantPixelBuffer buf(boost::extents[5][2][1][1][1][1][1][1][1],
                         params.type, params.endian);

  ASSERT_EQ(buf.num_elements(), 10U);
  ASSERT_TRUE(buf.data());
}

/*
 * Assign buffer and check.
 */
struct AssignTestVisitor : public boost::static_visitor<>
{
  VariantPixelBuffer& buf;

  AssignTestVisitor(VariantPixelBuffer& buf):
    buf(buf)
  {}

  template<typename T>
  void
  operator() (const T& v)
  {
    typedef typename T::element_type::value_type value_type;

    VariantPixelBuffer::size_type size(buf.num_elements());
    std::vector<value_type> data;
    for (int i = 0; i < size; ++i)
      data.push_back(pixel_value<value_type>(i));
    buf.assign(data.begin(), data.end());

    ASSERT_TRUE(buf.data());
    ASSERT_TRUE(buf.data<value_type>());
    ASSERT_TRUE(v->data());
    for (int i = 0; i < size; ++i)
      {
        ASSERT_EQ(*(buf.data<value_type>()+i), pixel_value<value_type>(i));
      }
  }
};

/*
 * Get index.
 */
struct GetIndexTestVisitor : public boost::static_visitor<>
{
  VariantPixelBuffer& buf;
  const VariantPixelBuffer& cbuf;

  GetIndexTestVisitor(VariantPixelBuffer& buf):
    buf(buf),
    cbuf(buf)
  {}

  template<typename T>
  void
  operator() (const T& /* v */)
  {
    typedef typename T::element_type::value_type value_type;

    ASSERT_EQ(buf.num_elements(), 100U);
    ASSERT_TRUE(buf.data());
    for (int i = 0U; i < 10U; ++i)
      for (int j = 0U; j < 10U; ++j)
        {
          VariantPixelBuffer::indices_type idx;
          idx[0] = i;
          idx[1] = j;
          idx[2] = idx[3] = idx[4] = idx[5] = idx[6] = idx[7] = idx[8] = 0;

          value_type val = pixel_value<value_type>((j * 10) + i);

          EXPECT_EQ(val, buf.at<value_type>(idx));
          EXPECT_EQ(val, cbuf.at<value_type>(idx));
        }
  }
};

/*
 * Set index.
 */
struct SetIndexTestVisitor : public boost::static_visitor<>
{
  VariantPixelBuffer& buf;
  const VariantPixelBuffer& cbuf;

  SetIndexTestVisitor(VariantPixelBuffer& buf):
    buf(buf),
    cbuf(buf)
  {}

  template<typename T>
  void
  operator() (const T& /* v */)
  {
    typedef typename T::element_type::value_type value_type;

    for (int i = 0U; i < 10U; ++i)
      for (int j = 0U; j < 10U; ++j)
        {
          VariantPixelBuffer::indices_type idx;
          idx[0] = i;
          idx[1] = j;
          idx[2] = idx[3] = idx[4] = idx[5] = idx[6] = idx[7] = idx[8] = 0;

          value_type val = pixel_value<value_type>(i + j + j);

          buf.at<value_type>(idx) = val;

          ASSERT_EQ(val, buf.at<value_type>(idx));
          ASSERT_EQ(val, cbuf.at<value_type>(idx));
        }
  }
};

/*
 * Set index death test.
 */
struct SetIndexDeathTestVisitor : public boost::static_visitor<>
{
  VariantPixelBuffer& buf;
  const VariantPixelBuffer& cbuf;

  SetIndexDeathTestVisitor(VariantPixelBuffer& buf):
    buf(buf),
    cbuf(buf)
  {}

  template<typename T>
  void
  operator() (const T& v)
  {
    typedef typename T::element_type::value_type value_type;

    VariantPixelBuffer::indices_type badidx;
    badidx[0] = 13;
    badidx[1] = 2;
    badidx[2] = badidx[3] = badidx[4] = badidx[5] = badidx[6] = badidx[7] = badidx[8] = 0;

    ASSERT_DEATH(buf.at<value_type>(badidx) = value_type(4), "Assertion.*failed");
    ASSERT_DEATH(value_type obs = cbuf.at<value_type>(badidx), "Assertion.*failed");
  }
};

/*
 * Stream input test.
 */
struct StreamInputTestVisitor : public boost::static_visitor<>
{
  VariantPixelBuffer& buf;
  const VariantPixelBuffer& cbuf;

  StreamInputTestVisitor(VariantPixelBuffer& buf):
    buf(buf),
    cbuf(buf)
  {}

  template<typename T>
  void
  operator() (const T& /* v */)
  {
    typedef typename T::element_type::value_type value_type;

    VariantPixelBuffer::size_type size = buf.num_elements();
    std::stringstream ss;

    for (VariantPixelBuffer::size_type i = 0; i < size; ++i)
      {
        value_type val = pixel_value<value_type>(i);
        ss.write(reinterpret_cast<const char *>(&val), sizeof(value_type));
      }

    ss.seekg(0, std::ios::beg);
    ss >> buf;
    EXPECT_FALSE(!ss);

    VariantPixelBuffer::indices_type idx;
    idx[0] = idx[1] = idx[2] = idx[3] = idx[4] = idx[5] = idx[6] = idx[7] = idx[8] = 0;
    std::vector<int>::size_type i = 0;
    for (idx[3] = 0; idx[3] < 4; ++idx[3])
      for (idx[2] = 0; idx[2] < 3; ++idx[2])
        for (idx[1] = 0; idx[1] < 2; ++idx[1])
          for (idx[0] = 0; idx[0] < 2; ++idx[0])
            EXPECT_EQ(pixel_value<value_type>(i++), buf.at<value_type>(idx));
  }
};

/*
 * Stream output test.
 */
struct StreamOutputTestVisitor : public boost::static_visitor<>
{
  VariantPixelBuffer& buf;
  const VariantPixelBuffer& cbuf;

  StreamOutputTestVisitor(VariantPixelBuffer& buf):
    buf(buf),
    cbuf(buf)
  {}

  template<typename T>
  void
  operator() (const T& /* v */)
  {
    typedef typename T::element_type::value_type value_type;

    VariantPixelBuffer::size_type size = buf.num_elements();
    std::stringstream ss;

    std::vector<value_type> vec;
    for (VariantPixelBuffer::size_type i = 0; i < size; ++i)
      {
        value_type val = pixel_value<value_type>(i);
        vec.push_back(val);
      }

    buf.assign(vec.begin(), vec.end());
    ss << buf;
    EXPECT_FALSE(!ss);
    ss.seekg(0, std::ios::beg);

    VariantPixelBuffer::indices_type idx;
    idx[0] = idx[1] = idx[2] = idx[3] = idx[4] = idx[5] = idx[6] = idx[7] = idx[8] = 0;
    std::vector<int>::size_type i = 0;
    for (idx[3] = 0; idx[3] < 4; ++idx[3])
      for (idx[2] = 0; idx[2] < 3; ++idx[2])
        for (idx[1] = 0; idx[1] < 2; ++idx[1])
          for (idx[0] = 0; idx[0] < 2; ++idx[0])
            {
              EXPECT_EQ(pixel_value<value_type>(i), buf.at<value_type>(idx));
              value_type sval;
              ss.read(reinterpret_cast<char *>(&sval), sizeof(value_type));
              EXPECT_FALSE(!ss);
              EXPECT_EQ(sval, pixel_value<value_type>(i));
              ++i;
            }
  }
};

TEST_P(VariantPixelBufferTest, ConstructRange)
{
  const VariantPixelBufferTestParameters& params = GetParam();

  VariantPixelBuffer buf(boost::extents[5][2][1][1][1][1][1][1][1],
                         params.type, params.endian);
  ASSERT_EQ(buf.num_elements(), 10U);

  AssignTestVisitor v(buf);
  boost::apply_visitor(v, buf.vbuffer());
}

TEST_P(VariantPixelBufferTest, ConstructCopy)
{
  const VariantPixelBufferTestParameters& params = GetParam();

  std::vector<boost::endian::native_uint8_t> source1;
  for (boost::endian::native_uint8_t i = 0U; i < 10U; ++i)
    source1.push_back(i);

  std::vector<boost::endian::native_uint8_t> source2;
  for (boost::endian::native_uint8_t i = 10U; i < 20U; ++i)
    source2.push_back(i);

  VariantPixelBuffer buf1(boost::extents[5][2][1][1][1][1][1][1][1],
                          params.type, params.endian);
  ASSERT_EQ(buf1.num_elements(), 10U);
  AssignTestVisitor v1(buf1);
  boost::apply_visitor(v1, buf1.vbuffer());

  VariantPixelBuffer buf2(boost::extents[5][2][1][1][1][1][1][1][1],
                          params.type, params.endian);
  ASSERT_EQ(buf1.num_elements(), 10U);
  AssignTestVisitor v2(buf1);
  boost::apply_visitor(v2, buf1.vbuffer());

  ASSERT_EQ(buf1, buf1);
  ASSERT_EQ(buf2, buf2);
  ASSERT_NE(buf1, buf2);

  VariantPixelBuffer buf3(buf2);
  ASSERT_EQ(buf2, buf3);
  ASSERT_NE(buf1, buf2);
}

TEST_P(VariantPixelBufferTest, GetIndex)
{
  const VariantPixelBufferTestParameters& params = GetParam();

  VariantPixelBuffer buf(boost::extents[10][10][1][1][1][1][1][1][1],
                         params.type, params.endian);
  ASSERT_EQ(buf.num_elements(), 100U);
  ASSERT_TRUE(buf.data());

  AssignTestVisitor v1(buf);
  boost::apply_visitor(v1, buf.vbuffer());
  GetIndexTestVisitor v2(buf);
  boost::apply_visitor(v2, buf.vbuffer());
}

TEST_P(VariantPixelBufferTest, SetIndex)
{
  const VariantPixelBufferTestParameters& params = GetParam();

  VariantPixelBuffer buf(boost::extents[10][10][1][1][1][1][1][1][1],
                         params.type, params.endian);
  ASSERT_EQ(buf.num_elements(), 100U);
  ASSERT_TRUE(buf.data());

  SetIndexTestVisitor v(buf);
  boost::apply_visitor(v, buf.vbuffer());
}

TEST_P(VariantPixelBufferTest, SetIndexDeathTest)
{
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";

  const VariantPixelBufferTestParameters& params = GetParam();

  VariantPixelBuffer buf(boost::extents[10][10][1][1][1][1][1][1][1],
                         params.type, params.endian);

  SetIndexTestVisitor v(buf);
  boost::apply_visitor(v, buf.vbuffer());
}

TEST_P(VariantPixelBufferTest, StreamInput)
{
  const VariantPixelBufferTestParameters& params = GetParam();

  VariantPixelBuffer buf(boost::extents[2][2][3][4][1][1][1][1][1],
                         params.type, params.endian);

  StreamInputTestVisitor v(buf);
  boost::apply_visitor(v, buf.vbuffer());
}

TEST_P(VariantPixelBufferTest, StreamOutput)
{
  const VariantPixelBufferTestParameters& params = GetParam();

  VariantPixelBuffer buf(boost::extents[2][2][3][4][1][1][1][1][1],
                         params.type, params.endian);

  StreamOutputTestVisitor v(buf);
  boost::apply_visitor(v, buf.vbuffer());
}

VariantPixelBufferTestParameters variant_params[] =
  { //                               PixelType          EndianType
    VariantPixelBufferTestParameters(PT::INT8,          ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::INT8,          ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::INT8,          ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::INT16,         ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::INT16,         ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::INT16,         ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::INT32,         ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::INT32,         ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::INT32,         ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::UINT8,         ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::UINT8,         ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::UINT8,         ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::UINT16,        ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::UINT16,        ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::UINT16,        ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::UINT32,        ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::UINT32,        ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::UINT32,        ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::FLOAT,         ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::FLOAT,         ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::FLOAT,         ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::DOUBLE,        ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::DOUBLE,        ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::DOUBLE,        ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::BIT,           ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::BIT,           ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::BIT,           ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::COMPLEX,       ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::COMPLEX,       ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::COMPLEX,       ome::bioformats::NATIVE),

    VariantPixelBufferTestParameters(PT::DOUBLECOMPLEX, ome::bioformats::BIG),
    VariantPixelBufferTestParameters(PT::DOUBLECOMPLEX, ome::bioformats::LITTLE),
    VariantPixelBufferTestParameters(PT::DOUBLECOMPLEX, ome::bioformats::NATIVE)
  };

// Disable missing-prototypes warning for INSTANTIATE_TEST_CASE_P;
// this is solely to work around a missing prototype in gtest.
#ifdef __GNUC__
#  if defined __clang__ || defined __APPLE__
#    pragma GCC diagnostic ignored "-Wmissing-prototypes"
#  endif
#endif

INSTANTIATE_TEST_CASE_P(VariantPixelBufferVariants, VariantPixelBufferTest, ::testing::ValuesIn(variant_params));
