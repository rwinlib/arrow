// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "arrow/dataset/type_fwd.h"
#include "arrow/dataset/visibility.h"
#include "arrow/util/macros.h"

namespace arrow {
namespace dataset {

/// \brief A granular piece of a Dataset, such as an individual file,
/// which can be read/scanned separately from other fragments.
///
/// A DataFragment yields a collection of RecordBatch, encapsulated in one or
/// more ScanTasks.
class ARROW_DS_EXPORT DataFragment {
 public:
  /// \brief Scan returns an iterator of ScanTasks, each of which yields
  /// RecordBatches from this DataFragment.
  virtual Status Scan(std::shared_ptr<ScanContext> scan_context,
                      ScanTaskIterator* out) = 0;

  /// \brief Return true if the fragment can benefit from parallel
  /// scanning
  virtual bool splittable() const = 0;

  /// \brief Filtering, schema reconciliation, and partition options to use when
  /// scanning this fragment. May be nullptr, which indicates that no filtering
  /// or schema reconciliation will be performed and all partitions will be
  /// scanned.
  virtual std::shared_ptr<ScanOptions> scan_options() const = 0;

  virtual ~DataFragment() = default;
};

/// \brief A trivial DataFragment that yields ScanTask out of a fixed set of
/// RecordBatch.
class ARROW_DS_EXPORT SimpleDataFragment : public DataFragment {
 public:
  explicit SimpleDataFragment(std::vector<std::shared_ptr<RecordBatch>> record_batches);

  Status Scan(std::shared_ptr<ScanContext> scan_context, ScanTaskIterator* out) override;

  bool splittable() const override { return false; }

  std::shared_ptr<ScanOptions> scan_options() const override { return NULLPTR; }

 protected:
  std::vector<std::shared_ptr<RecordBatch>> record_batches_;
};

/// \brief A basic component of a Dataset which yields zero or more
/// DataFragments. A DataSource acts as a discovery mechanism of DataFragments
/// and partitions, e.g. files deeply nested in a directory.
class ARROW_DS_EXPORT DataSource {
 public:
  /// \brief GetFragments returns an iterator of DataFragments. The ScanOptions
  /// controls filtering and schema inference.
  DataFragmentIterator GetFragments(std::shared_ptr<ScanOptions> options);

  /// \brief An expression which evaluates to true for all data viewed by this DataSource.
  /// May be null, which indicates no information is available.
  const std::shared_ptr<Expression>& partition_expression() const {
    return partition_expression_;
  }

  virtual std::string type() const = 0;

  virtual ~DataSource() = default;

 protected:
  DataSource() = default;
  explicit DataSource(std::shared_ptr<Expression> c)
      : partition_expression_(std::move(c)) {}

  virtual DataFragmentIterator GetFragmentsImpl(std::shared_ptr<ScanOptions> options) = 0;

  /// Mutates a ScanOptions by assuming partition_expression_ holds for all yielded
  /// fragments. Returns false if the selector is not satisfiable in this DataSource.
  virtual bool AssumePartitionExpression(
      const std::shared_ptr<ScanOptions>& scan_options,
      std::shared_ptr<ScanOptions>* simplified_scan_options) const;

  std::shared_ptr<Expression> partition_expression_;
};

/// \brief A DataSource consisting of a flat sequence of DataFragments
class ARROW_DS_EXPORT SimpleDataSource : public DataSource {
 public:
  explicit SimpleDataSource(DataFragmentVector fragments)
      : fragments_(std::move(fragments)) {}

  DataFragmentIterator GetFragmentsImpl(std::shared_ptr<ScanOptions> options) override;

  std::string type() const override { return "simple_data_source"; }

 private:
  DataFragmentVector fragments_;
};

/// \brief Top-level interface for a Dataset with fragments coming
/// from possibly multiple sources.
class ARROW_DS_EXPORT Dataset : public std::enable_shared_from_this<Dataset> {
 public:
  /// WARNING, this constructor is not recommend, use Dataset::Make instead.
  /// \param[in] sources one or more input data sources
  /// \param[in] schema a known schema to conform to, may be nullptr
  explicit Dataset(std::vector<std::shared_ptr<DataSource>> sources,
                   std::shared_ptr<Schema> schema)
      : schema_(std::move(schema)), sources_(std::move(sources)) {}

  static Status Make(std::vector<std::shared_ptr<DataSource>> sourcs,
                     std::shared_ptr<Schema> schema, std::shared_ptr<Dataset>* out);

  /// \brief Begin to build a new Scan operation against this Dataset
  Status NewScan(std::unique_ptr<ScannerBuilder>* out);

  const std::vector<std::shared_ptr<DataSource>>& sources() const { return sources_; }

  std::shared_ptr<Schema> schema() const { return schema_; }

 protected:
  // The data sources must conform their output to this schema (with
  // projections and filters taken into account)
  std::shared_ptr<Schema> schema_;

  std::vector<std::shared_ptr<DataSource>> sources_;
};

/// \brief Conditions to apply to a dataset when reading to include or
/// exclude fragments, filter out rows, etc.
struct DataSelector {
  std::vector<std::shared_ptr<Filter>> filters;

  // TODO(wesm): Select specific partition keys, file path globs, or
  // other common desirable selections
};

}  // namespace dataset
}  // namespace arrow
