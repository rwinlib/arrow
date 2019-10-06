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

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "arrow/dataset/file_base.h"
#include "arrow/dataset/filter.h"
#include "arrow/filesystem/localfs.h"
#include "arrow/filesystem/path_util.h"
#include "arrow/record_batch.h"
#include "arrow/testing/gtest_util.h"
#include "arrow/util/io_util.h"
#include "arrow/util/iterator.h"
#include "arrow/util/stl.h"

namespace arrow {
namespace dataset {

using fs::internal::GetAbstractPathExtension;
using internal::TemporaryDir;

class FileSourceFixtureMixin : public ::testing::Test {
 public:
  std::unique_ptr<FileSource> GetSource(std::shared_ptr<Buffer> buffer) {
    return internal::make_unique<FileSource>(std::move(buffer));
  }
};

template <typename Gen>
class GeneratedRecordBatch : public RecordBatchReader {
 public:
  GeneratedRecordBatch(std::shared_ptr<Schema> schema, Gen gen)
      : schema_(schema), gen_(gen) {}

  std::shared_ptr<Schema> schema() const override { return schema_; }

  Status ReadNext(std::shared_ptr<RecordBatch>* batch) override { return gen_(batch); }

 private:
  std::shared_ptr<Schema> schema_;
  Gen gen_;
};

void EnsureRecordBatchReaderDrained(RecordBatchReader* reader) {
  std::shared_ptr<RecordBatch> batch = nullptr;

  ARROW_EXPECT_OK(reader->Next(&batch));
  EXPECT_EQ(batch, nullptr);
}

class DatasetFixtureMixin : public ::testing::Test {
 public:
  DatasetFixtureMixin() : ctx_(std::make_shared<ScanContext>()) {}

  /// \brief Ensure that record batches found in reader are equals to the
  /// record batches yielded by the data fragment.
  void AssertScanTaskEquals(RecordBatchReader* expected, ScanTask* task,
                            bool ensure_drained = true) {
    auto it = task->Scan();
    ARROW_EXPECT_OK(it.Visit([expected](std::shared_ptr<RecordBatch> rhs) -> Status {
      std::shared_ptr<RecordBatch> lhs;
      RETURN_NOT_OK(expected->ReadNext(&lhs));
      EXPECT_NE(lhs, nullptr);
      AssertBatchesEqual(*lhs, *rhs);
      return Status::OK();
    }));

    if (ensure_drained) {
      EnsureRecordBatchReaderDrained(expected);
    }
  }

  /// \brief Ensure that record batches found in reader are equals to the
  /// record batches yielded by the data fragment.
  void AssertFragmentEquals(RecordBatchReader* expected, DataFragment* fragment,
                            bool ensure_drained = true) {
    ScanTaskIterator it;
    ARROW_EXPECT_OK(fragment->Scan(ctx_, &it));

    ARROW_EXPECT_OK(it.Visit([&](std::unique_ptr<ScanTask> task) -> Status {
      AssertScanTaskEquals(expected, task.get(), false);
      return Status::OK();
    }));

    if (ensure_drained) {
      EnsureRecordBatchReaderDrained(expected);
    }
  }

  /// \brief Ensure that record batches found in reader are equals to the
  /// record batches yielded by the data fragments of a source.
  void AssertDataSourceEquals(RecordBatchReader* expected, DataSource* source,
                              bool ensure_drained = true) {
    auto it = source->GetFragments(options_);

    ARROW_EXPECT_OK(it.Visit([&](std::shared_ptr<DataFragment> fragment) -> Status {
      AssertFragmentEquals(expected, fragment.get(), false);
      return Status::OK();
    }));

    if (ensure_drained) {
      EnsureRecordBatchReaderDrained(expected);
    }
  }

  /// \brief Ensure that record batches found in reader are equals to the
  /// record batches yielded by a scanner.
  void AssertScannerEquals(RecordBatchReader* expected, Scanner* scanner,
                           bool ensure_drained = true) {
    auto it = scanner->Scan();

    ARROW_EXPECT_OK(it.Visit([&](std::unique_ptr<ScanTask> task) -> Status {
      AssertScanTaskEquals(expected, task.get(), false);
      return Status::OK();
    }));

    if (ensure_drained) {
      EnsureRecordBatchReaderDrained(expected);
    }
  }

  /// \brief Ensure that record batches found in reader are equals to the
  /// record batches yielded by a dataset.
  void AssertDatasetEquals(RecordBatchReader* expected, Dataset* dataset,
                           bool ensure_drained = true) {
    std::unique_ptr<ScannerBuilder> builder;
    ASSERT_OK(dataset->NewScan(&builder));

    std::unique_ptr<Scanner> scanner;
    ASSERT_OK(builder->Finish(&scanner));

    AssertScannerEquals(expected, scanner.get());

    if (ensure_drained) {
      EnsureRecordBatchReaderDrained(expected);
    }
  }

 protected:
  std::shared_ptr<ScanOptions> options_ = nullptr;
  std::shared_ptr<ScanContext> ctx_;
};

template <typename Format>
class FileSystemBasedDataSourceMixin : public FileSourceFixtureMixin {
 public:
  virtual std::vector<std::string> file_names() const = 0;

  void SetUp() override {
    selector_.base_dir = "/";
    selector_.recursive = true;

    format_ = std::make_shared<Format>();
    schema_ = schema({field("dummy", null())});
    options_ = std::make_shared<ScanOptions>();

    ASSERT_OK(
        TemporaryDir::Make("test-fsdatasource-" + format_->name() + "-", &temp_dir_));
    local_fs_ = std::make_shared<fs::LocalFileSystem>();

    auto path = temp_dir_->path().ToString();
    fs_ = std::make_shared<fs::SubTreeFileSystem>(path, local_fs_);

    for (auto path : file_names()) {
      CreateFile(path, "");
    }

    partition_expression_ = ScalarExpression::Make(true);
  }

  void CreateFile(std::string path, std::string contents) {
    auto parent = fs::internal::GetAbstractPathParent(path).first;
    if (parent != "") {
      ASSERT_OK(this->fs_->CreateDir(parent, true));
    }
    std::shared_ptr<io::OutputStream> file;
    ASSERT_OK(this->fs_->OpenOutputStream(path, &file));
    ASSERT_OK(file->Write(contents));
  }

  void MakeDataSource() {
    ASSERT_OK(FileSystemBasedDataSource::Make(fs_.get(), selector_, format_,
                                              partition_expression_, &source_));
  }

 protected:
  std::function<Status(std::shared_ptr<DataFragment> fragment)> OpenFragments(
      size_t* count) {
    return [this, count](std::shared_ptr<DataFragment> fragment) {
      auto file_fragment =
          internal::checked_pointer_cast<FileBasedDataFragment>(fragment);
      ++*count;
      auto extension =
          fs::internal::GetAbstractPathExtension(file_fragment->source().path());
      EXPECT_TRUE(format_->IsKnownExtension(extension));
      std::shared_ptr<io::RandomAccessFile> f;
      return this->fs_->OpenInputFile(file_fragment->source().path(), &f);
    };
  }

  void NonRecursive() {
    selector_.recursive = false;
    MakeDataSource();

    size_t count = 0;
    ASSERT_OK(source_->GetFragments(options_).Visit(OpenFragments(&count)));
    ASSERT_EQ(count, 1);
  }

  void Recursive() {
    MakeDataSource();

    size_t count = 0;
    ASSERT_OK(source_->GetFragments(options_).Visit(OpenFragments(&count)));
    ASSERT_EQ(count, file_names().size());
  }

  void DeletedFile() {
    MakeDataSource();
    ASSERT_GT(file_names().size(), 0);
    ASSERT_OK(this->fs_->DeleteFile(file_names()[0]));

    size_t count = 0;
    ASSERT_RAISES(IOError, source_->GetFragments(options_).Visit(OpenFragments(&count)));
  }

  void PredicatePushDown() {
    partition_expression_ = equal(field_ref("alpha"), ScalarExpression::Make<int16_t>(3));
    MakeDataSource();

    options_->selector = std::make_shared<DataSelector>();
    options_->selector->filters.resize(1);

    // with a filter identical to the partition condition, all fragments are yielded
    options_->selector->filters[0] =
        std::make_shared<ExpressionFilter>(partition_expression_->Copy());

    size_t count = 0;
    // ASSERT_OK(source_->GetFragments(context_)->Visit(OpenFragments(&count)));
    // ASSERT_EQ(count, file_names().size());

    // with a filter which contradicts the partition condition, no fragments are yielded
    options_->selector->filters[0] = std::make_shared<ExpressionFilter>(
        equal(field_ref("alpha"), ScalarExpression::Make<int16_t>(0)));

    count = 0;
    ASSERT_OK(source_->GetFragments(options_).Visit(OpenFragments(&count)));
    ASSERT_EQ(count, 0);
  }

  fs::Selector selector_;
  std::unique_ptr<FileSystemBasedDataSource> source_;
  std::shared_ptr<fs::LocalFileSystem> local_fs_;
  std::shared_ptr<fs::FileSystem> fs_;
  std::unique_ptr<TemporaryDir> temp_dir_;
  std::shared_ptr<FileFormat> format_;
  std::shared_ptr<Schema> schema_;
  std::shared_ptr<ScanOptions> options_;
  std::shared_ptr<Expression> partition_expression_;
};

template <typename Gen>
std::unique_ptr<GeneratedRecordBatch<Gen>> MakeGeneratedRecordBatch(
    std::shared_ptr<Schema> schema, Gen&& gen) {
  return internal::make_unique<GeneratedRecordBatch<Gen>>(schema, std::forward<Gen>(gen));
}

/// \brief A dummy FileFormat implementation
class DummyFileFormat : public FileFormat {
 public:
  std::string name() const override { return "dummy"; }

  /// \brief Return true if the given file extension
  bool IsKnownExtension(const std::string& ext) const override { return ext == name(); }

  /// \brief Open a file for scanning (always returns an empty iterator)
  Status ScanFile(const FileSource& source, std::shared_ptr<ScanOptions> scan_options,
                  std::shared_ptr<ScanContext> scan_context,
                  ScanTaskIterator* out) const override {
    *out = MakeEmptyIterator<std::unique_ptr<ScanTask>>();
    return Status::OK();
  }

  inline Status MakeFragment(const FileSource& location,
                             std::shared_ptr<ScanOptions> opts,
                             std::unique_ptr<DataFragment>* out) override;
};

class DummyFragment : public FileBasedDataFragment {
 public:
  DummyFragment(const FileSource& source, std::shared_ptr<ScanOptions> options)
      : FileBasedDataFragment(source, std::make_shared<DummyFileFormat>(), options) {}

  bool splittable() const override { return false; }
};

Status DummyFileFormat::MakeFragment(const FileSource& source,
                                     std::shared_ptr<ScanOptions> opts,
                                     std::unique_ptr<DataFragment>* out) {
  *out = internal::make_unique<DummyFragment>(source, opts);
  return Status::OK();
}

}  // namespace dataset
}  // namespace arrow
